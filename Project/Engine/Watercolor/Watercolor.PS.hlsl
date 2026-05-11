#include"Watercolor.hlsli"

cbuffer Parameters : register(b0) {
	float PaperStrength; // paper modulation intensity,  ~0.12–0.25
	float ShadowDesaturationStrength; // shadow desaturation,         ~0.3–0.5
	float BloomStrength; // luminance bleed radius,      ~0.004–0.010
	float PaperScale;
	float2 TexelSize;
};

#define SUSPENDED rgb
#define FIXED a
#define ALBEDO rgb
#define WETNESS a

struct PSInput {
	float2 TexCoord : TEXCOORD0;
};

struct PSOutput {
	float4 Color;
};

struct KSCoefficients {
	float3 K; // absorption
	float3 S; // scattering
};

KSCoefficients GetPigmentKS(in float3 albedo_) {
	float3 r = clamp(albedo_, 0.01f, 0.99f);
	float3 kOverS = (1.0f - r) * (1.0f - r) / (2.0f * r);
	float3 s = float3(0.02f, 0.02f, 0.02f);
	float3 k = kOverS * s;

	KSCoefficients c;
	c.K = k;
	c.S = s;
	return c;
}

KSCoefficients StackLayers(in KSCoefficients top_, in KSCoefficients bot_, float topConcentraition_) {
	KSCoefficients ret;
	ret.K = top_.K * topConcentraition_ + bot_.K;
	ret.S = top_.S * topConcentraition_ + bot_.S;
	return ret;
}

float3 KMReflectance(in KSCoefficients ks_, float graycale_) {
	float3 ks = ks_.K / max(ks_.S, 1e-5f);
	
	float3 a = 1.0f + ks;
	float3 b = sqrt(ks * ks + 2.0f * ks);
	
	float3 bS = b * ks_.S;
	float3 coth = 1.0f / tanh(clamp(bS, 0.001f, 20.0f));

	float3 num = 1.0f - graycale_ * (a - b * coth);
	float3 den = a - graycale_ + b * coth;
	float3 r = num / max(den, 1e-5f);

	return saturate(r);
}

float3 PaperReflectance(float2 uv_) {
	float h = ImageTexture_Paper.SampleLevel(BilinearClamp, uv_ * PaperScale, 0.0f).r;
	
	float3 paperPeak = float3(0.99f, 0.98f, 0.97f);
	float3 paperValley = float3(0.88f, 0.86f, 0.82f);

	return lerp(paperValley, paperPeak, h);
}

float3 LuminanceBleed(float2 uv_, in float3 center_) {
	float4 sum = center_ * 0.5f;
	sum += SimulationMap::SRV::PigmentConcentration.SampleLevel(BilinearClamp, uv_ + float2(BloomStrength, 0.0f), 0.0f).rgb * 0.125f;
	sum += SimulationMap::SRV::PigmentConcentration.SampleLevel(BilinearClamp, uv_ + float2(-BloomStrength, 0.0f), 0.0f).rgb * 0.125f;
	sum += SimulationMap::SRV::PigmentConcentration.SampleLevel(BilinearClamp, uv_ + float2(0.0f, BloomStrength), 0.0f).rgb * 0.125f;
	sum += SimulationMap::SRV::PigmentConcentration.SampleLevel(BilinearClamp, uv_ + float2(0.0f, -BloomStrength), 0.0f).rgb * 0.125f;
	return sum;
}

float3 DesaturateShadows(in float3 color_) {
	float lum = dot(color_, float3(0.299f, 0.587f, 0.114f));

	// Shadow threshold — below 0.35 luminance = shadow region
	float shadowMask = 1.0f - smoothstep(0.15f, 0.40f, lum);

	// Desaturate in shadows
	float3 desat = lerp(color_, float3(lum, lum, lum), shadowMask * ShadowDesaturationStrength);

	// Slight warm shift in deep shadows (quinacridone violet undertone)
	float3 warmShift = float3(0.02f, -0.01f, 0.03f) * shadowMask * 0.5f;

	return desat + warmShift;
}

PSOutput CalculateKS(in PSInput input_) {
	float4 pigment = SimulationMap::SRV::PigmentConcentration.Sample(BilinearClamp, input_.TexCoord);
	float3 suspended = pigment.rgb; // suspended concentration
	float deposited = pigment.a; // deposited concentration
	
	float3 albedo = GBuffer::SRV::AlbedoWetness.Sample(BilinearClamp, input_.TexCoord).rgb;
	float wetness = SimulationMap::SRV::Wetness.Sample(BilinearClamp, input_.TexCoord);
	
	KSCoefficients pigmentKS = GetPigmentKS(albedo);
	
	float3 paperReflenctance = PaperReflectance(input_.TexCoord);
	float reflGrayscale = dot(paperReflenctance, float3(0.299f, 0.587f, 0.114f)); // scalar substrate
	
	KSCoefficients depositedLayer;
	depositedLayer.K = pigmentKS.K * deposited * 6.0f; // concentrated at deposit sites
	depositedLayer.S = pigmentKS.S * deposited * 1.5f;

	// Suspended pigment layer (thin wash, very low scatter)
	KSCoefficients suspendedLayer;
	suspendedLayer.K = pigmentKS.K * dot(suspended, float3(0.299f, 0.587f, 0.114f)) * 3.0f;
	suspendedLayer.S = pigmentKS.S * 0.3f; // suspended = barely scatters
	
	KSCoefficients combined = StackLayers(
		suspendedLayer, depositedLayer,
		dot(suspended, float3(0.333f, 0.333f, 0.333f))
	);
	
	float3 reflectance = KMReflectance(combined, reflGrayscale);
	
	reflectance *= lerp(float3(1.0f, 1.0f, 1.0f), paperReflenctance, 1.0f - deposited);

	float wetBloom = smoothstep(0.1f, 0.6f, wetness);
	reflectance = lerp(
		reflectance,
		LuminanceBleed(input_.TexCoord, reflectance),
		wetBloom * 0.35f
	);
	
	reflectance = DesaturateShadows(reflectance);
	
	float lum = dot(reflectance, float3(0.299f, 0.587f, 0.114f));
	reflectance = lerp(float3(lum, lum, lum), reflectance, 1.12f);
	reflectance = saturate(reflectance);

	PSOutput output;
	output.Color = float4(reflectance, 1.0f);
	
	return output;
}