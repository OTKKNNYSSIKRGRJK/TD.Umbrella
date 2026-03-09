export module Lumina.Core.Math : Geometry.BoundingBox;

import <memory>;

import : Geometry.Transformation;

namespace Lumina::Math {
	export class AxisAlignedBoundingBox {
	public:
		F32x3 Min;
		F32x3 Max;
	};

	export using AABB = AxisAlignedBoundingBox;

	export class OrientedBoundingBox {
	public:
		inline auto LocalToWorld() const noexcept -> SE3;
		inline auto WorldToLocal() const noexcept -> SE3;

	public:
		Versor Rotation;
		F32x3 Center;
		F32x3 HalfExtents[3];
	};

	export using OBB = OrientedBoundingBox;

	inline auto OBB::LocalToWorld() const noexcept -> SE3 {
		return SE3{ Rotation };
	}

	inline auto OBB::WorldToLocal() const noexcept -> SE3 {
		SE3 se3{ Rotation };
		se3 = se3.Inv();
		return se3;
	}
}