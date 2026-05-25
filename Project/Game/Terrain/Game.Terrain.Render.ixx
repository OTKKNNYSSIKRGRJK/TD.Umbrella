export module Game.Terrain : Render;

import <memory>;

import : Shape;

import Lumina.Core.Math;
import Lumina.D3D12;
import Lumina.Primitive;

namespace Game {
	namespace Impl { class TerrainRenderer; }

	export class TerrainRenderer {
	public:
		auto DebugRenderCollidersBatch(
			TerrainShapeCollection const& shapeCollection_
		) -> void;
		auto DebugRenderColliders(
			Lumina::D3D12::DescriptorTable const& srvTable_,
			Lumina::Math::F32x4x4<> const& vp_
		) -> void;

	public:
		auto Initialize() -> void;

	public:
		TerrainRenderer();
		~TerrainRenderer();

	private:
		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager_;
		std::unique_ptr<Impl::TerrainRenderer> Impl_{ nullptr };
	};
}