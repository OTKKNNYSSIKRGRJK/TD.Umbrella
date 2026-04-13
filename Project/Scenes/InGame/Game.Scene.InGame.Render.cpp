module Game.Scene.InGame;

import : Impl;

import Lumina.Main;
import Lumina.D3D12;
import Lumina.MeshManager;
import Lumina.Primitive;
import Game.MathUtils;
import Game.ProjectileManager;

namespace Game::Scene::Impl {
	void InGame::Render_Geometry() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		auto& meshMngr{ Lumina::Context::Instance().MeshContext() };

		meshMngr.Begin(cmdList);

		D3D12_RESOURCE_BARRIER const barriers_PreGeometryPass[]{
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.RenderTexture(0U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.RenderTexture(1U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.DepthTexture(),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_DEPTH_WRITE
			 ),
		};
		cmdList->ResourceBarrier(3U, barriers_PreGeometryPass);

		cmdList->RSSetViewports(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.Viewports().data()
		);
		cmdList->RSSetScissorRects(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.ScissorRects().data()
		);

		meshMngr.BatchBegin();

		// メッシュバッチはmeshMngr.BatchBegin()とmeshMngr.BatchEnd()の間に書かないといけない
		// メッシュを描画バッチに追加するテンプレート
		/*
		meshMngr.Batch(
			MeshShaderAssets_[メッシュ番号],
			1U,
			LocalHeap_Materials_.CPUHandle(マテリアル番号),
			ワールド行列
		);
		*/

		Player_->Draw();

		for (const auto& e : playState_.Enemies) {
			if (e.IsDead) continue;

			if (EnemyMeshIndices_.contains(e.BaseData.name)) {
				size_t meshIdx = EnemyMeshIndices_.at(e.BaseData.name);
				Lumina::Math::F32x3 scale{ e.Scale, e.Scale, e.Scale };
				Lumina::Math::F32x3 rot{ 0.0f, 0.0f, 0.0f };
				if (!e.FacingRight) {
					rot.Y = 3.14159265f; // 反転
				}
				auto worldMat = Game::MathUtils::SRT(scale, rot, { e.Position.X, e.Position.Y, e.Position.Z });
				
				meshMngr.Batch(
					MeshShaderAssets_[meshIdx],
					1U,
					LocalHeap_Materials_.CPUHandle(0U), // とりあえず共通マテリアル0を使用
					worldMat
				);
			}
		}

		// プロジェクタイルの描画（Actor メッシュで表現）
		const auto& projectiles = Game::ProjectileManager::GetInstance()->GetAll();
		for (const auto& proj : projectiles) {
			if (proj.isDead) continue;

			// Actor のメッシュを使用、なければ CubeMesh にフォールバック
			size_t meshIdx = CubeMeshIdx_;
			if (!proj.data.actorName.empty() && ActorMeshIndices_.contains(proj.data.actorName)) {
				meshIdx = ActorMeshIndices_.at(proj.data.actorName);
			}

			if (meshIdx < MeshShaderAssets_.size()) {
				// Actor Transform のスケールを使用
				float sx = proj.actorData.transform.scaleX;
				float sy = proj.actorData.transform.scaleY;
				float sz = proj.actorData.transform.scaleZ;
				// スケールが未設定（0）の場合はデフォルト
				if (sx <= 0.0f) sx = 0.15f;
				if (sy <= 0.0f) sy = 0.15f;
				if (sz <= 0.0f) sz = 0.15f;

				auto projWorldMat = Game::MathUtils::SRT(
					{ sx, sy, sz },
					{ 0.0f, 0.0f, 0.0f },
					{ proj.position.X, proj.position.Y, proj.position.Z }
				);
				meshMngr.Batch(
					MeshShaderAssets_[meshIdx],
					1U,
					LocalHeap_Materials_.CPUHandle(0U),
					projWorldMat
				);
			}
		}

		// ポータルを薄い立方体（cube.obj）で表現
		// Connectionsの座標はすでにワールド座標に変換されているため、そのまま使用する。
		for (const auto& conn : playState_.CurrentArea.connections) {
			float px = conn.trigger.position.x;
			float py = conn.trigger.position.y;
			float w = conn.trigger.size.x;
			float h = conn.trigger.size.y;
			
			float cx = px + w / 2.0f;
			float cy = py + h / 2.0f;

			float sx = w / 2.0f;
			float sy = h / 2.0f;
			float sz = 0.5f; // "薄く表示する" (ジオメトリとしての厚みを薄くする)

			Lumina::Math::F32x4x4<> worldMat{
				sx,  0.0f, 0.0f, 0.0f,
				0.0f, sy,  0.0f, 0.0f,
				0.0f, 0.0f, sz,  0.0f,
				cx,   cy,   0.0f, 1.0f
			};
			
			if (CubeMeshIdx_ < MeshShaderAssets_.size()) {
				meshMngr.Batch(
					MeshShaderAssets_[CubeMeshIdx_],
					1U,
					LocalHeap_Materials_.CPUHandle(0U),
					worldMat
				);
			}
		}

		meshMngr.BatchEnd();

		GeometryPass_.Begin(cmdList);
		meshMngr.Render(
			GraphicsPSO_MeshDeferredGeometry_,
			GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
			LocalHeap_Scene_.CPUHandle(0U)
		);
		GeometryPass_.End();

		auto rtv{ Canvas_GeometryPass_.RTV(0U) };
		auto dsv{ Canvas_GeometryPass_.DSV() };
		cmdList->OMSetRenderTargets(1U, &rtv, false, &dsv);
		
		TerrainRenderer_->DebugRenderCollidersBatch(*Terrain_);
		TerrainRenderer_->DebugRenderColliders(
			GlobalTable_SRV_CanvasTexture_,
			*WorldToHomogeneous_
		);

		ConvexColliderDebugRenderer_->BatchColliders(CollisionManager_->GetColliders());
		ConvexColliderDebugRenderer_->RenderBatched(
			GlobalTable_SRV_CanvasTexture_,
			*WorldToHomogeneous_
		);

		D3D12_RESOURCE_BARRIER const barriers_PostGeometryPass[]{
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(1U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.DepthTexture(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
		};
		cmdList->ResourceBarrier(3U, barriers_PostGeometryPass);

		meshMngr.End();
	}

	void InGame::Render_Merge() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		cmdList->RSSetViewports(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.Viewports().data()
		);
		cmdList->RSSetScissorRects(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.ScissorRects().data()
		);

		PrimitiveManager_->Begin(cmdList);
		PrimitiveManager_->BatchTriangle(
			{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, 0U },
			{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, 0U },
			{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, 0U }
		);
		PrimitiveManager_->BatchTriangle(
			{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, 0U },
			{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }, 0U },
			{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, 0U }
		);
		PrimitiveManager_->End(cmdList);

		auto const& swapChain{ Lumina::Context::Instance().D3D12Context().SwapChain() };
		MergePass_.RenderTarget(0).View() = swapChain.BackBufferRTVCPUHandle();
		MergePass_.DepthStencil().View() = swapChain.DSVCPUHandle();
		MergePass_.Begin(cmdList);
		PrimitiveManager_->Render(cmdList, GlobalTable_SRV_CanvasTexture_, Lumina::Math::F32x4x4<>::Identity, 1);
		MergePass_.End();
	}

	void InGame::Render() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		ID3D12DescriptorHeap* descriptorHeaps[]{
			Lumina::Context::Instance().D3D12Context().GlobalDescriptorHeap().Get(),
		};
		cmdList->SetDescriptorHeaps(1U, descriptorHeaps);

		UB_WorldToHomogeneous_.Store(*WorldToHomogeneous_, sizeof(Lumina::Math::F32x4x4<>), 0LLU);

		Render_Geometry();
		Render_Merge();
	}
}

namespace Game::Scene {
	void InGame::Render() {
		Impl_->Render();
	}
}