#include"Watercolor.hlsli"
#include"MathUtils.hlsli"



cbuffer Parameters : register(b0) {
	float DeltaTime;
	// * ~0.002 to 0.01
	float DecayRate;
	// * ~0.15
	float DiffusionRate;
	float BrushWetness;
	
	int2 TextureSize;
	// * Inverse of texture size
	float2 TexelSize;
};



namespace Simulation {
	void Evaporation(inout float wetness_) {
		const float evaporation = -(DecayRate * DeltaTime * sqrt(max(wetness_, 0.0f)));
		wetness_ += evaporation;
	}
	
	static const int2 CoordOffsets[4] = {
		{ 0, 1, 0 },
		{ 0, -1, 0 },
		{ 1, 0, 0 },
		{ -1, 0, 0 },
	};

	void Diffusion(inout float wetness_, in int2 coord_) {
		const int2 coordOffset = CoordOffsets[WaveGetLaneIndex() & 0x03U];
		const float wetness_Adjacent = SimulationMap::SRV::Wetness.Load(int3(coord_ + coordOffset, 0));
		const float laplacian = Laplacian(SimulationMap::SRV::Wetness, TextureSize, coord_);
		// * `diffusion` is non-negetive so that diffusion only goes from wet region to dry region.
		const float diffusion = DiffusionRate * DeltaTime * max(laplacian, 0.0f);
		wetness_ += diffusion;
	}
	
	void BrushInfluence(inout float wetness_, in int2 coord_) {
		const float brush = BrushMap.SampleLevel(BilinearClamp, float2(coord_) * TexelSize, 0);
		wetness_ = max(wetness_, brush * BrushWetness);
	}
}
	
[numthreads(8, 8, 1)]
void Simulate(uint3 tid_ : SV_DispatchThreadID) {
	const int2 coord = tid_.xy;

	float wetness = SimulationMap::SRV::Wetness.Load(int3(coord, 0));

	Simulation::Evaporation(wetness);
	Simulation::Diffusion(wetness, coord);
	Simulation::BrushInfluence(wetness, coord);

	SimulationMap::UAV::Wetness[coord] = saturate(wetness);
}



[numthreads(8, 8, 1)]
void Initialize(uint3 tid_ : SV_DispatchThreadID) {
	const int2 coord = tid_.xy;
	const float initVal = GBuffer::SRV::AlbedoWetness[coord].w;
	SimulationMap::UAV::Wetness[coord] = initVal;
}