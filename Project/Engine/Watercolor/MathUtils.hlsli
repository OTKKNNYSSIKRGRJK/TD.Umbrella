namespace MathUtils {
	static const int2 CoordOffsets[4] = {
		{ 1, 0, 0 },
		{ -1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, -1, 0 },
	};
	
	int2 AdjacentCoordinate(in uint2 mapSize_, in int2 coord_) {
		const int2 coordOffset = CoordOffsets[WaveGetLaneIndex() & 0x03U];
		return clamp((coord_ + coordOffset), int2(0, 0), mapSize_);
	}
}

// * Requires 4/8/16/32 etc. lane waves
float2 Gradient(in Texture2D<float> map_, in int2 mapSize_, in uint2 coord_) {
	const int2 coord_Adjacent = MathUtils::AdjacentCoordinate(mapSize_, coord_);
	const float val_Adjacent = map_.Load(int3(coord_Adjacent, 0));
	const float ddx = (WaveReadLaneAt(val_Adjacent, 0U) - WaveReadLaneAt(val_Adjacent, 1U)) * 0.5f;
	const float ddy = (WaveReadLaneAt(val_Adjacent, 2U) - WaveReadLaneAt(val_Adjacent, 3U)) * 0.5f;
	return float2(ddx, ddy);
}

float2 Gradient(in Texture2D<float4> map_, in int2 mapSize_, in uint2 coord_) {
	const int2 coord_Adjacent = MathUtils::AdjacentCoordinate(mapSize_, coord_);
	const float val_Adjacent = map_.Load(int3(coord_Adjacent, 0)).x;
	const float ddx = (WaveReadLaneAt(val_Adjacent, 0U) - WaveReadLaneAt(val_Adjacent, 1U)) * 0.5f;
	const float ddy = (WaveReadLaneAt(val_Adjacent, 2U) - WaveReadLaneAt(val_Adjacent, 3U)) * 0.5f;
	return float2(ddx, ddy);
}

// * Requires 4/8/16/32 etc. lane waves
#define TEMPLATE_LAPLACIAN(_T)\
_T Laplacian(in Texture2D<_T> map_, in int2 mapSize_, in uint2 coord_) {\
	const int2 coord_Adjacent = MathUtils::AdjacentCoordinate(mapSize_, coord_);\
	const _T val_Adjacent = map_.Load(int3(coord_Adjacent, 0));\
	const _T val_Center = map_.Load(int3(coord_, 0));\
	return (\
		(\
			WaveReadLaneAt(val_Adjacent, 0U) +\
			WaveReadLaneAt(val_Adjacent, 1U) +\
			WaveReadLaneAt(val_Adjacent, 2U) +\
			WaveReadLaneAt(val_Adjacent, 3U)\
		) - val_Center * 4.0f\
	);\
}

TEMPLATE_LAPLACIAN(float)
TEMPLATE_LAPLACIAN(float2)
TEMPLATE_LAPLACIAN(float3)
TEMPLATE_LAPLACIAN(float4)