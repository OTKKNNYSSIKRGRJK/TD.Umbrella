#include"Noise.hlsli"

namespace Surface {
	RWTexture2D<uint> Material : register(u0);
	RWTexture2D<float4> BlendAndElevation : register(u1);
	RWTexture2D<float4> Normal : register(u2);
}

cbuffer Common : register(b0) {
	uint2 MapSize;
	float2 TexelSize;
	float INV_MAX;
}
	
cbuffer Material : register(b1) {
	PerlinNoise::Args NoiseArguments_Material;
	uint NUM_Materials;
}

cbuffer Blend : register(b2) {
	PerlinNoise::Args NoiseArguments_Blend;
}

cbuffer Elevation : register(b3) {
	PerlinNoise::Args NoiseArguments_Elevation;
}

namespace Math {
	static const int2 CoordOffsets[4] = {
		{ 1, 0 },
		{ -1, 0 },
		{ 0, 1 },
		{ 0, -1 },
	};
	
	void AdjacentCoordinates(out int2 coords_[4], in int2 coord_) {
		coords_[0] = clamp((coord_ + CoordOffsets[0]), int2(0, 0), MapSize);
		coords_[1] = clamp((coord_ + CoordOffsets[1]), int2(0, 0), MapSize);
		coords_[2] = clamp((coord_ + CoordOffsets[2]), int2(0, 0), MapSize);
		coords_[3] = clamp((coord_ + CoordOffsets[3]), int2(0, 0), MapSize);
	}
	
	float2 BlendGradient(in uint2 coord_) {
		int2 adjacentCoords[4];
		AdjacentCoordinates(adjacentCoords, coord_);
		const float adjacentElevations[4] = {
			Surface::BlendAndElevation.Load(int3(adjacentCoords[0], 0)).b,
			Surface::BlendAndElevation.Load(int3(adjacentCoords[1], 0)).b,
			Surface::BlendAndElevation.Load(int3(adjacentCoords[2], 0)).b,
			Surface::BlendAndElevation.Load(int3(adjacentCoords[3], 0)).b,
		};
		const float ddx = (adjacentElevations[0] - adjacentElevations[1]) * 0.5f;
		const float ddy = (adjacentElevations[2] - adjacentElevations[3]) * 0.5f;
		return float2(ddx, ddy);
	}
	
	float2 ElevationGradient(in uint2 coord_) {
		int2 adjacentCoords[4];
		AdjacentCoordinates(adjacentCoords, coord_);
		const float adjacentElevations[4] = {
			Surface::BlendAndElevation.Load(int3(adjacentCoords[0], 0)).a,
			Surface::BlendAndElevation.Load(int3(adjacentCoords[1], 0)).a,
			Surface::BlendAndElevation.Load(int3(adjacentCoords[2], 0)).a,
			Surface::BlendAndElevation.Load(int3(adjacentCoords[3], 0)).a,
		};
		const float ddx = (adjacentElevations[0] - adjacentElevations[1]) * 0.5f;
		const float ddy = (adjacentElevations[2] - adjacentElevations[3]) * 0.5f;
		return float2(ddx, ddy);
	}
}

void GenerateMaterial(uint2 coord_) {
	const float3 pos = float3(coord_ * TexelSize, 0.0f);
	float noise = PerlinNoise::Generate(pos, NoiseArguments_Material, INV_MAX);
	noise += (noise - 0.5f) * 2.0f;
	
	noise *= float(NUM_Materials);
	noise = clamp(round(noise), 0.0f, float(NUM_Materials - 1U));
	
	Surface::Material[coord_] = uint(noise);
}

void GenerateBlend(uint2 coord_) {
	const float3 pos = float3(coord_ * TexelSize, 0.0f);
	
	Surface::BlendAndElevation[coord_].b = PerlinNoise::Generate(pos, NoiseArguments_Blend, INV_MAX);
}

void GenerateBlendGradient(uint2 coord_) {
	Surface::BlendAndElevation[coord_].rg = Math::BlendGradient(coord_);
}

void GenerateHeight(uint2 coord_) {
	const float3 pos = float3(coord_ * TexelSize, 0.0f);
	const float elevation = PerlinNoise::Generate(pos, NoiseArguments_Elevation, INV_MAX);
	Surface::BlendAndElevation[coord_].a = elevation;
}

void GenerateNormal(uint2 coord_) {	
	const float2 grad = Math::ElevationGradient(coord_);
	const float3 normal = normalize(float3(-grad, 0.0625f));
	
	Surface::Normal[coord_].rgb = normal * 0.5f + 0.5f;
	Surface::Normal[coord_].a = 1.0f;
}

[numthreads(1U, 1U, 1U)]
void main(uint3 dtid_ : SV_DispatchThreadID) {	
	const uint2 coord = dtid_.xy;
	
	GenerateMaterial(coord);
	GenerateBlend(coord);
	GenerateHeight(coord);
}

[numthreads(1U, 1U, 1U)]
void main2(uint3 dtid_ : SV_DispatchThreadID) {
	const uint2 coord = dtid_.xy;
	
	GenerateBlendGradient(coord);
	GenerateNormal(coord);
}