#include"Watercolor.hlsli"
#include"MathUtils.hlsli"

cbuffer Paramaters : register(b0) {
	// * Overall darkening strength, approx. 0.3f to 0.6f 
	float EdgeStrength;
	// * (~0.08f, ~0.55f)
	// * Wetness beyond the thresholds leads to fully darkening or no darkening.
	float DryingThreshold_LO;
	float DryingThreshold_HI;
	// * Curvature below the threshold is ignored, approx. 0.04f
	float CurvatureThreshold;
	// * Approx. 0.12f
	float CurvatureSoftness;
	float PaperScale;
	int2 TextureSize;
	// * Inverse of texture size
	float2 TexelSize;
};


float BoundaryMask(float wetness_) {
	// Rising edge: starts at dryingHi, full at midpoint
	float rising = smoothstep(
		DryingThreshold_HI,
		(DryingThreshold_LO + DryingThreshold_HI) * 0.5f,
		wetness_
	);
	
    // Falling edge: full at midpoint, gone below dryingLo
	float falling = smoothstep(
		DryingThreshold_LO,
		(DryingThreshold_LO + DryingThreshold_HI) * 0.5,
		wetness_
	);
	
	return rising * falling;
}

// * Wide-radius curvature sample for band width control
float CurvatureWide(uint2 coord_, int radius_) {
	float sum = 0.0f;
	float wSum = 0.0f;
	
	for (int dx = -radius_; dx <= radius_; dx++) {
		for (int dy = -radius_; dy <= radius_; dy++) {
			float w = 1.0f / (1.0f + length(float2(dx, dy)));
			sum += GBuffer::SRV::Curvature[coord_ + uint2(dx, dy)] * w;
			wSum += w;
		}
	}
	return sum / wSum;
}

#define SUSPENDED xyz
#define DEPOSITED w

[numthreads(8, 8, 1)]
void Simulate(uint3 tid_ : SV_DispatchThreadID) {
	const uint2 coord = tid_.xy;
	float4 pigment = SimulationMap::SRV::PigmentConcentration.Load(int3(coord, 0));

	const float wetness = SimulationMap::SRV::Wetness.Load(int3(coord, 0));
	const float curvature = GBuffer::SRV::Curvature.Load(int3(coord, 0));
	
	// Use both sharp and wide curvature
	const float curvature_Sharp = GBuffer::SRV::Curvature.Load(int3(coord, 0));
	const float curvature_Wide = CurvatureWide(coord, 2);

    // --- Curvature mask ---
    // Soft threshold so flat surfaces stay clean
	float curvMask = smoothstep(
		CurvatureThreshold,
		CurvatureThreshold + CurvatureSoftness,
		lerp(curvature_Sharp, curvature_Wide, 0.4f)
	);

    // --- Drying front mask ---
    // Darkening only happens during the drying transition,
    // not while soaking wet and not after fully dried
	float dryMask = BoundaryMask(wetness);

	
    // --- Combined accumulation weight ---
	float accumWeight = dryMask * curvMask * EdgeStrength;

    // --- Capillary flow ---
    // Pigment flows toward the highest nearby curvature.
    // Sample neighbours and pull concentration from lower-curvature texels.
	float2 grad_Curvature = Gradient(GBuffer::SRV::Curvature, TextureSize, coord);
	float flowMag = length(grad_Curvature);

    // Upstream offset — where pigment is flowing FROM
    // (opposite of gradient direction, i.e. low-curv toward high-curv)
	float2 upstream = -normalize(grad_Curvature + 1e-5) * TexelSize * 1.5;
	float2 uvCoord = (float2(coord) + 0.5) * TexelSize;
	float3 upstreamC = SimulationMap::SRV::PigmentConcentration.SampleLevel(BilinearClamp, uvCoord + upstream, 0).rgb;

    // Flow contribution: pull pigment from upstream, scaled by flow magnitude
	float flowStr = saturate(flowMag * 8.0) * wetness * accumWeight;
	float3 flowContrib = (upstreamC - pigment.SUSPENDED) * flowStr;

    // --- Direct accumulation at high-curvature sites ---
    // Separate from flow — pigment that was already here pools further
	float3 poolContrib = pigment.SUSPENDED * accumWeight * 0.4;

    // --- Apply both contributions to suspended ---
	pigment.SUSPENDED = pigment.SUSPENDED + flowContrib + poolContrib;
	pigment.SUSPENDED = max(pigment.SUSPENDED, 0.0);

    // --- Paper texture modulation ---
    // Edge darkening is stronger in paper valleys (more surface contact)
	float2 paperUV = (float2(coord) + 0.5) * TexelSize * PaperScale;
	float paper = ImageTexture_Paper.SampleLevel(BilinearClamp, paperUV, 0).r;
	float paperMod = lerp(1.1, 0.9, paper); // valleys accumulate more
	pigment.SUSPENDED *= paperMod;

    // --- Deposit extra suspended pigment at edges ---
    // During peak darkening, force some suspended into fixed
	float edgeDeposit = accumWeight * curvMask * 0.06;
	float3 deposited = pigment.SUSPENDED * edgeDeposit;
	pigment.SUSPENDED -= deposited;
	pigment.DEPOSITED += dot(deposited, float3(0.299, 0.587, 0.114));

	SimulationMap::UAV::PigmentConcentration[coord] = saturate(pigment);
}