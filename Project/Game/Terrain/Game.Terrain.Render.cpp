module Game.Terrain;

import : Render;
import : Shape;

import : Render.Impl;

import Lumina.Main;

namespace Game {
	auto TerrainRenderer::DebugRenderCollidersBatch(
		TerrainShapeCollection const& shapeCollection_
	) -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		PrimitiveManager_->Begin(cmdList);

		auto const& polygons{ shapeCollection_.PolygonsData() };
		for (auto const& polygon : polygons) {
			if (polygon.Vertices.size() > 2) {
				auto const* verts{ polygon.Vertices.data() };
				for (size_t i = 2; i < polygon.Vertices.size(); ++i) {
					PrimitiveManager_->BatchTriangle(
						{ { verts[0].Pos.X, verts[0].Pos.Y, verts[0].Pos.Z, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
						{ { verts[i - 1].Pos.X, verts[i - 1].Pos.Y, verts[i - 1].Pos.Z, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
						{ { verts[i].Pos.X, verts[i].Pos.Y, verts[i].Pos.Z, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U }
					);
				}
			}
		}

		auto const& ground{ shapeCollection_.GroundData() };
		for (auto const& collider : ground.Colliders) {
			auto const& verts{ collider->GetVertices() };
			PrimitiveManager_->BatchTriangle(
				{ { verts[0].X, verts[0].Y, verts[0].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
				{ { verts[1].X, verts[1].Y, verts[1].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
				{ { verts[3].X, verts[3].Y, verts[3].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U }
			);
			PrimitiveManager_->BatchTriangle(
				{ { verts[1].X, verts[1].Y, verts[1].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
				{ { verts[2].X, verts[2].Y, verts[2].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
				{ { verts[3].X, verts[3].Y, verts[3].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U }
			);
		}

		PrimitiveManager_->End(cmdList);
	}

	auto TerrainRenderer::DebugRenderColliders(
		Lumina::D3D12::DescriptorTable const& srvTable_,
		Lumina::Math::F32x4x4<> const& vp_
	) -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		PrimitiveManager_->Render(cmdList, srvTable_, vp_);
	}

	auto TerrainRenderer::Initialize() -> void {
		auto const& d3d12Context{ Lumina::Context::Instance().D3D12Context() };
		PrimitiveManager_ = std::make_unique<Lumina::PrimitiveManager>();
		PrimitiveManager_->Initialize(
			d3d12Context,
			L"Assets/Shaders/Terrain.Debug.VS.hlsl",
			L"Assets/Shaders/Terrain.Debug.PS.hlsl"
		);

		Impl_ = std::make_unique<Impl::TerrainRenderer>();
	}

	TerrainRenderer::TerrainRenderer() {}
	TerrainRenderer::~TerrainRenderer() {}
}