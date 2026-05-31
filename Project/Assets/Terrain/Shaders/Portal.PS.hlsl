struct VSOutput {
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD0;
	float3 Normal : NORMAL0;
	float3 LocalNormal : NORMAL1;
	nointerpolation uint InstanceID : ID0;
};

struct PSOutput {
	float4 Diffuse : SV_TARGET0;
	//float4 Normal : SV_TARGET1;
};

static const uint3 Permutations_CX0[6] = {
	{ 0U, 1U, 2U, },
	{ 1U, 0U, 2U, },
	{ 2U, 0U, 1U, },
	{ 2U, 1U, 0U, },
	{ 1U, 2U, 0U, },
	{ 0U, 2U, 1U, },
};

struct HSV {
	float H;
	float S;
	float V;
};

float3 HSVToRGB(in HSV hsv_) {
	hsv_.H = fmod(hsv_.H, 1.0f);
	hsv_.H = (hsv_.H < 0.0f) ? (hsv_.H + 1.0f) : (hsv_.H);

	const float chroma = hsv_.S * hsv_.V;
	const float hue_Prime = hsv_.H * 6.0f;

	const float m = hsv_.V - chroma;

	int hueSection = (int) floor(hue_Prime);
	hueSection = (hueSection + 6) % 6;

	const float3 CX0 = {
		chroma,
		chroma * (1.0f - abs(fmod(hue_Prime, 2.0f) - 1.0f)),
		0.0f,
	};
	
	return float3(
		CX0[Permutations_CX0[hueSection].x] + m,
		CX0[Permutations_CX0[hueSection].y] + m,
		CX0[Permutations_CX0[hueSection].z] + m
	);
}

cbuffer Constants : register(b0) {
	float4x4 WorldToProjective;
	float Time;
}

StructuredBuffer<float3> BaseColors : register(t0, space1);

Texture2D<float4> Texture : register(t0);

Texture2D<float4> GBuffer_Albedo : register(t0, space2);
Texture2D<float4> GBuffer_Normal : register(t0, space2);
Texture2D<float> GBuffer_Depth : register(t3, space2);

SamplerState Sampler : register(s0);

PSOutput main(VSOutput input_) {
	PSOutput output;
	const float4 texColor = Texture.Sample(Sampler, input_.TexCoord);
	output.Diffuse = texColor;
	const float3 normal = normalize(input_.Normal);
	const float3 localNormal = normalize(input_.LocalNormal);
	const HSV hsv = {
		localNormal.z + Time,
		0.4f,
		0.25f
	};
	const float3 rgb = HSVToRGB(hsv);
	output.Diffuse.rgb *= rgb * 0.65f + BaseColors[input_.InstanceID].rgb * 0.35f;
	
	const float depth = GBuffer_Depth.Load(int3(input_.Position.xy, 0.0f));
	output.Diffuse.a = depth < input_.Position.z ? output.Diffuse.a * 0.1f : output.Diffuse.a;
	
	return output;
}