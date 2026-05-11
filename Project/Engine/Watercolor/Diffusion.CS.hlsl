#include"Watercolor.hlsli"
#include"MathUtils.hlsli"

cbuffer Paramaters : register(b0) {
	float DiffusionRate; // ~0.10–0.18
	float AdvectionStrength; // paper-guided advection,       ~0.06–0.14
	float BackrunStrength; // wetness gradient blooms,      ~0.08–0.20
	float FlowPersistence; // how long advection direction holds, ~0.6
	float DeltaTime; // timestep, typically 1/60 or fixed 0.016
	float PaperScale;
	int2 TextureSize;
	float2 TexelSize;
};

// ---------------------------------------------------------------
// Anisotropic diffusion tensor
// Diffusion is faster along paper fibres than across them.
// The tensor is derived from the paper gradient — flow is
// strongest perpendicular to the gradient (along contour lines).
// ---------------------------------------------------------------
float3 AnisotropicDiffusion(in float2 gradient_Paper_, in uint2 coord_, in float3 suspended_Center_, float wetness_) {
	float gLen = length(gradient_Paper_);

	// Tangent direction (perpendicular to gradient = along fibres)
	float2 tangent = (gLen > 1e-4f)
		? float2(-gradient_Paper_.y, gradient_Paper_.x) / gLen
		: float2(1.0f, 0.0f);

	// Diffuse more along tangent, less across it
	float dAlong = DiffusionRate * 1.4f;
	float dAcross = DiffusionRate * 0.6f;

	// 4-tap neighbours
	
	const int2 coord_Adjacent = MathUtils::AdjacentCoordinate(TextureSize, coord_);
	const float3 suspended_Adjacent = SimulationMap::SRV::PigmentConcentration.Load(int3(coord_Adjacent, 0)).rgb;

	float3 cE = WaveReadLaneAt(suspended_Adjacent, 0U);
	float3 cW = WaveReadLaneAt(suspended_Adjacent, 1U);
	float3 cN = WaveReadLaneAt(suspended_Adjacent, 2U);
	float3 cS = WaveReadLaneAt(suspended_Adjacent, 3U);

	// Weight each neighbour by how aligned it is with the tangent
	float2 eDir = float2(1.0f, 0.0f);
	float2 nDir = float2(0.0f, 1.0f);

	float wE = lerp(dAcross, dAlong, abs(dot(eDir, tangent)));
	float wW = wE;
	float wN = lerp(dAcross, dAlong, abs(dot(nDir, tangent)));
	float wS = wN;

	// Weighted Laplacian
	float3 lap = cE * wE + cW * wW + cN * wN + cS * wS
			   - suspended_Center_ * (wE + wW + wN + wS);

	return suspended_Center_ + DeltaTime * wetness_ * lap;
}


// ---------------------------------------------------------------
// Advection — pigment carried by paper-surface flow
// ---------------------------------------------------------------
float3 Advection(in float2 gradient_Paper_, in float2 uv_, in float3 center_, float wetness_) {
	// Flow direction: downhill on the paper surface
	// Scale by wetness — no flow on dry paper
	float2 flowDir = -gradient_Paper_ * AdvectionStrength * wetness_;

	// Sample upstream position (where pigment is flowing FROM)
	float2 upstream = uv_ - flowDir * TexelSize * 2.0f;
	float3 upstreamC = SimulationMap::SRV::PigmentConcentration.SampleLevel(BilinearClamp, upstream, 0).rgb;

	// Blend toward upstream concentration
	float flowMag = length(flowDir);
	return lerp(center_, upstreamC, saturate(flowMag * 8.0f));
}


// ---------------------------------------------------------------
// Backrun — wetter region invades drier neighbour
// Creates hard-edged blooms, characteristic of wet-on-wet
// ---------------------------------------------------------------
float3 Backrun(in uint2 coord_, in float3 center_, float wetness_) {
	const int2 coord_Adjacent = MathUtils::AdjacentCoordinate(TextureSize, coord_);
	const float wetness_Adjacent = SimulationMap::SRV::Wetness.Load(int3(coord_Adjacent, 0));
	const float4 pigment_Adjacent = SimulationMap::SRV::PigmentConcentration.Load(int3(coord_Adjacent, 0));
	
	float wE = WaveReadLaneAt(wetness_Adjacent, 0U);
	float wW = WaveReadLaneAt(wetness_Adjacent, 1U);
	float wN = WaveReadLaneAt(wetness_Adjacent, 2U);
	float wS = WaveReadLaneAt(wetness_Adjacent, 3U);

	float3 cE = WaveReadLaneAt(pigment_Adjacent, 0U).xyz;
	float3 cW = WaveReadLaneAt(pigment_Adjacent, 1U).xyz;
	float3 cN = WaveReadLaneAt(pigment_Adjacent, 2U).xyz;
	float3 cS = WaveReadLaneAt(pigment_Adjacent, 3U).xyz;

	// A wetter neighbour pushes its pigment INTO this texel
	// A drier neighbour cannot pull pigment out
	float3 backrunContrib = float3(0.0f, 0.0f, 0.0f);

	float dE = max(wE - wetness_, 0.0f); // positive = neighbour wetter than us
	float dW = max(wW - wetness_, 0.0f);
	float dN = max(wN - wetness_, 0.0f);
	float dS = max(wS - wetness_, 0.0f);

	backrunContrib += cE * dE;
	backrunContrib += cW * dW;
	backrunContrib += cN * dN;
	backrunContrib += cS * dS;

	// Scale by backrun strength and current wetness
	// (backruns need the receiving area to have some moisture)
	float receiverWet = smoothstep(0.05f, 0.3f, wetness_);
	return center_ + backrunContrib * BackrunStrength * receiverWet * DeltaTime;
}



// ---------------------------------------------------------------
// Main
// ---------------------------------------------------------------
[numthreads(8, 8, 1)]
void main(uint3 tid_ : SV_DispatchThreadID) {
	const uint2 coord = tid_.xy;
	const float4 pigment = SimulationMap::SRV::PigmentConcentration.Load(int3(coord, 0));
	const float3 suspended = pigment.rgb;
	const float deposited = pigment.a;
	
	float2 grad_Paper = Gradient(ImageTexture_Paper, TextureSize, coord);

	const float wetness = SimulationMap::SRV::Wetness.Load(int3(coord, 0));
	float2 uv = (float2(coord) + 0.5f) * TexelSize;

	// Short-circuit: no simulation on dry paper
	if (wetness < 0.01f) {
		SimulationMap::UAV::PigmentConcentration[coord] = pigment;
		return;
	}

	// 1. Anisotropic diffusion along paper fibres
	float3 afterDiffision = AnisotropicDiffusion(grad_Paper, coord, suspended, wetness);

	// 2. Advection along paper surface gradient
	float3 afterAdvection = Advection(grad_Paper, uv, afterDiffision, wetness);

	// 3. Backrun from wetter neighbours
	float3 afterBackrun = Backrun(coord, afterAdvection, wetness);

	// 4. Conservation nudge
	// Diffusion should roughly conserve pigment mass.
	// A small correction prevents gradual brightening from
	// numerical drift over many iterations.
	float3 mass_In = suspended;
	float3 mass_Out = afterBackrun;
	float3 drift = mass_Out - mass_In;
	// Allow upward drift (backruns add pigment from neighbours — correct)
	// Clamp downward drift to avoid pigment disappearing
	afterBackrun = max(afterBackrun, suspended * 0.98f);

	// 5. Clamp
	float3 suspended_New = saturate(afterBackrun);

	SimulationMap::UAV::PigmentConcentration[coord] = float4(suspended_New, deposited);
}