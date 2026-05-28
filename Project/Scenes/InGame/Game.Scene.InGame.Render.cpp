module Game.Scene.InGame;

import : Impl;

import <cmath>;
import <algorithm>;
import <string>;

import Lumina.Main;
import Lumina.D3D12;
import Lumina.MeshManager;
import Lumina.Primitive;
import Game.MathUtils;
import Game.ProjectileManager;
import Game.TutorialManager;
import Game.UIMenu;
import Game.Events;

namespace Game::Scene::Impl {
	template<>
	void InGame::Render_<"Skybox">(
		Lumina::D3D12::CommandList const& cmdList_
	) {
		auto rtv{ Canvas_GeometryPass_.RTV(0U) };
		auto dsv{ Canvas_GeometryPass_.DSV() };
		cmdList_->OMSetRenderTargets(1U, &rtv, false, &dsv);
		Skybox_->Render(cmdList_, LocalHeap_Scene_.CPUHandle(0U));
	}

	template<>
	void InGame::Render_<"Portal">(
		Lumina::D3D12::CommandList const& cmdList_,
		Lumina::I32&& idx_,
		Lumina::F32x2&& worldPos_
	) {
		static Lumina::Math::F32x4x4<> world{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};
		world[3] = { worldPos_.X, worldPos_.Y, 0.0f, 1.0f };
		Portals_[idx_]->Render(cmdList_, RS_Portal_, PSO_Portal_, world, *WorldToHomogeneous_);
	}
	template<>
	void InGame::Render_<"Portals">(
		Lumina::D3D12::CommandList const& cmdList_
	) {
		auto rtv{ Canvas_GeometryPass_.RTV(0U) };
		auto dsv{ Canvas_GeometryPass_.DSV() };
		cmdList_->OMSetRenderTargets(1U, &rtv, false, &dsv);

		int idx_Portal{ 0 };
		for (const auto& conn : playState_.CurrentArea.connections) {
			float cx = conn.position.x;
			float cy = conn.position.y;

			Render_<"Portal">(cmdList_, int{ idx_Portal }, Lumina::F32x2{ cx, cy });
			++idx_Portal;
		}
	}

	template<>
	auto InGame::Render_<"PrepareParticle">() -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		PlayerEffects_->Update(
			cmdList,
			Lumina::Math::F32x4x4<>::Identity,
			[this] (Lumina::Particle& p_, void const*) -> bool {
				this->Update_<"PlayerEffectParticle">(p_);
				return (p_.Life > 0.0f);
			}
		);
		UmbrellaEffects_->Update(
			cmdList,
			Lumina::Math::F32x4x4<>::Identity,
			[this] (Lumina::Particle2& p_, void const*) -> bool {
				this->Update_<"UmbrellaEffectParticle">(p_);
				return (p_.Life > 0.0f);
			}
		);
		Raindrops_->Update(
			cmdList,
			Lumina::Math::F32x4x4<>::Identity,
			[this] (Lumina::Particle& p_, void const*) -> bool {
				this->Update_<"RaindropParticle">(p_);
				return (p_.Life > 0.0f);
			}
		);
		AmbientSparkles_->Update(
			cmdList,
			Lumina::Math::F32x4x4<>::Identity,
			[this] (Lumina::Particle& p_, void const*) -> bool {
				this->Update_<"AmbientSparkleParticle">(p_);
				return (p_.Life > 0.0f);
			}
		);
	}

	template<>
	auto InGame::Render_<"Characters">() -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		cmdList->SetGraphicsRootSignature(RS_Skinning_.Get());
		cmdList->SetPipelineState(GraphicsPSO_SkinnedMeshDeferredGeometry_.Get());
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (Player_) {
			auto const& playerModel{ Player_->GetAnimatedModel() };
			cmdList->SetGraphicsRootDescriptorTable(0U, GlobalTable_CBV_Scene_.GPUHandle(0U));
			cmdList->SetGraphicsRootDescriptorTable(1U, playerModel.second.SkinCluster_.PaletteSRVHandle.second);
			cmdList->SetGraphicsRootDescriptorTable(2U, GlobalTable_Materials_.GPUHandle(0U));
			cmdList->SetGraphicsRootDescriptorTable(3U, GlobalTable_SRV_ImageTexture_.GPUHandle(0U));
			cmdList->SetGraphicsRootDescriptorTable(5U, Skybox_->GlobalTable().GPUHandle(0U));
			auto const& cameraPos{ Camera_Player_->WorldPosition() };
			cmdList->SetGraphicsRoot32BitConstants(6U, 3U, &cameraPos, 0U);

			D3D12_VERTEX_BUFFER_VIEW const vbvs[2]{
				reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW const&>(playerModel.first.VBV_),
				reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW const&>(playerModel.second.SkinCluster_.InfluenceBufferView)
			};
			cmdList->IASetVertexBuffers(0, 2, vbvs);
			cmdList->IASetIndexBuffer(reinterpret_cast<D3D12_INDEX_BUFFER_VIEW const*>(&playerModel.first.IBV_));
			cmdList->DrawIndexedInstanced(
				static_cast<Lumina::U32>(playerModel.first.Collection_.Meshes[0].Indices.size()),
				1U, 0U, 0U, 0U
			);
		}

		for (const auto& e : playState_.Enemies) {
			if (e.IsDead) continue;
			
			if (EnemySkinnedModels_.contains(e.BaseData.name) && EnemySkinnedInstances_.contains(e.Id)) {
				auto& model = EnemySkinnedModels_[e.BaseData.name];
				auto& inst = EnemySkinnedInstances_[e.Id];
				
				Lumina::Math::F32x3 renderPos{ e.Position.X, e.Position.Y, e.Position.Z };
				Lumina::Math::F32x3 scale{ e.Scale, e.Scale, e.Scale };
				Lumina::Math::F32x3 rot{ 0.0f, 0.0f, 0.0f };
				
				if (e.SpawnTimer > 0.0f && e.SpawnDuration > 0.0f) {
					float spawnT = 1.0f - (e.SpawnTimer / e.SpawnDuration);
					if (spawnT < 0.0f) spawnT = 0.0f;
					else if (spawnT > 1.0f) spawnT = 1.0f;

					float const riseEase = 1.0f - std::pow(1.0f - spawnT, 4.0f);
					float const overshoot = std::sin(spawnT * 3.14159265f) * (1.0f - spawnT);
					float const shake = std::sin(spawnT * 28.0f + static_cast<float>(e.Id) * 0.31f) * (1.0f - spawnT);
					float const twist = std::sin(spawnT * 15.0f + static_cast<float>(e.Id) * 0.17f) * (1.0f - spawnT);

					renderPos.Y -= (1.0f - riseEase) * (2.8f * e.Scale);
					renderPos.Y += overshoot * (0.95f * e.Scale);
					renderPos.X += shake * (0.16f * e.Scale);

					scale.X *= 0.38f + 0.62f * riseEase + overshoot * 0.18f;
					scale.Y *= 0.06f + 0.94f * riseEase + overshoot * 0.42f;
					scale.Z *= 0.38f + 0.62f * riseEase + overshoot * 0.18f;

					rot.Z += twist * 0.28f;
					rot.X += std::abs(twist) * 0.12f;
				}
				if (e.HurtTimer > 0.0f && e.CurrentHP > 0 && e.CurrentHP < e.BaseData.hp) {
					float hurtRatio = e.HurtTimer / 0.2f;
					if (hurtRatio > 1.0f) hurtRatio = 1.0f;

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

				rot.Y = e.RenderFacingYaw;
				rot.X += e.RenderPitch;
				
				// Optional visual offset applied via behavior (like jump anticipation)
				if (e.VisualOffset.X != 0.0f || e.VisualOffset.Y != 0.0f || e.VisualOffset.Z != 0.0f) {
					renderPos.X += e.VisualOffset.X;
					renderPos.Y += e.VisualOffset.Y;
					renderPos.Z += e.VisualOffset.Z;
				}
				rot.Y += e.VisualYaw;
				
				Lumina::Math::F32x4x4<> meshWorld = Game::MathUtils::SRT(scale, rot, renderPos);
				Lumina::Math::F32x4x4<> tr_INV_MeshWorld = meshWorld.Inverse().Transpose();
				Lumina::Math::F32x4x4<> wvp = meshWorld * (*WorldToHomogeneous_);
				
				inst->TransformsBuffer_.Store(&wvp, sizeof(Lumina::Math::F32x4x4<>), 0LLU);
				inst->TransformsBuffer_.Store(&meshWorld, sizeof(Lumina::Math::F32x4x4<>), sizeof(Lumina::Math::F32x4x4<>));
				inst->TransformsBuffer_.Store(&tr_INV_MeshWorld, sizeof(Lumina::Math::F32x4x4<>), sizeof(Lumina::Math::F32x4x4<>) * 2);

				uint32_t materialIdx = 0U;
				if (EnemyMaterialIndices_.contains(e.BaseData.name)) {
					materialIdx = static_cast<uint32_t>(EnemyMaterialIndices_.at(e.BaseData.name));
				}
				
				cmdList->SetGraphicsRootDescriptorTable(0U, inst->CBV_SceneTable_.GPUHandle(0U));
				cmdList->SetGraphicsRootDescriptorTable(1U, inst->SkinCluster_.PaletteSRVHandle.second);
				cmdList->SetGraphicsRootDescriptorTable(2U, GlobalTable_Materials_.GPUHandle(materialIdx));
				cmdList->SetGraphicsRootDescriptorTable(3U, GlobalTable_SRV_ImageTexture_.GPUHandle(0U));

				D3D12_VERTEX_BUFFER_VIEW const vbvs[2]{
					reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW const&>(model->VBV_),
					reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW const&>(inst->SkinCluster_.InfluenceBufferView)
				};
				cmdList->IASetVertexBuffers(0, 2, vbvs);
				cmdList->IASetIndexBuffer(reinterpret_cast<D3D12_INDEX_BUFFER_VIEW const*>(&model->IBV_));
				cmdList->DrawIndexedInstanced(
					static_cast<Lumina::U32>(model->Collection_.Meshes[0].Indices.size()),
					1U, 0U, 0U, 0U
				);
			}
		}
	}

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
				 Canvas_GeometryPass_.RenderTexture(2U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.DepthTexture(),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_DEPTH_WRITE
			 ),
		};
		cmdList->ResourceBarrier(4U, barriers_PreGeometryPass);

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
               Lumina::Math::F32x3 rot{ 0.0f, 0.0f, 0.0f };
             if (e.SpawnTimer > 0.0f && e.SpawnDuration > 0.0f) {
					float spawnT = 1.0f - (e.SpawnTimer / e.SpawnDuration);
					if (spawnT < 0.0f) {
						spawnT = 0.0f;
					} else if (spawnT > 1.0f) {
						spawnT = 1.0f;
					}

                   float const riseEase = 1.0f - std::pow(1.0f - spawnT, 4.0f);
					float const overshoot = std::sin(spawnT * 3.14159265f) * (1.0f - spawnT);
					float const shake = std::sin(spawnT * 28.0f + static_cast<float>(e.Id) * 0.31f) * (1.0f - spawnT);
					float const twist = std::sin(spawnT * 15.0f + static_cast<float>(e.Id) * 0.17f) * (1.0f - spawnT);

					renderPos.Y -= (1.0f - riseEase) * (2.8f * e.Scale);
					renderPos.Y += overshoot * (0.95f * e.Scale);
					renderPos.X += shake * (0.16f * e.Scale);

					scale.X *= 0.38f + 0.62f * riseEase + overshoot * 0.18f;
					scale.Y *= 0.06f + 0.94f * riseEase + overshoot * 0.42f;
					scale.Z *= 0.38f + 0.62f * riseEase + overshoot * 0.18f;

					rot.Z += twist * 0.28f;
					rot.X += std::abs(twist) * 0.12f;
				}
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

               rot.Y = e.RenderFacingYaw;
			   rot.X += e.RenderPitch; // ピッチを適用
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

				// removed hardcoded boss sword rendering to allow data-driven equipment
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

		meshMngr.BatchEnd();

		GeometryPass_.Begin(cmdList);
		meshMngr.Render(
			GraphicsPSO_MeshDeferredGeometry_,
			GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
			LocalHeap_Scene_.CPUHandle(0U)
		);
		Render_<"Characters">();
		GeometryPass_.End();

		Render_<"Skybox">(cmdList);

		{
			auto rtv{ Canvas_GeometryPass_.RTV(0U) };
			auto dsv{ Canvas_GeometryPass_.DSV() };
			cmdList->OMSetRenderTargets(1U, &rtv, false, &dsv);

			EnemyEffects_->Render(
				cmdList,
				RS_ParticleSystem_,
				GraphicsPSO_BasicParticle_AdditiveMode_,
				LocalHeap_Scene_.CPUHandle(0U),
				LocalHeap_Scene_.CPUHandle(0U),
				GlobalTable_SRV_ImageTexture_,
				GlobalTable_SRV_ImageTexture_
			);

			PlayerEffects_->Render(
				cmdList,
				RS_ParticleSystem_,
				GraphicsPSO_BasicParticle_AdditiveMode_,
				LocalHeap_Scene_.CPUHandle(0U),
				LocalHeap_Scene_.CPUHandle(0U),
				GlobalTable_SRV_ImageTexture_,
				GlobalTable_SRV_ImageTexture_
			);
			UmbrellaEffects_->Render(
				cmdList,
				RS_ParticleSystem_,
				GraphicsPSO_BasicParticle_AdditiveMode_,
				LocalHeap_Scene_.CPUHandle(0U),
				LocalHeap_Scene_.CPUHandle(0U),
				GlobalTable_SRV_ImageTexture_,
				GlobalTable_SRV_ImageTexture_
			);
			Raindrops_->Render(
				cmdList,
				RS_ParticleSystem_,
				GraphicsPSO_BasicParticle_AdditiveMode_,
				LocalHeap_Scene_.CPUHandle(0U),
				LocalHeap_Scene_.CPUHandle(0U),
				GlobalTable_SRV_ImageTexture_,
				GlobalTable_SRV_ImageTexture_
			);
			AmbientSparkles_->Render(
				cmdList,
				RS_ParticleSystem_,
				GraphicsPSO_BasicParticle_AdditiveMode_,
				LocalHeap_Scene_.CPUHandle(0U),
				LocalHeap_Scene_.CPUHandle(0U),
				GlobalTable_SRV_ImageTexture_,
				GlobalTable_SRV_ImageTexture_
			);
		}

		TerrainRenderer_->Render(
			static_cast<Lumina::D3D12::Canvas const&>(Canvas_GeometryPass_),
			Camera_Player_->WorldPosition(),
			static_cast<Lumina::Math::F32x4x4<> const&>(*WorldToHomogeneous_)
		);

		Render_<"Portals">(cmdList);

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
				 Canvas_GeometryPass_.RenderTexture(2U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			 ),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.DepthTexture(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
		};
		cmdList->ResourceBarrier(4U, barriers_PostGeometryPass);

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

		//auto rtv = swapChain.BackBufferRTVCPUHandle();
		//auto dsv = swapChain.DSVCPUHandle();
		//cmdList->OMSetRenderTargets(1U, &rtv, false, nullptr);
	}

	void InGame::Render() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		ID3D12DescriptorHeap* descriptorHeaps[]{
			Lumina::Context::Instance().D3D12Context().GlobalDescriptorHeap().Get(),
		};
		cmdList->SetDescriptorHeaps(1U, descriptorHeaps);

		UB_WorldToHomogeneous_.Store(*WorldToHomogeneous_, sizeof(Lumina::Math::F32x4x4<>), 0LLU);

		auto const& playerModel{ Player_->GetAnimatedModel() };
		Lumina::Math::F32x4x4<> meshWorld{ Game::MathUtils::SRT(playerModel.second.MeshScale_, playerModel.second.MeshRotate_, playerModel.second.MeshTranslate_) };
		Lumina::Math::F32x4x4<> tr_INV_MeshWorld{ meshWorld.Inverse().Transpose() };
		Lumina::Math::F32x4x4<> wvp{ meshWorld * (*WorldToHomogeneous_) };
		UB_Transforms_.Store(&wvp, sizeof(Lumina::Math::F32x4x4<>), 0LLU);
		UB_Transforms_.Store(&meshWorld, sizeof(Lumina::Math::F32x4x4<>), sizeof(Lumina::Math::F32x4x4<>));
		UB_Transforms_.Store(&tr_INV_MeshWorld, sizeof(Lumina::Math::F32x4x4<>), sizeof(Lumina::Math::F32x4x4<>) * 2);

		Render_<"PrepareParticle">();
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

				// 経験値オーブの描画
				Game::ExpOrbManager::GetInstance()->Draw(PrimitiveManager_Tutorial_.get(), *WorldToHomogeneous_);

				// ---------------------------------
				// プレイヤーHPバー描画（左上）
				// ---------------------------------
				if (playState_.IsPlaying && Player_) {
					float p_hp = Player_->GetStatusComponent().GetHp();
					float p_maxHp = Player_->GetStatusComponent().GetMaxHp();
					float p_ratio = p_hp / (std::max)(1.0f, p_maxHp);

					// 経験値進捗の取得
					float currentXp = static_cast<float>(Player_->GetExperienceComponent().GetCurrentXp());
					float nextLevelXp = static_cast<float>(Player_->GetExperienceComponent().GetNextLevelXp());
					float xp_ratio = currentXp / (std::max)(1.0f, nextLevelXp);
					xp_ratio = (std::max)(0.0f, (std::min)(1.0f, xp_ratio));

					// 頭部の座標・サイズ (アスペクト比 1280.0f / 720.0f を考慮して正方形にする)
					float head_w = 0.12f;
					float head_h = head_w * (1280.0f / 720.0f);
					float head_x = -0.95f;
					float head_y = 0.94f;

					// 1. 暗い頭部背景（暗転：明度0.2f, アルファ0.8f）
					Lumina::F32x4 darkHeadCol{ 0.2f, 0.2f, 0.2f, 0.8f };
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { head_x, head_y, 0.0f, 1.0f }, darkHeadCol, {0.0f, 0.0f}, 13U },
						{ { head_x + head_w, head_y, 0.0f, 1.0f }, darkHeadCol, {1.0f, 0.0f}, 13U },
						{ { head_x, head_y - head_h, 0.0f, 1.0f }, darkHeadCol, {0.0f, 1.0f}, 13U }
					);
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { head_x + head_w, head_y, 0.0f, 1.0f }, darkHeadCol, {1.0f, 0.0f}, 13U },
						{ { head_x + head_w, head_y - head_h, 0.0f, 1.0f }, darkHeadCol, {1.0f, 1.0f}, 13U },
						{ { head_x, head_y - head_h, 0.0f, 1.0f }, darkHeadCol, {0.0f, 1.0f}, 13U }
					);

					// 2. 経験値の量に応じて下から徐々に明るく（明度1.0f, アルファ1.0f）
					if (xp_ratio > 0.0f) {
						Lumina::F32x4 brightHeadCol{ 1.0f, 1.0f, 1.0f, 1.0f };
						float active_top_y = (head_y - head_h) + head_h * xp_ratio;
						float active_top_v = 1.0f - xp_ratio;

						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { head_x, active_top_y, 0.0f, 1.0f }, brightHeadCol, {0.0f, active_top_v}, 13U },
							{ { head_x + head_w, active_top_y, 0.0f, 1.0f }, brightHeadCol, {1.0f, active_top_v}, 13U },
							{ { head_x, head_y - head_h, 0.0f, 1.0f }, brightHeadCol, {0.0f, 1.0f}, 13U }
						);
						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { head_x + head_w, active_top_y, 0.0f, 1.0f }, brightHeadCol, {1.0f, active_top_v}, 13U },
							{ { head_x + head_w, head_y - head_h, 0.0f, 1.0f }, brightHeadCol, {1.0f, 1.0f}, 13U },
							{ { head_x, head_y - head_h, 0.0f, 1.0f }, brightHeadCol, {0.0f, 1.0f}, 13U }
						);
					}

					// HPバーは頭部の右隣に配置するため右へシフト
					float base_x = head_x + head_w + 0.02f;
					float height = 0.08f;
					// 頭部画像の下端 (head_y - head_h) とHPバーの下端を揃える
					float base_y = (head_y - head_h) + height; 
					float width = 0.92f;  // 頭部スペースの分、バーの長さを調整

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


				// --- Ender Lilies Style Minimap ---
				if (playState_.IsPlaying && !areaEditor_.GetAllAreas().empty()) {
					const auto& allAreas = areaEditor_.GetAllAreas();

					struct GridPos {
						int x, y;
						bool operator<(const GridPos& other) const {
							if (x != other.x) return x < other.x;
							return y < other.y;
						}
					};

					std::map<int, std::vector<int>> adjList;
					for (const auto& a : allAreas) {
						for (const auto& conn : a.connections) {
							int targetIdx = conn.targetAreaIndex;
							if (a.index == 0 && targetIdx == 0) continue;
							adjList[a.index].push_back(targetIdx);
							adjList[targetIdx].push_back(a.index);
						}
					}
					for (auto& pair : adjList) {
						auto& vec = pair.second;
						std::sort(vec.begin(), vec.end());
						vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
					}

					std::vector<int> startNodes;
					auto hasNode = [&](int idx) {
						for (const auto& a : allAreas) if (a.index == idx) return true;
						return false;
					};
					if (hasNode(0)) startNodes.push_back(0);
					for (const auto& a : allAreas) {
						if (std::find(startNodes.begin(), startNodes.end(), a.index) == startNodes.end()) {
							startNodes.push_back(a.index);
						}
					}

					std::map<int, GridPos> gridLayout;
					std::map<GridPos, int> gridOccupancy;

					for (int startIdx : startNodes) {
						if (gridLayout.count(startIdx) > 0) continue;
						std::vector<int> queue = { startIdx };
						
						int startX = 0, startY = 0;
						while (gridOccupancy.count({startX, startY}) > 0) startY++;
						gridLayout[startIdx] = {startX, startY};
						gridOccupancy[{startX, startY}] = startIdx;

						size_t head = 0;
						while (head < queue.size()) {
							int curr = queue[head++];
							GridPos cPos = gridLayout[curr];

							const Game::Editor::AreaData* currData = nullptr;
							for (const auto& a : allAreas) {
								if (a.index == curr) {
									currData = &a;
									break;
								}
							}

							for (int neighbor : adjList[curr]) {
								if (gridLayout.count(neighbor) == 0) {
									int dirX = 1;
									int dirY = 0;
									if (currData) {
										for (const auto& conn : currData->connections) {
											if (conn.targetAreaIndex == neighbor) {
												float cx = currData->width / 2.0f;
												float cy = currData->height / 2.0f;
												float rx = (conn.position.x - cx) / currData->width;
												float ry = (conn.position.y - cy) / currData->height;
												if (std::abs(rx) > std::abs(ry)) {
													dirX = (rx > 0) ? 1 : -1;
													dirY = 0;
												} else {
													dirX = 0;
													dirY = (ry > 0) ? 1 : -1;
												}
												break;
											}
										}
									}

									int nx = cPos.x + dirX;
									int ny = cPos.y + dirY;
									int step = 1;
									while (gridOccupancy.count({nx, ny}) > 0) {
										if (dirX != 0) {
											ny = cPos.y + dirY + ((step % 2 == 0) ? step / 2 : -(step + 1) / 2);
										} else {
											nx = cPos.x + dirX + ((step % 2 == 0) ? step / 2 : -(step + 1) / 2);
										}
										step++;
									}
									gridLayout[neighbor] = {nx, ny};
									gridOccupancy[{nx, ny}] = neighbor;
									queue.push_back(neighbor);
								}
							}
						}
					}

					struct AreaNode {
						int index;
						float cx, cy;
						float w, h; // for rectangles
					};
					std::vector<AreaNode> nodes;
					float minCX = 1e9f, minCY = 1e9f, maxCX = -1e9f, maxCY = -1e9f;
					
					// Spacing needs to be larger than max width/height to avoid overlaps
					constexpr float gridSpacingX = 2.2f;
					constexpr float gridSpacingY = 1.8f;

					for (const auto& a : allAreas) {
						GridPos gp = gridLayout[a.index];
						float cx = gp.x * gridSpacingX;
						float cy = gp.y * gridSpacingY;
						float aw = (std::max)(1.0f, a.width / 1280.0f);
						float ah = (std::max)(1.0f, a.height / 720.0f);

						if (aw > 2.0f) aw = 2.0f;
						if (ah > 1.5f) ah = 1.5f;

						// Calculate bounds for ALL areas to keep scale fixed
						minCX = (std::min)(minCX, cx - aw * 0.5f);
						minCY = (std::min)(minCY, cy - ah * 0.5f);
						maxCX = (std::max)(maxCX, cx + aw * 0.5f);
						maxCY = (std::max)(maxCY, cy + ah * 0.5f);

						// Only draw visited areas
						if (playState_.VisitedAreas.count(a.index) == 0) continue;

						nodes.push_back({ a.index, cx, cy, aw, ah });
					}

					if (!nodes.empty()) {
						float rangeX = maxCX - minCX;
						float rangeY = maxCY - minCY;
						if (rangeX < 1e-3f) rangeX = 1.0f;
						if (rangeY < 1e-3f) rangeY = 1.0f;


						float mapLeft, mapRight, mapBottom, mapTop;
						if (minimapExpanded_) {
							mapLeft = -0.85f; mapRight = 0.85f;
							mapBottom = -0.85f; mapTop = 0.85f;
						} else {
							mapLeft = 0.4f; mapRight = 0.95f;
							mapBottom = -0.95f; mapTop = -0.3f;
						}
						
						float mapW = mapRight - mapLeft;
						float mapH = mapTop - mapBottom;


						rangeX *= 1.2f;
						rangeY *= 1.2f;

						float scaleX = mapW / rangeX;
						float scaleY = mapH / rangeY;
						float scale = (std::min)(scaleX, scaleY);

						float scaledW = rangeX * scale;
						float scaledH = rangeY * scale;
						float offsetX = mapLeft + (mapW - scaledW) * 0.5f;
						float offsetY = mapTop - (mapH - scaledH) * 0.5f;

						auto ToScreen = [&](float ex, float ey) -> std::pair<float, float> {
							return {
								offsetX + (ex - minCX) * scale,
								offsetY - (ey - minCY) * scale
							};
						};

						std::map<int, std::pair<float, float>> nodeScreenPos;
						std::map<int, AreaNode> nodeData;
						for (const auto& n : nodes) {
							nodeScreenPos[n.index] = ToScreen(n.cx, n.cy);
							nodeData[n.index] = n;
						}

						auto findArea = [&](int id) -> const Game::Editor::AreaData* {
							for(const auto& ar : allAreas) if(ar.index == id) return &ar;
							return nullptr;
						};


						std::vector<std::pair<int,int>> drawnEdges;
						auto edgeDrawn = [&](int a, int b) -> bool {
							for (const auto& e : drawnEdges) {
								if ((e.first == a && e.second == b) || (e.first == b && e.second == a)) return true;
							}
							return false;
						};

						if (minimapExpanded_) {
							Lumina::F32x4 darkenCol{ 0.0f, 0.0f, 0.0f, 0.8f };
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ { -1.0f,  1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 9U },
								{ {  1.0f,  1.0f, 0.0f, 1.0f }, darkenCol, {1.0f, 0.0f}, 9U },
								{ { -1.0f, -1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 1.0f}, 9U }
							);
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ {  1.0f,  1.0f, 0.0f, 1.0f }, darkenCol, {1.0f, 0.0f}, 9U },
								{ {  1.0f, -1.0f, 0.0f, 1.0f }, darkenCol, {1.0f, 1.0f}, 9U },
								{ { -1.0f, -1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 1.0f}, 9U }
							);
						}

						Lumina::F32x4 lineCol{ 1.0f, 1.0f, 1.0f, 1.0f };
						float lineThickness = 0.005f;

						for (const auto& a : allAreas) {

							if (playState_.VisitedAreas.count(a.index) == 0) continue;

							auto itFrom = nodeData.find(a.index);
							if (itFrom == nodeData.end()) continue;

							for (const auto& conn : a.connections) {
								int targetIdx = conn.targetAreaIndex;
								if (a.index == 0 && targetIdx == 0) continue;

								if (playState_.VisitedAreas.count(targetIdx) == 0) continue;

								auto itTo = nodeData.find(targetIdx);
								if (itTo == nodeData.end()) continue;

								if (edgeDrawn(a.index, targetIdx)) continue;
								drawnEdges.push_back({ a.index, targetIdx });


								float localX1 = (conn.position.x / (std::max)(1.0f, static_cast<float>(a.width))) - 0.5f;
								float localY1 = (conn.position.y / (std::max)(1.0f, static_cast<float>(a.height))) - 0.5f;
								float gateA_cx = itFrom->second.cx + localX1 * itFrom->second.w;
								float gateA_cy = itFrom->second.cy + localY1 * itFrom->second.h;


								const Game::Editor::AreaData* bData = findArea(targetIdx);
								float gateB_cx = itTo->second.cx;
								float gateB_cy = itTo->second.cy;
								
								if (bData) {
									for (const auto& bConn : bData->connections) {
										if (bConn.targetAreaIndex == a.index) {
											float localX2 = (bConn.position.x / (std::max)(1.0f, static_cast<float>(bData->width))) - 0.5f;
											float localY2 = (bConn.position.y / (std::max)(1.0f, static_cast<float>(bData->height))) - 0.5f;
											gateB_cx = itTo->second.cx + localX2 * itTo->second.w;
											gateB_cy = itTo->second.cy + localY2 * itTo->second.h;
											break;
										}
									}
								}

								auto screenA = ToScreen(gateA_cx, gateA_cy);
								auto screenB = ToScreen(gateB_cx, gateB_cy);

								float x1 = screenA.first;
								float y1 = screenA.second;
								float x2 = screenB.first;
								float y2 = screenB.second;


								float xA = nodeScreenPos[a.index].first;
								float yA = nodeScreenPos[a.index].second;
								float hwA = itFrom->second.w * scale * 0.5f;
								float hhA = itFrom->second.h * scale * 0.5f;
								float lA = xA - hwA, rA = xA + hwA, tA = yA + hhA, bA = yA - hhA;


								float xB = nodeScreenPos[targetIdx].first;
								float yB = nodeScreenPos[targetIdx].second;
								float hwB = itTo->second.w * scale * 0.5f;
								float hhB = itTo->second.h * scale * 0.5f;
								float lB = xB - hwB, rB = xB + hwB, tB = yB + hhB, bB = yB - hhB;

								float deltaX = std::abs(xB - xA);
								float deltaY = std::abs(yB - yA);

								float pxA = xA, pyA = yA;
								float pxB = xB, pyB = yB;
								
								// Clamp exit points strictly to the faces based on routing
								if (deltaX > deltaY) {
									pxA = (xB > xA) ? rA : lA;
									pxB = (xA > xB) ? rB : lB;
									pyA = y1;
									pyB = y2;
								} else {
									pxA = x1;
									pxB = x2;
									pyA = (yB > yA) ? tA : bA;
									pyB = (yA > yB) ? tB : bB;
								}

								auto drawSegment = [&](float sx, float sy, float ex, float ey, bool extS, bool extE) {
									float dx = ex - sx;
									float dy = ey - sy;
									float len = std::sqrt(dx*dx + dy*dy);
									if (len > 1e-4f) {
										float tx = (dx / len) * lineThickness;
										float ty = (dy / len) * lineThickness;
										if (extS) { sx -= tx; sy -= ty; }
										if (extE) { ex += tx; ey += ty; }
										
										float nx = -dy / len * lineThickness;
										float ny = dx / len * lineThickness;
										PrimitiveManager_Tutorial_->BatchTriangle(
											{ { sx + nx, sy + ny, 0.0f, 1.0f }, lineCol, {0.5f, 0.5f}, 9U },
											{ { ex + nx, ey + ny, 0.0f, 1.0f }, lineCol, {0.5f, 0.5f}, 9U },
											{ { sx - nx, sy - ny, 0.0f, 1.0f }, lineCol, {0.5f, 0.5f}, 9U }
										);
										PrimitiveManager_Tutorial_->BatchTriangle(
											{ { ex + nx, ey + ny, 0.0f, 1.0f }, lineCol, {0.5f, 0.5f}, 9U },
											{ { ex - nx, ey - ny, 0.0f, 1.0f }, lineCol, {0.5f, 0.5f}, 9U },
											{ { sx - nx, sy - ny, 0.0f, 1.0f }, lineCol, {0.5f, 0.5f}, 9U }
										);
									}
								};

								if (deltaX > deltaY) {
									if (deltaY < 1e-3f) {
										float avgY = (pyA + pyB) * 0.5f;
										pyA = avgY;
										pyB = avgY;
									}
									float midX = (pxA + pxB) * 0.5f;
									drawSegment(pxA, pyA, midX, pyA, false, true);
									drawSegment(midX, pyA, midX, pyB, true, true);
									drawSegment(midX, pyB, pxB, pyB, true, false);
								} else {
									if (deltaX < 1e-3f) {
										float avgX = (pxA + pxB) * 0.5f;
										pxA = avgX;
										pxB = avgX;
									}
									float midY = (pyA + pyB) * 0.5f;
									drawSegment(pxA, pyA, pxA, midY, false, true);
									drawSegment(pxA, midY, pxB, midY, true, true);
									drawSegment(pxB, midY, pxB, pyB, true, false);
								}
							}
						}


						for (const auto& n : nodes) {
							bool isCurrent = (n.index == playState_.CurrentArea.index);
							float x = nodeScreenPos[n.index].first;
							float y = nodeScreenPos[n.index].second;

							float hw = (n.w * scale) * 0.5f;
							float hh = (n.h * scale) * 0.5f;
							
							float left = x - hw;
							float right = x + hw;
							float top = y + hh;
							float bottom = y - hh;


							Lumina::F32x4 fillCol = isCurrent ? Lumina::F32x4{0.2f, 0.4f, 1.0f, 0.9f} : Lumina::F32x4{0.0f, 0.0f, 0.0f, 0.9f};
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ { left,  top, 0.0f, 1.0f }, fillCol, {0.5f, 0.5f}, 9U },
								{ { right, top, 0.0f, 1.0f }, fillCol, {0.5f, 0.5f}, 9U },
								{ { left,  bottom, 0.0f, 1.0f }, fillCol, {0.5f, 0.5f}, 9U }
							);
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ { right, top, 0.0f, 1.0f }, fillCol, {0.5f, 0.5f}, 9U },
								{ { right, bottom, 0.0f, 1.0f }, fillCol, {0.5f, 0.5f}, 9U },
								{ { left,  bottom, 0.0f, 1.0f }, fillCol, {0.5f, 0.5f}, 9U }
							);


							Lumina::F32x4 borderCol{1.0f, 1.0f, 1.0f, 1.0f};
							float bt = 0.003f;
							
							auto addRect = [&](float l, float r, float t, float b) {
								PrimitiveManager_Tutorial_->BatchTriangle(
									{ { l, t, 0.0f, 1.0f }, borderCol, {0.5f, 0.5f}, 9U },
									{ { r, t, 0.0f, 1.0f }, borderCol, {0.5f, 0.5f}, 9U },
									{ { l, b, 0.0f, 1.0f }, borderCol, {0.5f, 0.5f}, 9U }
								);
								PrimitiveManager_Tutorial_->BatchTriangle(
									{ { r, t, 0.0f, 1.0f }, borderCol, {0.5f, 0.5f}, 9U },
									{ { r, b, 0.0f, 1.0f }, borderCol, {0.5f, 0.5f}, 9U },
									{ { l, b, 0.0f, 1.0f }, borderCol, {0.5f, 0.5f}, 9U }
								);
							};
							
							addRect(left, right, top + bt, top);
							addRect(left, right, bottom, bottom - bt);
							addRect(left - bt, left, top, bottom);
							addRect(right, right + bt, top, bottom);

							const Game::Editor::AreaData* aData = findArea(n.index);
							if (aData && aData->hasGoal) {
								Lumina::F32x4 goalCol{ 0.1f, 0.8f, 0.1f, 1.0f };
								float gw = 0.012f, gh = 0.012f * 1280.0f / 720.0f;
								PrimitiveManager_Tutorial_->BatchTriangle(
									{ { x, y + gh, 0.0f, 1.0f }, goalCol, {0.5f, 0.5f}, 9U },
									{ { x + gw, y, 0.0f, 1.0f }, goalCol, {0.5f, 0.5f}, 9U },
									{ { x - gw, y, 0.0f, 1.0f }, goalCol, {0.5f, 0.5f}, 9U }
								);
								PrimitiveManager_Tutorial_->BatchTriangle(
									{ { x + gw, y, 0.0f, 1.0f }, goalCol, {0.5f, 0.5f}, 9U },
									{ { x, y - gh, 0.0f, 1.0f }, goalCol, {0.5f, 0.5f}, 9U },
									{ { x - gw, y, 0.0f, 1.0f }, goalCol, {0.5f, 0.5f}, 9U }
								);
							}
						}

						// --- Minimap UI Labels ---
						float texW = 0.375f;
						float texH = 0.222f;
						Lumina::F32x4 uiCol{1.0f, 1.0f, 1.0f, 1.0f};
						
						if (minimapExpanded_) {
							uint32_t texID = 11U; // minimap_close_ui
							float cx = 0.0f;
							float bottom = -0.9f;
							float left = cx - texW * 0.5f;
							float right = cx + texW * 0.5f;
							float top = bottom + texH;
							
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ { left,  top, 0.0f, 1.0f }, uiCol, {0.0f, 0.0f}, texID },
								{ { right, top, 0.0f, 1.0f }, uiCol, {1.0f, 0.0f}, texID },
								{ { left,  bottom, 0.0f, 1.0f }, uiCol, {0.0f, 1.0f}, texID }
							);
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ { right, top, 0.0f, 1.0f }, uiCol, {1.0f, 0.0f}, texID },
								{ { right, bottom, 0.0f, 1.0f }, uiCol, {1.0f, 1.0f}, texID },
								{ { left,  bottom, 0.0f, 1.0f }, uiCol, {0.0f, 1.0f}, texID }
							);
						} else {
							uint32_t texID = 10U; // minimap_ui
							float right = 0.95f;
							float bottom = -0.95f;
							float left = right - texW;
							float top = bottom + texH;
							
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ { left,  top, 0.0f, 1.0f }, uiCol, {0.0f, 0.0f}, texID },
								{ { right, top, 0.0f, 1.0f }, uiCol, {1.0f, 0.0f}, texID },
								{ { left,  bottom, 0.0f, 1.0f }, uiCol, {0.0f, 1.0f}, texID }
							);
							PrimitiveManager_Tutorial_->BatchTriangle(
								{ { right, top, 0.0f, 1.0f }, uiCol, {1.0f, 0.0f}, texID },
								{ { right, bottom, 0.0f, 1.0f }, uiCol, {1.0f, 1.0f}, texID },
								{ { left,  bottom, 0.0f, 1.0f }, uiCol, {0.0f, 1.0f}, texID }
							);
						}
					}
				}

				// ゲームオーバーUIメニュー描画
				if (GameOverMenu_.IsVisible()) {
					Lumina::F32x4 darkenCol{ 0.15f, 0.0f, 0.0f, 0.8f };
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { -1.0f,  1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U },
						{ {  1.0f,  1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U },
						{ { -1.0f, -1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U }
					);
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ {  1.0f,  1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U },
						{ {  1.0f, -1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U },
						{ { -1.0f, -1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U }
					);

					float titleHalfW = (480.0f / 1280.0f);
					float titleH = (120.0f / 720.0f) * 2.0f;
					float titleTopY = 0.9f;
					float titleBottomY = titleTopY - titleH;
					float titleLeftX = -titleHalfW;
					float titleRightX = titleHalfW;

					Lumina::F32x4 titleCol{ 1.0f, 1.0f, 1.0f, 1.0f };
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { titleLeftX,  titleTopY, 0.0f, 1.0f }, titleCol, {0.0f, 0.0f}, 8U },
						{ { titleRightX, titleTopY, 0.0f, 1.0f }, titleCol, {1.0f, 0.0f}, 8U },
						{ { titleLeftX,  titleBottomY, 0.0f, 1.0f }, titleCol, {0.0f, 1.0f}, 8U }
					);
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { titleRightX, titleTopY, 0.0f, 1.0f }, titleCol, {1.0f, 0.0f}, 8U },
						{ { titleRightX, titleBottomY, 0.0f, 1.0f }, titleCol, {1.0f, 1.0f}, 8U },
						{ { titleLeftX,  titleBottomY, 0.0f, 1.0f }, titleCol, {0.0f, 1.0f}, 8U }
					);

					float itemW = (360.0f / 1280.0f) * 2.0f;
					float itemH = (120.0f / 720.0f) * 2.0f;
					float startY = 0.2f;
					float gap = 0.05f;

					for (int i = 0; i < 2; ++i) {
						Lumina::F32x4 color;
						float scale = 1.0f;
						if (GameOverMenu_.SelectedIndex() == i) {
							color = { 1.0f, 1.0f, 1.0f, 1.0f };
							scale = 1.0f + 0.08f * (0.5f + 0.5f * std::sin(Event::PhaseTimer * 8.0f));
						} else {
							color = { 0.4f, 0.4f, 0.4f, 0.9f };
						}

						float currentItemW = itemW * scale;
						float currentItemH = itemH * scale;
						float centerY = startY - i * (itemH + gap) - itemH * 0.5f;
						float topY = centerY + currentItemH * 0.5f;
						float bottomY = centerY - currentItemH * 0.5f;
						float leftX = -currentItemW * 0.5f;
						float rightX = currentItemW * 0.5f;

						uint32_t texID = 6U + i;

						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { leftX,  topY, 0.0f, 1.0f }, color, {0.0f, 0.0f}, texID },
							{ { rightX, topY, 0.0f, 1.0f }, color, {1.0f, 0.0f}, texID },
							{ { leftX,  bottomY, 0.0f, 1.0f }, color, {0.0f, 1.0f}, texID }
						);
						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { rightX, topY, 0.0f, 1.0f }, color, {1.0f, 0.0f}, texID },
							{ { rightX, bottomY, 0.0f, 1.0f }, color, {1.0f, 1.0f}, texID },
							{ { leftX,  bottomY, 0.0f, 1.0f }, color, {0.0f, 1.0f}, texID }
						);
					}
				}

				if (playState_.IsPaused) {
					// 画面全体を少し暗くする
					Lumina::F32x4 darkenCol{ 0.0f, 0.0f, 0.0f, 0.8f };
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { -1.0f,  1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U },
						{ {  1.0f,  1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U },
						{ { -1.0f, -1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U }
					);
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ {  1.0f,  1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U },
						{ {  1.0f, -1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U },
						{ { -1.0f, -1.0f, 0.0f, 1.0f }, darkenCol, {0.0f, 0.0f}, 0U }
					);

					float pauseHalfW = (480.0f / 1280.0f);
					float pauseH = (120.0f / 720.0f) * 2.0f;
					float pauseTopY = 0.9f;
					float pauseBottomY = pauseTopY - pauseH;
					float pauseLeftX = -pauseHalfW;
					float pauseRightX = pauseHalfW;

					Lumina::F32x4 pauseCol{ 1.0f, 1.0f, 1.0f, 1.0f };
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { pauseLeftX,  pauseTopY, 0.0f, 1.0f }, pauseCol, {0.0f, 0.0f}, 2U },
						{ { pauseRightX, pauseTopY, 0.0f, 1.0f }, pauseCol, {1.0f, 0.0f}, 2U },
						{ { pauseLeftX,  pauseBottomY, 0.0f, 1.0f }, pauseCol, {0.0f, 1.0f}, 2U }
					);
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { pauseRightX, pauseTopY, 0.0f, 1.0f }, pauseCol, {1.0f, 0.0f}, 2U },
						{ { pauseRightX, pauseBottomY, 0.0f, 1.0f }, pauseCol, {1.0f, 1.0f}, 2U },
						{ { pauseLeftX,  pauseBottomY, 0.0f, 1.0f }, pauseCol, {0.0f, 1.0f}, 2U }
					);

					float itemW = (360.0f / 1280.0f) * 2.0f;
					float itemH = (120.0f / 720.0f) * 2.0f;
					float startY = 0.4f;
					float gap = 0.05f;

					for (int i = 0; i < 3; ++i) {
						Lumina::F32x4 color;
						float scale = 1.0f;
						if (playState_.PauseSelectedIndex == i) {
							color = { 1.0f, 1.0f, 1.0f, 1.0f };
							scale = 1.0f + 0.08f * (0.5f + 0.5f * std::sin(playState_.PauseAnimationTimer * 8.0f));
						} else {
							color = { 0.4f, 0.4f, 0.4f, 0.9f };
						}

						float currentItemW = itemW * scale;
						float currentItemH = itemH * scale;
						float centerY = startY - i * (itemH + gap) - itemH * 0.5f;
						float topY = centerY + currentItemH * 0.5f;
						float bottomY = centerY - currentItemH * 0.5f;
						float leftX = -currentItemW * 0.5f;
						float rightX = currentItemW * 0.5f;

						uint32_t texID = 3U + i;

						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { leftX,  topY, 0.0f, 1.0f }, color, {0.0f, 0.0f}, texID },
							{ { rightX, topY, 0.0f, 1.0f }, color, {1.0f, 0.0f}, texID },
							{ { leftX,  bottomY, 0.0f, 1.0f }, color, {0.0f, 1.0f}, texID }
						);
						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { rightX, topY, 0.0f, 1.0f }, color, {1.0f, 0.0f}, texID },
							{ { rightX, bottomY, 0.0f, 1.0f }, color, {1.0f, 1.0f}, texID },
							{ { leftX,  bottomY, 0.0f, 1.0f }, color, {0.0f, 1.0f}, texID }
						);
					}
				}

				// --- Pause UI Hint (左下に「Pでポーズ」表示、ポーズ中は非表示) ---
				if (playState_.IsPlaying && !playState_.IsPaused && !GameOverMenu_.IsVisible()) {
					// pause_UI.png は 360x120, テクスチャIndex = 12
					float puiW = (360.0f / 1280.0f) * 2.0f; // NDC幅
					float puiH = (120.0f / 720.0f) * 2.0f;  // NDC高さ

					float puiLeft   = -0.95f;
					float puiRight  = puiLeft + puiW;
					float puiBottom = -0.95f;
					float puiTop    = puiBottom + puiH;

					Lumina::F32x4 puiCol{ 1.0f, 1.0f, 1.0f, 0.7f };
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { puiLeft,  puiTop, 0.0f, 1.0f }, puiCol, {0.0f, 0.0f}, 12U },
						{ { puiRight, puiTop, 0.0f, 1.0f }, puiCol, {1.0f, 0.0f}, 12U },
						{ { puiLeft,  puiBottom, 0.0f, 1.0f }, puiCol, {0.0f, 1.0f}, 12U }
					);
					PrimitiveManager_Tutorial_->BatchTriangle(
						{ { puiRight, puiTop, 0.0f, 1.0f }, puiCol, {1.0f, 0.0f}, 12U },
						{ { puiRight, puiBottom, 0.0f, 1.0f }, puiCol, {1.0f, 1.0f}, 12U },
						{ { puiLeft,  puiBottom, 0.0f, 1.0f }, puiCol, {0.0f, 1.0f}, 12U }
					);
				}

				// --- Screen Fade Overlay (Iris Effect) ---
				if (playState_.ScreenFadeState != 0 || playState_.ScreenFadeAlpha > 0.0f) {
					float targetNdcX = 0.0f;
					float targetNdcY = 0.0f;
					if (Player_ && WorldToHomogeneous_) {
						Lumina::Math::F32x4 playerPos{
							playState_.Player.Position.X,
							playState_.Player.Position.Y + 1.0f,
							playState_.Player.Position.Z,
							1.0f
						};
						auto clip = playerPos * (*WorldToHomogeneous_);
						if (clip.W() > 0.0f) {
							targetNdcX = clip.X() / clip.W();
							targetNdcY = clip.Y() / clip.W();
						}
					}

					// Easing: make it feel slightly snappy
					float t = playState_.ScreenFadeAlpha;
					float easeT = t * t * (3.0f - 2.0f * t); // Smoothstep
					float r = 2.5f * (1.0f - easeT);
					
					Lumina::F32x4 fadeCol{ 0.0f, 0.0f, 0.0f, 1.0f }; // Solid black
					
					const int segments = 32;
					float aspect = 1280.0f / 720.0f;
					float outR = 4.0f; // Large enough to cover the screen corners
					
					for (int i = 0; i < segments; ++i) {
						float theta1 = (2.0f * 3.14159265f * i) / segments;
						float theta2 = (2.0f * 3.14159265f * (i + 1)) / segments;
						
						// Inner circle points
						float inX1 = targetNdcX + (r * std::cos(theta1)) / aspect;
						float inY1 = targetNdcY + (r * std::sin(theta1));
						float inX2 = targetNdcX + (r * std::cos(theta2)) / aspect;
						float inY2 = targetNdcY + (r * std::sin(theta2));
						
						// Outer bounding circle points
						float outX1 = targetNdcX + (outR * std::cos(theta1)) / aspect;
						float outY1 = targetNdcY + (outR * std::sin(theta1));
						float outX2 = targetNdcX + (outR * std::cos(theta2)) / aspect;
						float outY2 = targetNdcY + (outR * std::sin(theta2));
						
						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { inX1, inY1, 0.0f, 1.0f }, fadeCol, {0.0f, 0.0f}, 0U },
							{ { outX1, outY1, 0.0f, 1.0f }, fadeCol, {0.0f, 0.0f}, 0U },
							{ { inX2, inY2, 0.0f, 1.0f }, fadeCol, {0.0f, 0.0f}, 0U }
						);
						PrimitiveManager_Tutorial_->BatchTriangle(
							{ { outX1, outY1, 0.0f, 1.0f }, fadeCol, {0.0f, 0.0f}, 0U },
							{ { outX2, outY2, 0.0f, 1.0f }, fadeCol, {0.0f, 0.0f}, 0U },
							{ { inX2, inY2, 0.0f, 1.0f }, fadeCol, {0.0f, 0.0f}, 0U }
						);
					}
				}

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