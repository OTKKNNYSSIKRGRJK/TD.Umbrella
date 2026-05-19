namespace Math {
	static const int2 CoordOffsets[4] = {
		{ 1, 0 },
		{ -1, 0 },
		{ 0, 1 },
		{ 0, -1 },
	};
	
	void AdjacentCoordinates(out int2 coords_[4], in uint2 mapSize_, in int2 coord_) {
		coords_[0] = clamp((coord_ + CoordOffsets[0]), int2(0, 0), mapSize_);
		coords_[1] = clamp((coord_ + CoordOffsets[1]), int2(0, 0), mapSize_);
		coords_[2] = clamp((coord_ + CoordOffsets[2]), int2(0, 0), mapSize_);
		coords_[3] = clamp((coord_ + CoordOffsets[3]), int2(0, 0), mapSize_);
	}
	
	
	
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//..''	''..''	''..''	''..''	''..''	''..''	''..''	''..''	''..//
	/// @class		Math::Gradient
	/// @details
	/// ### Descrition
	/// The number of the waves in a lane is required to be 4/8/16/32 etc.
	//''..	..''..	..''..	..''..	..''..	..''..	..''..	..''..	..''//
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	
	float2 Gradient(in Texture2D<float> map_, in int2 mapSize_, in uint2 coord_) {
		int2 adjacentCoords[4];
		AdjacentCoordinates(adjacentCoords, mapSize_, coord_);
		const float adjacentVals[4] = {
			map_.Load(int3(adjacentCoords[0], 0)),
			map_.Load(int3(adjacentCoords[1], 0)),
			map_.Load(int3(adjacentCoords[2], 0)),
			map_.Load(int3(adjacentCoords[3], 0)),
		};
		const float ddx = (adjacentVals[0] - adjacentVals[1]) * 0.5f;
		const float ddy = (adjacentVals[2] - adjacentVals[3]) * 0.5f;
		return float2(ddx, ddy);
	}

	float2 Gradient(in Texture2D<float4> map_, in int2 mapSize_, in uint2 coord_) {
		int2 adjacentCoords[4];
		AdjacentCoordinates(adjacentCoords, mapSize_, coord_);
		const float adjacentVals[4] = {
			map_.Load(int3(adjacentCoords[0], 0)).x,
			map_.Load(int3(adjacentCoords[1], 0)).x,
			map_.Load(int3(adjacentCoords[2], 0)).x,
			map_.Load(int3(adjacentCoords[3], 0)).x,
		};
		const float ddx = (adjacentVals[0] - adjacentVals[1]) * 0.5f;
		const float ddy = (adjacentVals[2] - adjacentVals[3]) * 0.5f;
		return float2(ddx, ddy);
	}
	
	
	
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//..''	''..''	''..''	''..''	''..''	''..''	''..''	''..''	''..//
	/// @class		Math::Laplacian
	/// @details
	/// ### Descrition
	/// The number of the waves in a lane is required to be 4/8/16/32 etc.
	//''..	..''..	..''..	..''..	..''..	..''..	..''..	..''..	..''//
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	
	#define TEMPLATE_LAPLACIAN(_T)\
	_T Laplacian(in Texture2D<_T> map_, in int2 mapSize_, in uint2 coord_) {\
		int2 adjacentCoords[4];\
		AdjacentCoordinates(adjacentCoords, mapSize_, coord_);\
		const _T adjacentVals[4] = {\
			map_.Load(int3(adjacentCoords[0], 0)),\
			map_.Load(int3(adjacentCoords[1], 0)),\
			map_.Load(int3(adjacentCoords[2], 0)),\
			map_.Load(int3(adjacentCoords[3], 0)),\
		};\
		const _T centerVal = map_.Load(int3(coord_, 0));\
		return (\
			(\
				adjacentVals[0] +\
				adjacentVals[1] +\
				adjacentVals[2] +\
				adjacentVals[3]\
			) - centerVal * 4.0f\
		);\
	}

	TEMPLATE_LAPLACIAN(float)
	TEMPLATE_LAPLACIAN(float2)
	TEMPLATE_LAPLACIAN(float3)
	TEMPLATE_LAPLACIAN(float4)
}

namespace Color {
	float Luminance(in float3 rgb_) {
		return dot(rgb_, float3(0.2125f, 0.7154f, 0.0721f));
	}
}