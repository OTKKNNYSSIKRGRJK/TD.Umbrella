#include"Watercolor.hlsli"
#include"MathUtils.hlsli"

cbuffer Paramaters : register(b0) {
	float GranulationStrength; // ~0.15–0.45
	float FlocculationStrength; // flocculation intensity,       ~0.08–0.25
	float GranularityCoefficient; // per-material granularity,     ~0.0–1.0
	float SettlingRate; // how fast particles settle,    ~0.03–0.08
	float PaperScale; // paper texture tiling
	float FlocculationScale; // flocculation pattern scale,   ~0.04–0.12
	float2 TexelSize;
	float DeltaTime;
	float Time; // for animated flocculation
};



float Hash(float2 p_) {
	p_ = frac(p_ * float2(543.21f, 234.56f));
	p_ += dot(p_, p_ + 67.89f);
	return frac(p_.x * p_.y);
}

float ValueNoise(float2 p_) {
	float2 i = floor(p_);
	float2 f = frac(p_);
	float2 u = f * f * (3.0f - 2.0f * f); // smoothstep

	return lerp(
		lerp(Hash(i + float2(0.0f, 0.0f)), Hash(i + float2(1.0f, 0.0f)), u.x),
		lerp(Hash(i + float2(0.0f, 1.0f)), Hash(i + float2(1.0f, 1.0f)), u.x),
		u.y
	);
}

float FlocculationPattern(float2 uv_) {
	float2 p = uv_ / FlocculationScale;

	// * slowly drift over time
	// * flocculation pattern evolves as wash dries
	p += Time * 0.004f;
	
	float n = ValueNoise(p) * 0.625f;
	n += ValueNoise(p * 2.3f) * 0.375f;

	// * sharpen into clumps
	return pow(saturate(n), 1.8f);
}

float CavityDepth(float2 uv_) {
	float h = ImageTexture_Paper.SampleLevel(BilinearClamp, uv_ * PaperScale, 0.0f).r;
	return (1.0f - h);
}

// * local paper relief:
// * how much this texel stands out from its neighbourhood. High relief = prominent peak or valley
// * used to sharpen granulation on pronounced paper texture
float PaperRelief(float2 uv_) {
	float2 off = TexelSize * PaperScale;
	float c = ImageTexture_Paper.SampleLevel(BilinearClamp, uv_ * PaperScale, 0.0f).r;
	float h = ImageTexture_Paper.SampleLevel(
		BilinearClamp,
		uv_ * PaperScale +
		off.x * MathUtils::CoordOffsets[WaveGetLaneIndex() & 0x03U],
		0.0f
	).r;
	float hE = WaveReadLaneAt(h, 0U);
	float hW = WaveReadLaneAt(h, 1U);
	float hN = WaveReadLaneAt(h, 2U);
	float hS = WaveReadLaneAt(h, 3U);

	float lap = abs((hE + hW + hN + hS) - 4.0f * c);
	return saturate(lap * 6.0f); // scale to ~0..1 range
}

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
	uint2 coord = tid.xy;
	float4 pigment = SimulationMap::SRV::PigmentConcentration[coord];
	float3 suspended = pigment.rgb;
	float deposited = pigment.a;

	float wetness = SimulationMap::SRV::Wetness.Load(int3(coord, 0));
	float2 uv = (float2(coord) + 0.5f) * TexelSize;

	// Granulation activates as paper dries
	// Peak effect in the drying window, zero when wet or fully dry
	float dryPhase = smoothstep(0.55f, 0.0f, wetness); // 0 when wet, 1 when dry
	float wetFloor = smoothstep(0.0f, 0.05f, wetness); // zero out fully dry (no movement)
	float granulationPhase = dryPhase * wetFloor;

	// Skip if nothing to do
	if (granulationPhase < 0.005f || GranulationStrength < 0.001f) {
		SimulationMap::UAV::PigmentConcentration[coord] = pigment;
		return;
	}

	// --- Paper data ---
	float cavity = CavityDepth(uv); // 0 = peak, 1 = valley
	float relief = PaperRelief(uv); // how prominent this feature is

	// --- Granulation settling ---
	// Pigment migrates from suspended into fixed, biased by cavity depth.
	// Valley texels accumulate more; peak texels accumulate less.

	// Base settling amount this frame
	float baseSettle = SettlingRate * granulationPhase * DeltaTime;

	// Cavity bias: valleys settle faster, peaks slower
	// cavity = 1 in valleys → more deposition
	// cavity = 0 at peaks   → less deposition
	float cavityBias = lerp(0.4f, 1.6f, cavity);

	// Pigment-specific granulation coefficient
	// Pass per-material via constant buffer or a material ID lookup
	float settleAmount =
		baseSettle * cavityBias * GranularityCoefficient *
		GranulationStrength * relief;

	// Transfer suspended → fixed
	float3 depositing = suspended * settleAmount;
	suspended -= depositing;
	deposited += dot(depositing, float3(0.299f, 0.587f, 0.114f));


	// --- Flocculation ---
	// Pigment clumps into blotchy clusters at a larger scale.
	// Separate from granulation — this is about particle aggregation,
	// not paper topology.

	float flocPattern = FlocculationPattern(uv);

	// Flocculation redistributes suspended pigment — concentrates it
	// in clump centres, depletes it between clumps.
	// Net mass is roughly conserved (we add and subtract equally on average).
	float flocMod = (flocPattern - 0.5f) * 2.0f; // remap to [-1, 1]

	// Scale by wetness phase and strength
	// Flocculation peaks slightly earlier than granulation (while still wet)
	float flocPhase = smoothstep(0.7f, 0.1f, wetness) * smoothstep(0.0f, 0.15f, wetness);
	float flocScale = FlocculationStrength * flocPhase * GranularityCoefficient;

	// * concentrate in clumps
	suspended *= (1.0f + flocMod * flocScale);
	suspended = max(suspended, 0.0f);


	// --- Relief sharpening ---
	// On prominent paper features, push the effect harder.
	// This makes coarse paper look coarse and smooth paper look smooth.
	float reliefBoost = 1.0 + relief * 0.4 * GranulationStrength;
	deposited = saturate(deposited * reliefBoost);


	SimulationMap::UAV::PigmentConcentration[coord] = float4(saturate(suspended), saturate(deposited));
}