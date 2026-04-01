export module Game.ConvexColliderDebug;

import <memory>;
import <vector>;

import Lumina.Core.Math;
import Lumina.D3D12;
import Lumina.Primitive;

import Collider;

namespace Game {
	export class ConvexColliderDebugRenderer {
	public:
		auto BatchColliders(
			std::vector<Collider*> const& colliders_
		) -> void;
		auto RenderBatched(
			Lumina::D3D12::DescriptorTable const& srvTable_,
			Lumina::Math::F32x4x4<> const& vp_
		) -> void;

	public:
		auto Initialize() -> void;

	public:
		ConvexColliderDebugRenderer();
		~ConvexColliderDebugRenderer();

	public:
		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager_;
	};
}