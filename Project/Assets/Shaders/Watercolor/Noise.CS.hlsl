#include"Watercolor.hlsli"
#include"Noise.hlsli"


static const float NoisePersistence = 0.75f;
static const PerlinNoise::Args NoiseArgs = {
	0.5f,
	float3(0.5f, 0.6f, 0.7f),
	NoisePersistence,
	4U,
	1.0f / (
		1.0f +
		NoisePersistence +
		NoisePersistence * NoisePersistence +
		NoisePersistence * NoisePersistence * NoisePersistence
	)
};

[numthreads(1U, 1U, 1U)]
void main(uint3 dtid_ : SV_DispatchThreadID) {
	PerlinNoise::Args noiseArgs = NoiseArgs;
	noiseArgs.Offset.z += Watercolor::Time;
	Watercolor::Output::Noise[dtid_.xy] = PerlinNoise::Generate(dtid_.xyz * 1.0f, noiseArgs);
}