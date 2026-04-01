module Game.ConvexColliderDebug;

import Lumina.Main;

namespace Game {
	auto ConvexColliderDebugRenderer::BatchColliders(
		std::vector<Collider*> const& colliders_
	) -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		PrimitiveManager_->Begin(cmdList);

		for (auto const& collider : colliders_) {
			auto const& verts{ static_cast<ConvexCollider*>(collider)->GetVertices() };
			auto const& world{ static_cast<ConvexCollider*>(collider)->GetWorldMatrix() };
			if (verts.size() > 2) {
				auto v0{ Lumina::Math::F32x4{ verts[0], 1.0f } * world };
				Lumina::Math::F32x4 v1{ v0 }, v2{};
				for (size_t i = 1; i < verts.size(); ++i) {
					v2 = Lumina::Math::F32x4{ verts[i], 1.0f } * world;
					PrimitiveManager_->BatchLine(
						{ { v1.X(), v1.Y(), v1.Z(), 1.0f}, {1.0f, 0.25f, 0.25f, 0.5f}, {0.0f, 0.0f}, 0 },
						{ { v2.X(), v2.Y(), v2.Z(), 1.0f}, {1.0f, 0.25f, 0.25f, 0.5f}, {0.0f, 0.0f}, 0 }
					);
					v1 = v2;
				}
				v2 = v0;
				PrimitiveManager_->BatchLine(
					{ { v1.X(), v1.Y(), v1.Z(), 1.0f}, {1.0f, 0.25f, 0.25f, 0.5f}, {0.0f, 0.0f}, 0 },
					{ { v2.X(), v2.Y(), v2.Z(), 1.0f}, {1.0f, 0.25f, 0.25f, 0.5f}, {0.0f, 0.0f}, 0 }
				);
			}
		}

		PrimitiveManager_->End(cmdList);
	}

	auto ConvexColliderDebugRenderer::RenderBatched(
		Lumina::D3D12::DescriptorTable const& srvTable_,
		Lumina::Math::F32x4x4<> const& vp_
	) -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		PrimitiveManager_->Render(cmdList, srvTable_, vp_);
	}

	auto ConvexColliderDebugRenderer::Initialize() -> void {
		auto const& d3d12Context{ Lumina::Context::Instance().D3D12Context() };
		PrimitiveManager_ = std::make_unique<Lumina::PrimitiveManager>();
		PrimitiveManager_->Initialize(
			d3d12Context,
			L"Assets/Shaders/Terrain.Debug.VS.hlsl",
			L"Assets/Shaders/Terrain.Debug.PS.hlsl",
			1, 0
		);
	}

	ConvexColliderDebugRenderer::ConvexColliderDebugRenderer() {}
	ConvexColliderDebugRenderer::~ConvexColliderDebugRenderer() {}
}