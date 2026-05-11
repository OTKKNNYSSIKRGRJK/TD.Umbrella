#include"Watercolor.hlsli"
#include"MathUtils.hlsli"

cbuffer Parameters : register(b0) {
	float DeltaTime;
	float DiffusionRate;
	float DepositionRate;
	float EdgeAccumulationStrength;
	
	int2 TextureSize;
};



float Grayscale(in float3 rgb_) {
	return dot(rgb_, float3(0.299f, 0.587f, 0.114f));
}



#define SUSPENDED xyz
#define FIXED w

namespace Simulation {	
	static const int2 CoordOffsets[4] = {
		{ 0, 1, 0 },
		{ 0, -1, 0 },
		{ 1, 0, 0 },
		{ -1, 0, 0 },
	};

	void Suspension(
		inout float4 pigment_OUT_,
		in float4 pigment_IN_,
		in float wetness_,
		in int2 coord_
	) {
		const float4 laplacian = Laplacian(
			SimulationMap::SRV::PigmentConcentration,
			TextureSize,
			coord_
		);
		pigment_OUT_.SUSPENDED =
			pigment_IN_.SUSPENDED +
			DiffusionRate * DeltaTime * wetness_ * max(laplacian.SUSPENDED, 0.0f);
		pigment_OUT_.SUSPENDED = max(pigment_OUT_.SUSPENDED, float3(0.0f, 0.0f, 0.0f));
	}
	
	void Deposition(
		inout float4 pigment_OUT_,
		out float dryingFront_,
		in float4 pigment_IN_,
		in float wetness_
	) {
		// * Deposition rate peaks as paper dries.
		// * `smoothstep` gives the drying-front behaviour.
		
		dryingFront_ = smoothstep(0.5f, 0.05f, wetness_);
		const float depositionRatio = DepositionRate * (0.3f + 0.7f * dryingFront_) * DeltaTime;
		const float3 pigment_Deposited = pigment_OUT_.SUSPENDED * depositionRatio;
		
		// * Suspended pigment settles.
		
		pigment_OUT_.SUSPENDED -= pigment_Deposited;

		// * Deposited pigment accumulates into `FIXED` channel.
		
		const float pigment_Fixed_IN = pigment_IN_.FIXED;
		const float pigment_Fixed_OUT = saturate(
			pigment_Fixed_IN +
			Grayscale(pigment_Deposited)
		);
		pigment_OUT_.FIXED = pigment_Fixed_OUT;
	}
		
	void EdgeAccumulation(
		inout float4 pigment_OUT_,
		in float dryingFront_,
		in int2 coord_
	) {
		const float2 grad_Curvature = Gradient(GBuffer::SRV::Curvature, TextureSize, coord_);
		const float mag_Curvature = length(grad_Curvature);

	    // * Pigment accumulates at high-curvature sites during drying.
		
		const float edgeAccumulation = EdgeAccumulationStrength * mag_Curvature * dryingFront_ * DeltaTime;
		pigment_OUT_.SUSPENDED += pigment_OUT_.SUSPENDED * edgeAccumulation;
	}
}

[numthreads(8, 8, 1)]
void Simulate(uint3 tid_ : SV_DispatchThreadID) {
	float4 pigment_OUT;
	
	const int2 coord = tid_.xy;
	const float4 pigment_IN = SimulationMap::SRV::PigmentConcentration[coord];
	const float wetness = SimulationMap::SRV::Wetness[coord];
	float dryingFront;
	
	Simulation::Suspension(pigment_OUT, pigment_IN, wetness, coord);
	Simulation::Deposition(pigment_OUT, dryingFront, pigment_IN, wetness);
	Simulation::EdgeAccumulation(pigment_OUT, dryingFront, coord);

	SimulationMap::UAV::PigmentConcentration[coord] = saturate(pigment_OUT);
}



[numthreads(8, 8, 1)]
void Initialize(uint3 tid_ : SV_DispatchThreadID) {
	const float4 albedoWetness = GBuffer::SRV::AlbedoWetness.Load(int3(tid_.xy, 0));
	const float3 albedo = albedoWetness.rgb;
	const float wetness = albedoWetness.a;
	
	// * Makeshift calculation where darker color means higher pigment concentration.
	// * TODO: other approches to represent the concentration
	const float3 pigment = 1.0f - albedo;
	
	// * Pigment suspends in wet areas and fixes in dry areas.
	const float3 pigment_Suspended = pigment * wetness;
	const float pigment_Fixed = Grayscale(pigment * (1.0f - wetness));

	SimulationMap::UAV::PigmentConcentration[tid_.xy] = float4(pigment_Suspended, pigment_Fixed);
}