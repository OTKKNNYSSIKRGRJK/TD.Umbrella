module Game.Terrain;

import : Render;
import : Shape;

import Lumina.Main;

namespace Game {
	auto TerrainRenderer::DebugRenderCollidersBatch(
		TerrainShapeCollection const& shapeCollection_
	) -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		PrimitiveManager_->Begin(cmdList);

		auto const& ground{ shapeCollection_.GroundData() };
		for (auto const& collider : ground.Colliders) {
			auto const& verts{ collider->GetVertices() };
			PrimitiveManager_->BatchTriangle(
				{ { verts[0].X, verts[0].Y, verts[0].Z - 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0 },
				{ { verts[1].X, verts[1].Y, verts[1].Z - 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0 },
				{ { verts[3].X, verts[3].Y, verts[3].Z - 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0 }
			);
			PrimitiveManager_->BatchTriangle(
				{ { verts[1].X, verts[1].Y, verts[1].Z - 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0 },
				{ { verts[2].X, verts[2].Y, verts[2].Z - 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0 },
				{ { verts[3].X, verts[3].Y, verts[3].Z - 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0 }
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
	}

	TerrainRenderer::TerrainRenderer() {}
	TerrainRenderer::~TerrainRenderer() {}
}