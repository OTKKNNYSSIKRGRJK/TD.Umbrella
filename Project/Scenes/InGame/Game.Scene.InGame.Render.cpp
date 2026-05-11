module Game.Scene.InGame;

import : Impl;

import <cmath>;

import Lumina.Main;
import Lumina.D3D12;
import Lumina.MeshManager;
import Lumina.Primitive;
import Game.MathUtils;
import Game.ProjectileManager;
import Game.TutorialManager;
import Game.UIMenu;

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
				const auto& range = EnemyMeshIndices_.at(e.BaseData.name);
				
				uint32_t materialIdx = 0U;
				if (EnemyMaterialIndices_.contains(e.BaseData.name)) {
					materialIdx = static_cast<uint32_t>(EnemyMaterialIndices_.at(e.BaseData.name));
				}

             Lumina::Math::F32x3 renderPos{ e.Position.X, e.Position.Y, e.Position.Z };
				Lumina::Math::F32x3 scale{ e.Scale, e.Scale, e.Scale };
				if (e.HurtTimer > 0.0f && e.CurrentHP > 0 && e.CurrentHP < e.BaseData.hp) {
					float hurtRatio = e.HurtTimer / 0.2f;
					if (hurtRatio > 1.0f) {
						hurtRatio = 1.0f;
					}

					float const pulse = 0.5f + 0.5f * std::sin(hurtRatio * 18.0f);
					float const stretch = 1.0f + hurtRatio * 0.18f;
					float const squash = 1.0f - hurtRatio * 0.12f;
					float const shakeDir = e.FacingRight ? -1.0f : 1.0f;

					renderPos.X += shakeDir * pulse * 0.18f;
					renderPos.Y += hurtRatio * 0.08f;
					scale.X *= stretch;
					scale.Y *= squash;
					scale.Z *= stretch;
				}

				Lumina::Math::F32x3 rot{ 0.0f, 0.0f, 0.0f };
               rot.Y = e.RenderFacingYaw;
             auto worldMat = Game::MathUtils::SRT(scale, rot, renderPos);
				
				// マルチメッシュ対応: 全サブメッシュを描画
				for (size_t i = 0; i < range.count; ++i) {
					size_t idx = range.startIndex + i;
					if (idx < MeshShaderAssets_.size()) {
						meshMngr.Batch(
							MeshShaderAssets_[idx],
							1U,
							LocalHeap_Materials_.CPUHandle(materialIdx),
							worldMat
						);
					}
				}
			}
		}

		// プロジェクタイルの描画（Actor メッシュで表現）
		const auto& projectiles = Game::ProjectileManager::GetInstance()->GetAll();
		for (const auto& proj : projectiles) {
			if (proj.isDead) continue;

			// Actor のメッシュを使用、なければ CubeMesh にフォールバック
			size_t meshIdx = CubeMeshIdx_;
			size_t meshCount = 1;
			if (!proj.data.actorName.empty() && ActorMeshIndices_.contains(proj.data.actorName)) {
				const auto& range = ActorMeshIndices_.at(proj.data.actorName);
				meshIdx = range.startIndex;
				meshCount = range.count;
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

				float ox = proj.actorData.transform.posX;
				float oy = proj.actorData.transform.posY;
				float oz = proj.actorData.transform.posZ;
				
				float rx = proj.actorData.transform.rotX * 3.14159265f / 180.0f;
				float ry = proj.actorData.transform.rotY * 3.14159265f / 180.0f;
				float rz = proj.actorData.transform.rotZ * 3.14159265f / 180.0f;

				// Mesh local transform (Scale -> Rotate -> Offset)
				auto localMat = Game::MathUtils::SRT(
					{ sx, sy, sz },
					{ rx, ry, rz },
					{ ox, oy, oz }
				);

				// Projectile world position
				auto worldPosMat = Game::MathUtils::Translate(proj.position);

				// Combine: mesh is locally transformed, then moved to projectile's world position
				auto projWorldMat = localMat * worldPosMat;

				// マルチメッシュ対応: 全サブメッシュを描画
				for (size_t i = 0; i < meshCount; ++i) {
					size_t idx = meshIdx + i;
					if (idx < MeshShaderAssets_.size()) {
						meshMngr.Batch(
							MeshShaderAssets_[idx],
							1U,
							LocalHeap_Materials_.CPUHandle(0U),
							projWorldMat
						);
					}
				}
			}
		}

		// ポータルを薄い立方体（cube.obj）で表現
		// Connectionsの座標はすでにワールド座標に変換されているため、そのまま使用する。
		for (const auto& conn : playState_.CurrentArea.connections) {
			float cx = conn.position.x;
			float cy = conn.position.y;

			float sx = 1.5f;
			float sy = 1.5f;
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

			// オーバーレイ描画（プレイヤーHPバー、敵HPバー、チュートリアル等）
		if (PrimitiveManager_Tutorial_) {
			bool drawTutorial = (TutorialManager_ && TutorialManager_->IsActive());
			// プレイ中、またはチュートリアル等があれば描画パスを回す
			if (drawTutorial || !playState_.Enemies.empty() || playState_.IsPlaying) {
				auto const& swapChain{ Lumina::Context::Instance().D3D12Context().SwapChain() };
				auto rtv = swapChain.BackBufferRTVCPUHandle();
				cmdList->OMSetRenderTargets(1U, &rtv, false, nullptr);

				D3D12_VIEWPORT viewport{
					.TopLeftX{ 0.0f }, .TopLeftY{ 0.0f },
					.Width{ 1280.0f }, .Height{ 720.0f },
					.MinDepth{ 0.0f }, .MaxDepth{ 1.0f },
				};
				D3D12_RECT scissor{
					.left{ 0 }, .top{ 0 }, .right{ 1280 }, .bottom{ 720 },
				};
				cmdList->RSSetViewports(1U, &viewport);
				cmdList->RSSetScissorRects(1U, &scissor);

				PrimitiveManager_Tutorial_->Begin(cmdList);

				// ---------------------------------
				// プレイヤーHPバー描画（左上）
				// ---------------------------------
				if (playState_.IsPlaying && Player_) {
					float p_hp = Player_->GetStatusComponent().GetHp();
					float p_maxHp = Player_->GetStatusComponent().GetMaxHp();
					float p_ratio = p_hp / (std::max)(1.0f, p_maxHp);

					// NDCでの左上の座標・サイズ
					float base_x = -0.95f;
					float base_y = 0.9f;
					float width = 0.7f;  // バーの長さ (元: 0.4f)
					float height = 0.08f; // バーの太さ (元: 0.04f)

					// 背景（暗いグレー）
					Lumina::F32x4 bgCol{ 0.1f, 0.1f, 0.1f, 0.8f };
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { base_x, base_y, 0.0f, 1.0f }, bgCol, {0.0f, 0.0f}, 0U },
						{ { base_x + width, base_y, 0.0f, 1.0f }, bgCol, {0.0f, 0.0f}, 0U },
						{ { base_x, base_y - height, 0.0f, 1.0f }, bgCol, {0.0f, 0.0f}, 0U }
					);
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { base_x + width, base_y, 0.0f, 1.0f }, bgCol, {0.0f, 0.0f}, 0U },
						{ { base_x + width, base_y - height, 0.0f, 1.0f }, bgCol, {0.0f, 0.0f}, 0U },
						{ { base_x, base_y - height, 0.0f, 1.0f }, bgCol, {0.0f, 0.0f}, 0U }
					);

					// 前景（プレイヤーHP色：シアン系や緑系）
					Lumina::F32x4 hpCol{ 0.2f, 0.8f, 0.4f, 0.9f }; // デフォルト緑
					if (p_ratio <= 0.3f) hpCol = { 0.9f, 0.2f, 0.2f, 0.9f }; // ピンチで赤

					float current_width = width * p_ratio;
					float padX = 0.005f;
					float padY = 0.008f;
					
					if (current_width > 0.0f) {
						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { base_x + padX, base_y - padY, 0.0f, 1.0f }, hpCol, {0.0f, 0.0f}, 0U },
							{ { base_x + padX + current_width - padX * 2.0f, base_y - padY, 0.0f, 1.0f }, hpCol, {0.0f, 0.0f}, 0U },
							{ { base_x + padX, base_y - height + padY, 0.0f, 1.0f }, hpCol, {0.0f, 0.0f}, 0U }
						);
						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { base_x + padX + current_width - padX * 2.0f, base_y - padY, 0.0f, 1.0f }, hpCol, {0.0f, 0.0f}, 0U },
							{ { base_x + padX + current_width - padX * 2.0f, base_y - height + padY, 0.0f, 1.0f }, hpCol, {0.0f, 0.0f}, 0U },
							{ { base_x + padX, base_y - height + padY, 0.0f, 1.0f }, hpCol, {0.0f, 0.0f}, 0U }
						);
					}
				}

				// ---------------------------------
				// 敵HPバー描画
				// ---------------------------------
				auto const& m = *WorldToHomogeneous_;
				for (const auto& e : playState_.Enemies) {
					if (e.IsDead || e.CurrentHP <= 0 || e.CurrentHP >= e.BaseData.hp) continue; // MAXHP時や死亡時は非表示

					// 敵の頭上の座標
					Lumina::Math::F32x4 pos(e.Position.X, e.Position.Y + e.Scale * 1.5f + 1.0f, e.Position.Z, 1.0f);
					
					// 3D -> 2D (Clip Space)
					Lumina::Math::F32x4 clipPos(
						pos.X() * m[0].X() + pos.Y() * m[1].X() + pos.Z() * m[2].X() + pos.W() * m[3].X(),
						pos.X() * m[0].Y() + pos.Y() * m[1].Y() + pos.Z() * m[2].Y() + pos.W() * m[3].Y(),
						pos.X() * m[0].Z() + pos.Y() * m[1].Z() + pos.Z() * m[2].Z() + pos.W() * m[3].Z(),
						pos.X() * m[0].W() + pos.Y() * m[1].W() + pos.Z() * m[2].W() + pos.W() * m[3].W()
					);

					// カメラ前方にあるかチェック
					if (clipPos.W() > 0.1f) {
						float ndcX = clipPos.X() / clipPos.W();
						float ndcY = clipPos.Y() / clipPos.W();

                       // スケール計算（遠くにあるほど小さく）
                     // 敵HPバーの全体スケール (見た目調整)
						constexpr float EnemyHpBarScale = 3.0f; // 以前は2.0f
						float hw = 0.8f / clipPos.W(); // half width
						if (hw > 0.08f) hw = 0.08f;
						if (hw < 0.02f) hw = 0.02f;
						hw *= EnemyHpBarScale;
                       // 高さは幅に対して比率で決定。より太く見せるため倍率を増加
						float hh = hw * 0.20f; // half height (was 0.15f)

						float ratio = static_cast<float>(e.CurrentHP) / e.BaseData.hp;

						// 背景（黒・半透明）
						Lumina::F32x4 bgColor{ 0.0f, 0.0f, 0.0f, 0.6f };
						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { ndcX - hw, ndcY + hh, 0.0f, 1.0f }, bgColor, {0.0f, 0.0f}, 0U },
							{ { ndcX + hw, ndcY + hh, 0.0f, 1.0f }, bgColor, {0.0f, 0.0f}, 0U },
							{ { ndcX - hw, ndcY - hh, 0.0f, 1.0f }, bgColor, {0.0f, 0.0f}, 0U }
						);
						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { ndcX + hw, ndcY + hh, 0.0f, 1.0f }, bgColor, {0.0f, 0.0f}, 0U },
							{ { ndcX + hw, ndcY - hh, 0.0f, 1.0f }, bgColor, {0.0f, 0.0f}, 0U },
							{ { ndcX - hw, ndcY - hh, 0.0f, 1.0f }, bgColor, {0.0f, 0.0f}, 0U }
						);

						// 前景（HP色）
						Lumina::F32x4 barColor{ 0.2f, 1.0f, 0.2f, 0.9f }; // 緑
						if (ratio < 0.3f) barColor = { 1.0f, 0.2f, 0.2f, 0.9f }; // 赤
						else if (ratio < 0.6f) barColor = { 1.0f, 1.0f, 0.2f, 0.9f }; // 黄

						// パディング考慮
						float pad = hw * 0.05f;
						float p_startX = ndcX - hw + pad;
						float p_endX = p_startX + (hw * 2.0f - pad * 2.0f) * ratio;
						float p_top = ndcY + hh - pad;
						float p_bottom = ndcY - hh + pad;

						if (ratio > 0.0f) {
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ { p_startX, p_top, 0.0f, 1.0f }, barColor, {0.0f, 0.0f}, 0U },
								{ { p_endX,   p_top, 0.0f, 1.0f }, barColor, {0.0f, 0.0f}, 0U },
								{ { p_startX, p_bottom, 0.0f, 1.0f }, barColor, {0.0f, 0.0f}, 0U }
							);
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ { p_endX,   p_top, 0.0f, 1.0f }, barColor, {0.0f, 0.0f}, 0U },
								{ { p_endX,   p_bottom, 0.0f, 1.0f }, barColor, {0.0f, 0.0f}, 0U },
								{ { p_startX, p_bottom, 0.0f, 1.0f }, barColor, {0.0f, 0.0f}, 0U }
							);
						}
					}
				}

				if (drawTutorial) {
					TutorialManager_->RenderOverlay(*PrimitiveManager_Tutorial_);
				}

				// ゲームオーバーUIメニュー描画
				GameOverMenu_.Render(*PrimitiveManager_Tutorial_);

				PrimitiveManager_Tutorial_->Render(
					cmdList,
					GlobalTable_SRV_ImageTexture_,
					Lumina::Math::F32x4x4<>::Identity,
					1
				);
			}
		}
	}
}