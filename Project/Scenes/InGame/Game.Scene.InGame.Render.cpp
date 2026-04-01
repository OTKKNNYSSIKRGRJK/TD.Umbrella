module Game.Scene.InGame;

import : Impl;

import Lumina.Main;
import Lumina.D3D12;
import Lumina.MeshManager;
import Lumina.Primitive;

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

		auto const worldToHomogeneous_c{ Camera_->View() * Camera_->Projection() };
		auto tmp{ Lumina::Math::F32x4{ 0.0f, 0.0f, 0.0f, 1.0f } * worldToHomogeneous_c };
		tmp /= tmp.W();

		Lumina::F32 const inv_ViewportWidth{ 1.0f / 1280.0f };
		Lumina::F32 const inv_ViewportHeight{ 1.0f / 720.0f };
		
		auto const& inv_View{ Camera_->ViewInverse() };
		auto const inv_Proj{ Camera_->Projection().Inverse() };
		auto const ndcToWorld{ inv_Proj * inv_View };

		for (const auto& e : playState_.Enemies) {
			if (e.IsDead) continue;

			if (EnemyMeshIndices_.contains(e.BaseData.name)) {
				size_t meshIdx = EnemyMeshIndices_.at(e.BaseData.name);
				float dir = e.FacingRight ? 1.0f : -1.0f;
				
				// e.Position.Y は地面から上の高さ（上が正）になっており、
				// TerrainScreenData_ が持っていた生のピクセル座標（下が正）は (AreaHeight - e.Position.Y) です。
				float rawScreenY = playState_.CurrentArea.height - e.Position.Y;

				// 2Dの座標（AreaEditorと同じスクリーン座標）から3Dワールド座標に変換
				Lumina::Math::F32x4 ndcPos{
					(e.Position.X * inv_ViewportWidth) * 2.0f - 1.0f,
					1.0f - (rawScreenY * inv_ViewportHeight) * 2.0f,
					tmp.Z(),
					1.0f
				};
				auto worldPos = ndcPos * ndcToWorld;
				worldPos /= worldPos.W();
				
				Lumina::Math::F32x4x4<> worldMat{
					dir,  0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, dir,  0.0f,
					worldPos.X(), worldPos.Y(), 0.0f, 1.0f
				};
				
				meshMngr.Batch(
					MeshShaderAssets_[meshIdx],
					1U,
					LocalHeap_Materials_.CPUHandle(0U), // とりあえず共通マテリアル0を使用
					worldMat
				);
			}
		}

		// ポータルを薄い立方体（cube.obj）で表現
		// Cubeの大きさが2x2x2と仮定し、判定矩形サイズに合わせて薄くスケールする
		for (const auto& conn : playState_.CurrentArea.connections) {
			float px = conn.trigger.position.x;
			float py = conn.trigger.position.y;
			float w = conn.trigger.size.x;
			float h = conn.trigger.size.y;
			
			float rawCenterY = playState_.CurrentArea.height - (py + h / 2.0f);
			Lumina::Math::F32x4 ndcPos{
				((px + w / 2.0f) * inv_ViewportWidth) * 2.0f - 1.0f,
				1.0f - (rawCenterY * inv_ViewportHeight) * 2.0f,
				tmp.Z(),
				1.0f
			};
			auto worldPos = ndcPos * ndcToWorld;
			worldPos /= worldPos.W();

			auto screenToWorldPos = [&](float sx, float sy) -> Lumina::Math::F32x3 {
				Lumina::Math::F32x4 ndcP{
					(sx * inv_ViewportWidth) * 2.0f - 1.0f,
					1.0f - (sy * inv_ViewportHeight) * 2.0f,
					tmp.Z(),
					1.0f
				};
				auto wPos = ndcP * ndcToWorld;
				wPos /= wPos.W();
				return { wPos.X(), wPos.Y(), 0.0f };
			};
			auto v0 = screenToWorldPos(px, playState_.CurrentArea.height - py);
			auto v2 = screenToWorldPos(px + w, playState_.CurrentArea.height - (py + h));
			
			float sx = std::abs(v2.X - v0.X) / 2.0f;
			float sy = std::abs(v2.Y - v0.Y) / 2.0f;
			float sz = 0.5f; // "薄く表示する" (ジオメトリとしての厚みを薄くする)

			Lumina::Math::F32x4x4<> worldMat{
				sx,  0.0f, 0.0f, 0.0f,
				0.0f, sy,  0.0f, 0.0f,
				0.0f, 0.0f, sz,  0.0f,
				worldPos.X(), worldPos.Y(), 0.0f, 1.0f
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