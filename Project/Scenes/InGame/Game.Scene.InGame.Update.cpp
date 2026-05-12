module Game.Scene.InGame;

import : Impl;

import <cmath>;
import <numbers>;
import <algorithm>;
import <string>;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import Lumina.Core.Math;
import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.Utils.Data;
import nlohmann.json;
import Lumina.Utils.Color;

import Game.MotionManager;
import Game.EnemyManager;
import Game.ProjectileManager;

import Game.Events;
import Game.UIMenu;
import Lumina.Scene;

#if defined(_DEBUG)
namespace {
	constexpr ImU32 MakeCol32(int r, int g, int b, int a) {
		return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r);
	}
}
#endif

namespace {
	constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };
	constexpr float BossPresentationDuration{ 2.0f };
	constexpr float BossPresentationCameraZoom{ 6.0f };
	constexpr char BossEnemyName[]{ "KingSlime" };
	
	bool UpdatePlayerEffect(Lumina::Particle& p_, void const*) {
		p_.Translate.X += p_.Velocity.X;
		p_.Translate.Z += p_.Velocity.Z;
		p_.RenderData.RGBA.W *= 0.95f;
		p_.Scale.X *= 0.93f;
		p_.Scale.Y *= 0.93f;
		p_.Rotate.Z += p_.Velocity.Z * 0.01f;
		p_.Life -= 1.0f;
		return (p_.Life > 0.0f);
	}
}

namespace Game::Scene::Impl {
   bool InGame::HasBossEncounterInCurrentArea() const {
		return std::any_of(
			playState_.CurrentArea.enemies.cbegin(),
			playState_.CurrentArea.enemies.cend(),
			[] (auto const& enemy_) {
				return enemy_.enemyName == BossEnemyName;
			}
		);
	}

	void InGame::StartBossEncounterPresentation() {
		playState_.IsBossPresentationActive = false;
		playState_.BossPresentationTimer = 0.0f;
		playState_.BossPresentationDuration = 0.0f;

		if (!HasBossEncounterInCurrentArea()) {
			return;
		}

		auto const bossIt = std::find_if(
			playState_.Enemies.cbegin(),
			playState_.Enemies.cend(),
			[] (auto const& enemy_) {
				return enemy_.BaseData.name == BossEnemyName;
			}
		);

		if (bossIt == playState_.Enemies.cend()) {
			return;
		}

		playState_.IsBossPresentationActive = true;
		playState_.BossPresentationTimer = BossPresentationDuration;
		playState_.BossPresentationDuration = BossPresentationDuration;
		playState_.BossPresentationFocusPosition = bossIt->Position;
		playState_.TransitionCooldownTimer = (std::max)(playState_.TransitionCooldownTimer, BossPresentationDuration);
		Event::CameraShakingTimer = (std::max)(Event::CameraShakingTimer, 20);
	}

	void InGame::CheckAndLoadArea(int areaIndex, int previousAreaIndex) {
		std::string filename = "area" + std::to_string(areaIndex) + ".json";
		areaEditor_.LoadArea(playState_.CurrentArea, filename);
		
		try {
			// Load Terrain (which reads "Polygons" and "GroundPoints" stored inside area json)
			TerrainScreenData_ = std::make_unique<TerrainShapeCollection>();
			TerrainScreenData_->Initialize(
				Lumina::Utils::LoadFromFile<nlohmann::json>(filename, "Assets/Data/Terrain/")
			);
			
			Terrain_ = std::make_unique<TerrainShapeCollection>();
			TerrainScreenData_->ConvertToWorldCoordinate(
				*Terrain_,
				*Camera_,
				{ 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f }
			);
#if defined(_DEBUG)
			TerrainEditor_->SetShapes(*Terrain_);
#endif
		} catch (...) {
			// Fallback or empty terrain if file has no terrain data yet
			TerrainScreenData_ = std::make_unique<TerrainShapeCollection>();
			Terrain_ = std::make_unique<TerrainShapeCollection>();
		}
		
		// Reset player position when entering area
		playState_.Player.Position.Y = 0.0f;
		playState_.Player.Position.Z = 0.0f;
		playState_.Player.Velocity = {0.f, 0.f, 0.f};

		float playerScreenX = 100.0f;
		float playerScreenY = 0.0f;

		// Spawn location logic
		bool spawnedAtConnection = false;
		if (previousAreaIndex != -1) {
			for (const auto& conn : playState_.CurrentArea.connections) {
				if (conn.targetAreaIndex == previousAreaIndex && !(areaIndex == 0 && previousAreaIndex == 0)) {
					// Spawn at the center of the connection linking back to where we came from
					playerScreenX = conn.position.x;
					playerScreenY = conn.position.y;
					spawnedAtConnection = true;
					break;
				}
			}
		}

		if (!spawnedAtConnection && areaIndex == 0) {
			for (const auto& conn : playState_.CurrentArea.connections) {
				if (conn.targetAreaIndex == 0) {
					playerScreenX = conn.position.x;
					playerScreenY = conn.position.y;
					spawnedAtConnection = true;
					break;
				}
			}
		}

		if (TutorialManager_) {
			// エリア遷移時に現在アクティブなチュートリアルを中断
			TutorialManager_->Skip();

			if (areaIndex == 0) {
				TutorialManager_->TryStartSequence("BasicControls");
			} else if (areaIndex == 2) {
				TutorialManager_->TryStartSequence("Parachute");
			}
		}

		if (!spawnedAtConnection) {
			playerScreenX = 100.0f; // Fallback / Start location
		}

		// Convert Player, Enemies, and Connections to World Coordinates
		if (Camera_) {
			auto const worldToHomogeneous_c = Camera_->View() * Camera_->Projection();
			auto tmp{ Lumina::Math::F32x4{ 0.0f, 0.0f, 0.0f, 1.0f } * worldToHomogeneous_c };
			tmp /= tmp.W();

			Lumina::F32 const inv_ViewportWidth{ 1.0f / 1280.0f };
			Lumina::F32 const inv_ViewportHeight{ 1.0f / 720.0f };
			
			auto const& inv_View{ Camera_->ViewInverse() };
			auto const inv_Proj{ Camera_->Projection().Inverse() };
			auto const ndcToWorld{ inv_Proj * inv_View };

			auto ScreenToWorld = [&](float sx, float sy) {
				Lumina::Math::F32x4 ndcPos{
					(sx * inv_ViewportWidth) * 2.0f - 1.0f,
					1.0f - (sy * inv_ViewportHeight) * 2.0f,
					tmp.Z(),
					1.0f
				};
				auto worldPos = ndcPos * ndcToWorld;
				worldPos /= worldPos.W();
				return std::make_pair(worldPos.X(), worldPos.Y());
			};

			// Apply Player Coordinates
			auto pPos = ScreenToWorld(playerScreenX, playState_.CurrentArea.height - playerScreenY);
			playState_.Player.Position.X = pPos.first;
			playState_.Player.Position.Y = pPos.second;
			playState_.Player.Position.Z = 0.0f;

			// Apply Enemies Coordinates
			for (auto& ep : playState_.CurrentArea.enemies) {
				auto ePos = ScreenToWorld(ep.position.x, playState_.CurrentArea.height - ep.position.y);
				ep.position.x = ePos.first;
				ep.position.y = ePos.second;
			}

			// Apply Connections Coordinates
			playState_.PortalColliders.clear();
			for (auto& conn : playState_.CurrentArea.connections) {
				auto connWPos = ScreenToWorld(conn.position.x, playState_.CurrentArea.height - conn.position.y);
				conn.position.x = connWPos.first;
				conn.position.y = connWPos.second;
				
				float wSizeX = 1.5f;
				float wSizeY = 1.5f;
				auto col = std::make_shared<ConvexCollider>();
				col->SetMyType(COL_None);
				col->SetYourType(COL_None);
				std::vector<Lumina::Math::F32x3> verts = {
					{ connWPos.first - wSizeX, connWPos.second - wSizeY, -0.5f },
					{ connWPos.first + wSizeX, connWPos.second - wSizeY, -0.5f },
					{ connWPos.first + wSizeX, connWPos.second + wSizeY, -0.5f },
					{ connWPos.first - wSizeX, connWPos.second + wSizeY, -0.5f },
					{ connWPos.first - wSizeX, connWPos.second - wSizeY, 0.5f },
					{ connWPos.first + wSizeX, connWPos.second - wSizeY, 0.5f },
					{ connWPos.first + wSizeX, connWPos.second + wSizeY, 0.5f },
					{ connWPos.first - wSizeX, connWPos.second + wSizeY, 0.5f }
				};
				col->SetVertices(verts);
				col->UpdateAABB();
				playState_.PortalColliders.push_back(col);
			}
		} else {
			playState_.Player.Position.X = playerScreenX;
			playState_.Player.Position.Y = playerScreenY;
		}

		playState_.Enemies.clear();
		Game::EnemyManager::GetInstance()->ClearInstances();
		Game::ProjectileManager::GetInstance()->ClearAll();

		for (auto& ep : playState_.CurrentArea.enemies) {
			PlayEnemy pe;
			enemyEditor_.LoadEnemy(pe.BaseData, ep.enemyName + ".json");
			pe.Position.X = ep.position.x;
			pe.Position.Y = ep.position.y; 
			pe.Position.Z = 0.0f;

			// サイズ段階に応じたステータスを適用
			int tierIdx = (std::max)(0, (std::min)(2, ep.sizeCategory));
			pe.BaseData.hp    = pe.BaseData.sizeTiers[tierIdx].hp;
			pe.BaseData.power = pe.BaseData.sizeTiers[tierIdx].power;
			pe.SizeTier       = tierIdx;
			pe.Scale          = pe.BaseData.sizeTiers[tierIdx].scale;

			pe.CurrentHP = pe.BaseData.hp;
			pe.IsDead = false;
			pe.FacingRight = ep.facingRight;
           pe.RenderFacingYaw = pe.FacingRight ? 0.0f : 3.14159265f;
			playState_.Enemies.push_back(pe);

			// EnemyManager側にも生成
			Game::EnemyManager::GetInstance()->SpawnFromData(pe.BaseData, pe.Position, pe.FacingRight, pe.Scale, pe.SizeTier);
		}

		if (Player_) {
			Player_->SetPosition({ playState_.Player.Position.X, playState_.Player.Position.Y, 0.0f });
			Event::RespawnPos = Player_->GetPosition();
			Player_->myVelocity_ = { 0.0f, 0.0f, 0.0f };
			Player_->externalVelocity_ = { 0.0f, 0.0f, 0.0f };
		}
		
		playState_.TransitionCooldownTimer = 0.5f; // Add delay
		if (previousAreaIndex != -1) {
			StartBossEncounterPresentation();
		}
		else {
			playState_.IsBossPresentationActive = false;
			playState_.BossPresentationTimer = 0.0f;
			playState_.BossPresentationDuration = 0.0f;
		}
		
		if (CollisionManager_) {
			CollisionManager_->Begin();
		}

		playState_.IsGoalReached = false;

		// ゲームフェーズをリセット
		Event::CurrentPhase = Event::GamePhase::InBattle;
		Event::PhaseTimer = 0.0f;
		Event::ElapsedBattleTime = 0.0f;
		Event::EnemiesDefeated = 0;
		Event::FallDeathCount = 0;
	}

#if defined(_DEBUG)
	void InGame::DrawPlayMode() {
		ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_FirstUseEver);
		ImGui::Begin("Player Info");
		if (Player_) {
			auto const& pos = Player_->GetPosition();
			ImGui::Text("Player 3D Position: %.2f, %.2f, %.2f", pos.X, pos.Y, pos.Z);
		}
		ImGui::Separator();

		// ポーズ・リスタートUI
		if (playState_.IsPaused) {
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "== PAUSED ==");
		}

		if (ImGui::Button(playState_.IsPaused ? "Resume (P)" : "Pause (P)", ImVec2(140, 0))) {
			playState_.IsPaused = !playState_.IsPaused;
		}
		ImGui::SameLine();
		if (ImGui::Button("Restart", ImVec2(140, 0))) {
			playState_.IsPaused = false;
			CheckAndLoadArea(0);
		}

		// Pキーによるポーズは InGame::Update で処理するように変更済み

		ImGui::End();
	}
#endif

	template<>
	void InGame::Update_<"Player">() {
		// チュートリアル入力制限の適用
		if (TutorialManager_ && TutorialManager_->IsActive()) {
			Player_->InputMask = TutorialManager_->GetAllowedInputs();
		} else {
			Player_->InputMask = 0xFFFF; // 全入力許可
		}

		Player_->Update(1.0f / 60.0f);

		playState_.Player.Position.X = Player_->GetPosition().X;
		playState_.Player.Position.Y = Player_->GetPosition().Y;
		playState_.Player.Position.Z = Player_->GetPosition().Z;
	}

	template<>
	void InGame::Update_<"Enemies-1">(Lumina::F32&& deltaTime_) {
		Game::EnemyManager::GetInstance()->Update(
			deltaTime_,
			Player_->GetPosition()
		);
	}

	template<>
	void InGame::Update_<"Collision">() {
		// プロジェクタイル更新
		Game::ProjectileManager::GetInstance()->Update(1.0f / 60.0f, Player_->GetPosition());

		// 中身をclear
		CollisionManager_->Begin();

		// ここからColliderを設定
		Game::EnemyManager::GetInstance()->RegisterCollidersTo(*CollisionManager_);
		Game::ProjectileManager::GetInstance()->RegisterCollidersTo(*CollisionManager_);
		CollisionManager_->SetColliders(Player_->GetCollider());
		CollisionManager_->SetColliders(Player_->GetUmbrella().top_->GetCollider());
		CollisionManager_->SetColliders(Player_->GetSmashCollider());
		for (auto const& polygon : Terrain_->PolygonsData()) {
			CollisionManager_->SetColliders(polygon.Col.get());
		}
		auto const& groundColliders = Terrain_->GroundData().Colliders;
		for (auto const& col : groundColliders) {
			CollisionManager_->SetColliders(col.get());
		}
		
		for (auto const& pCol : playState_.PortalColliders) {
			if (pCol) {
				CollisionManager_->SetColliders(pCol.get());
			}
		}

		// Check!
		CollisionManager_->CheckAllCollisions();
	}

	template<>
	void InGame::Update_<"Enemies-2">() {
		// 死亡済みプロジェクタイルを除去
		Game::ProjectileManager::GetInstance()->RemoveDeadProjectiles();

		const auto& enemyInstances = Game::EnemyManager::GetInstance()->GetAllInstances();
		playState_.Enemies.clear();
		playState_.Enemies.reserve(enemyInstances.size());
		for (const auto& inst : enemyInstances) {
			PlayEnemy pe;
			pe.BaseData = inst.baseData;
			pe.Position = inst.position;
			pe.CurrentHP = inst.isDead ? 0 : inst.currentHP;
			pe.IsDead = inst.isDead;
          pe.HurtTimer = inst.hurtTimer;
			pe.FacingRight = inst.facingRight;
            pe.RenderFacingYaw = inst.renderFacingYaw;
			pe.SizeTier = inst.sizeTier;
			pe.Scale = inst.modelScale;
			// pull debug flag from behavior if available
			if (inst.behavior) {
				pe.WalkActive = inst.behavior->IsWalkActive();
				pe.MotionPlaying = inst.behavior->IsMotionPlaying();
				pe.ActiveNodeIndex = inst.behavior->GetActiveNodeIndex();
			} else {
				pe.WalkActive = false;
			}
			playState_.Enemies.push_back(std::move(pe));
		}
	}

	template<>
	void InGame::Update_<"[Debug] TerrainEditor">() {
		#if defined(_DEBUG)
		//TerrainEditor_->Update();
		#endif
	}

	template<>
	void InGame::Update_<"[Debug] Area">() {
		// エリアの移動処理
#if defined(_DEBUG)
		bool shouldProcessAreaTransition = (activeEditor_ == EditorTab::Play && playState_.IsPlaying);
#else
		bool shouldProcessAreaTransition = playState_.IsPlaying;
#endif
		if (shouldProcessAreaTransition) {
			if (playState_.TransitionCooldownTimer > 0.0f) {
				playState_.TransitionCooldownTimer -= 1.0f / 60.0f;
			}
			else {
				auto const& pos = playState_.Player.Position;
				auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
				auto const& keyboard{ inputMngr.Keyboard() };
				using Lumina::OS::Windows::KEY;

				for (const auto& conn : playState_.CurrentArea.connections) {
					if (std::abs(pos.X - conn.position.x) <= 1.5f &&
						std::abs(pos.Y - conn.position.y) <= 1.5f) {

						if (keyboard.IsJustPressed(KEY::W) || inputMngr.Pad().IsHold(0x0001)) {
							int prevAreaIndex = playState_.CurrentArea.index;
							CheckAndLoadArea(conn.targetAreaIndex, prevAreaIndex);
							break;
						}
					}
				}
			}
		}
	}

	template<>
	void InGame::Update_<"Camera">() {
		#if defined(_DEBUG)
		ImGui::Begin("Camera");
		static Lumina::Math::F32x3 eye{ 0.0f, 5.0f, -30.0f };
		static Lumina::Math::F32x3 target{ 0.0f, 5.0f, 0.0f };
		static bool isUsingDebugCamera = false;
		ImGui::Checkbox("Use Debug Camera", &isUsingDebugCamera);
		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		[[maybe_unused]] auto const& mouse{ inputMngr.Mouse() };
		ImGui::DragFloat3("Eye", &eye.X, 0.1f);
		ImGui::DragFloat3("Target", &target.X, 0.1f);
		Camera_->LookAt(eye, target, { 0.0f, 1.0f, 0.0f });
		ImGui::End();
		#endif

		if (!playState_.IsPaused) {
			Lumina::Math::F32x3 cameraPos = Camera_Player_->WorldPosition();
			auto const& playerPos = Player_->GetPosition();
			Lumina::Math::F32x3 newCameraPos{};
			if (playState_.IsBossPresentationActive && playState_.BossPresentationDuration > 0.0f) {
				float const progress = 1.0f - playState_.BossPresentationTimer / playState_.BossPresentationDuration;
				float const bossFocusWeight = std::sin(progress * std::numbers::pi_v<float>);
				newCameraPos = {
					playerPos.X + (playState_.BossPresentationFocusPosition.X - playerPos.X) * bossFocusWeight,
					playerPos.Y + ((playState_.BossPresentationFocusPosition.Y + 2.0f) - playerPos.Y) * bossFocusWeight,
					-30.0f + BossPresentationCameraZoom * bossFocusWeight
				};
				Event::CameraShakingTimer = (std::max)(Event::CameraShakingTimer, 2);
			}
			else {
				newCameraPos = {
					cameraPos.X * 0.95f + playerPos.X * 0.05f,
					cameraPos.Y * 0.95f + playerPos.Y * 0.05f,
					-30.0f
				};
			}
			if (Event::CameraShakingTimer > 0) {
				auto angleInDeg = Lumina::Math::Random::Generator()() % 3;
				angleInDeg += (Lumina::Math::Random::Generator()() & 1) * 180;
				float const angleInRad = Lumina::Math::DegToRad(static_cast<float>(angleInDeg));
				Lumina::Math::F32x2 const dir = { Lumina::Math::COS(angleInRad), Lumina::Math::SIN(angleInRad) };
				float const mag = std::exp(static_cast<float>(Event::CameraShakingTimer) / 15.0f) * 0.1f;
				newCameraPos += { dir.X* mag, dir.Y* mag, 0.0f };
				--Event::CameraShakingTimer;
			}
			Camera_Player_->LookAt(newCameraPos, { newCameraPos.X, newCameraPos.Y, 0.0f }, { 0.0f, 1.0f, 0.0f });
		}

		#if defined(_DEBUG)
		if (!isUsingDebugCamera) {
			*WorldToHomogeneous_ = Camera_Player_->View() * Camera_->Projection();
		}
		else {
			*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
		}
		#else
		*WorldToHomogeneous_ = Camera_Player_->View() * Camera_->Projection();
		#endif
	}

	
	template<>
	void InGame::Update_<"Particles">(
		Lumina::D3D12::CommandList const& cmdList_,
		Lumina::Math::F32x4x4<> const& viewToWorld_
	) {
		auto& rndEngine{ Lumina::Math::Random::Generator() };

		// Player effects
		{
			static float playerEffectTimeFactor{ 0.0f };
			playerEffectTimeFactor += 0.5f;

			auto rgb_Gaming = Lumina::Utils::Color::Convert(
				Lumina::Utils::Color::HSV{
					rndEngine() * Inv_0xFFFFFFFF * 60.0f +
					playerEffectTimeFactor +
					playerEffectTimeFactor * 0.1f * 180.0f * std::numbers::inv_pi_v<float>,
					rndEngine() * Inv_0xFFFFFFFF * 0.3f + 0.5f,
					0.95f
				}
			);

			for (int i = 0; i < 2; ++i) {
				Lumina::Particle playerEffect{};
				{
					playerEffect.Translate = {
						std::cos(playerEffectTimeFactor * 0.3f + i * 3.6f) * 1.5f,
						std::sin(playerEffectTimeFactor * 0.4f * i) * 1.0f,
						std::sin(playerEffectTimeFactor * 0.5f - i * 1.2f) * 1.5f
					};

					playerEffect.Velocity.X = playerEffect.Translate.Z * (-0.05f);
					playerEffect.Velocity.Z = playerEffect.Translate.X * (-0.05f);

					playerEffect.Translate.X += Player_->GetPosition().X;
					playerEffect.Translate.Y += Player_->GetPosition().Y;
					playerEffect.Translate.Z += Player_->GetPosition().Z;

					playerEffect.Scale.X = 0.7f;
					playerEffect.Scale.Y = 0.7f;

					playerEffect.Rotate.Z = rndEngine() * Inv_0xFFFFFFFF * std::numbers::pi_v<float> *2.0f;

					playerEffect.Life = 64.0f;

					playerEffect.RenderData.RGBA = {
						rgb_Gaming.R + rndEngine() * Inv_0xFFFFFFFF * 0.1f,
						rgb_Gaming.G + rndEngine() * Inv_0xFFFFFFFF * 0.1f,
						rgb_Gaming.B,
						0.15f
					};
					// Particles TextureのIDは1
					playerEffect.RenderData.DiffuseID = 1U;
					playerEffect.RenderData.DiffuseAtlasID = (rndEngine() & 3) ? (3U) : (4U);
					PlayerEffects_->Emit(std::move(playerEffect));
				}
			}

			/*if (PlayerJumpEffectEmitFrameCount > 0) {
				for (int i = 0; i < 2; ++i) {
					Particle p_Jump{};
					{
						p_Jump.Translate = {
							std::cos(playerEffectTimeFactor * 1.6f + std::numbers::pi_v<float> *i) * 0.5f,
							std::sin(playerEffectTimeFactor * 0.8f) * 0.1f,
							std::sin(playerEffectTimeFactor * 1.6f + std::numbers::pi_v<float> *i) * 0.5f
						};

						p_Jump.Velocity.x = p_Jump.Translate.z * (-0.05f);
						p_Jump.Velocity.z = p_Jump.Translate.x * (-0.05f);

						p_Jump.Translate.x += Player_->ModelTranslate().x;
						p_Jump.Translate.y += Player_->ModelTranslate().y - 1.0f;
						p_Jump.Translate.z += Player_->ModelTranslate().z;

						p_Jump.Scale.x = 1.0f;
						p_Jump.Scale.y = 1.0f;

						p_Jump.Rotate.z = rndEngine() * Inv_0xFFFFFFFF * std::numbers::pi_v<float> *2.0f;

						p_Jump.Life = 32.0f;

						p_Jump.RenderData.RGBA = {
							0.1f + rgb_Gaming.R + rndEngine() * Inv_0xFFFFFFFF * 0.05f,
							0.1f + rgb_Gaming.G + rndEngine() * Inv_0xFFFFFFFF * 0.05f,
							0.1f + rgb_Gaming.B + rndEngine() * Inv_0xFFFFFFFF * 0.05f,
							1.0f
						};
						p_Jump.RenderData.DiffuseID = 0U;
						p_Jump.RenderData.DiffuseAtlasID = (rndEngine() & 3) ? (3U) : (4U);
						PlayerEffects_->Emit(std::move(p_Jump));
					}
				}
				--PlayerJumpEffectEmitFrameCount;
			}*/

			/*if (PlayerDashEffectEmitFrameCount > 0) {
				float const cos_Theta{ std::cos(Player_->Angle()) };
				float const sin_Theta{ std::sin(Player_->Angle()) };

				for (int i = 0; i < 2; ++i) {
					Particle p_Dash{};
					{
						p_Dash.Translate = {
							std::cos(playerEffectTimeFactor * 2.4f + std::numbers::pi_v<float> *i) * 0.2f +
							cos_Theta * 0.9f,
							std::sin(playerEffectTimeFactor * 0.8f) * 0.1f,
							std::sin(playerEffectTimeFactor * 2.4f + std::numbers::pi_v<float> *i) * 0.2f +
							sin_Theta * 0.9f
						};

						p_Dash.Velocity.x = p_Dash.Translate.x * (-0.1f);
						p_Dash.Velocity.z = p_Dash.Translate.z * (-0.1f);

						p_Dash.Translate.x += Player_->ModelTranslate().x;
						p_Dash.Translate.y += Player_->ModelTranslate().y;
						p_Dash.Translate.z += Player_->ModelTranslate().z;

						p_Dash.Scale.x = 3.0f;
						p_Dash.Scale.y = 3.0f;

						p_Dash.Rotate.z = rndEngine() * Inv_0xFFFFFFFF * std::numbers::pi_v<float> *2.0f;

						p_Dash.Life = 32.0f;

						p_Dash.RenderData.RGBA = {
							0.1f + rgb_Gaming.R,
							0.1f + rgb_Gaming.G,
							0.1f + rgb_Gaming.B,
							0.5f
						};
						p_Dash.RenderData.DiffuseID = 0U;
						p_Dash.RenderData.DiffuseAtlasID = (rndEngine() & 3) ? (3U) : (4U);
						PlayerEffects_->Emit(std::move(p_Dash));
					}
				}
				--PlayerDashEffectEmitFrameCount;
			}*/

			
			PlayerEffects_->Update(cmdList_, viewToWorld_, UpdatePlayerEffect);
		}

		//// Ambient sparkles
		//{
		//	static float sparkleTimeFactor{ 0.0f };
		//	sparkleTimeFactor += 0.75f;

		//	float const spawnPosRad = rndEngine() * Inv_0xFFFFFFFF * 100.0f;
		//	float const spawnPosTheta = rndEngine() * Inv_0xFFFFFFFF * std::numbers::pi_v<float> *2.0f;
		//	float const x{ spawnPosRad * std::cos(spawnPosTheta) };
		//	float const z{ spawnPosRad * std::sin(spawnPosTheta) };
		//	if (std::abs(x) < 40.0f && std::abs(z) < 40.0f) {
		//		Particle sparkle{};
		//		sparkle.Translate = {
		//			x,
		//			rndEngine() * Inv_0xFFFFFFFF * 2.0f,
		//			z
		//		};
		//		sparkle.Scale.x = 1.0f;
		//		sparkle.Scale.y = 1.0f;
		//		sparkle.Life = 60.0f;
		//		auto rgb = Lumina::Utils::Color::Convert(
		//			Lumina::Utils::Color::HSV{
		//				rndEngine() * Inv_0xFFFFFFFF * 45.0f,
		//				rndEngine() * Inv_0xFFFFFFFF * 0.5f + 0.5f,
		//				0.75f
		//			}
		//		);
		//		auto rgb_Gaming = Lumina::Utils::Color::Convert(
		//			Lumina::Utils::Color::HSV{
		//				rndEngine() * Inv_0xFFFFFFFF * 45.0f +
		//				sparkleTimeFactor +
		//				spawnPosTheta * 180.0f * std::numbers::inv_pi_v<float>,
		//				rndEngine() * Inv_0xFFFFFFFF * 0.3f + 0.5f,
		//				0.95f
		//			}
		//		);
		//		sparkle.RenderData.RGBA = {
		//			rgb.R * (0.7f + rgb_Gaming.R * 0.3f),
		//			rgb.G * (0.7f + rgb_Gaming.G * 0.3f),
		//			rgb.B * (0.7f + rgb_Gaming.B * 0.3f),
		//			0.0f
		//		};
		//		sparkle.RenderData.DiffuseID = 0U;
		//		sparkle.RenderData.DiffuseAtlasID = 5U;
		//		AmbientSparkles_->Emit(std::move(sparkle));
		//	}

		//	AmbientSparkles_->Update(cmdList_, viewToWorld, UpdateAmbientSparkle);
		//}

		//KnockEffects_->Update(cmdList_, viewToWorld, UpdateKnockEffect);
	}
	

	template<>
	void InGame::Update_<"Lighting">() {
		List_PointLight_.Clear();
		List_LocalToWorld_LightSphere_.Clear();

		using ParticleListIterator = Lumina::List<Lumina::Particle>::Iterator;

		auto makePointLightBasedOnParticle{
			[this] (
				Lumina::Particle const& particle_,
				Lumina::F32 intensity_
			) -> Lumina::PointLight& {
				auto& pointLight{ List_PointLight_.New() };

				pointLight.WorldPosition = {
					particle_.Translate.X,
					particle_.Translate.Y,
					particle_.Translate.Z,
					1.0f
				};
				pointLight.RGB = {
					particle_.RenderData.RGBA.X,
					particle_.RenderData.RGBA.Y,
					particle_.RenderData.RGBA.Z
				};
				pointLight.Intensity = particle_.Scale.X * intensity_;
				
				return pointLight;
			}
		};

		auto makeLightSphereTransform{
			[this] (
				Lumina::PointLight const& pointLight_,
				Lumina::F32 inv_Threshold_ = 1024.0f,
				Lumina::F32 factor_MaxComp_intensity_ = 0.5f,
				Lumina::F32 factor_Constant_ = 1.0f,
				Lumina::F32 factor_Linear_ = 1.0f,
				Lumina::F32 factor_Quadratic_ = 0.5f
			) -> void {
				auto& lightSphere{ List_LocalToWorld_LightSphere_.New() };

				Lumina::F32 const radius{
					Lumina::LightSphereRadius(
						inv_Threshold_,
						pointLight_.Intensity * factor_MaxComp_intensity_,
						factor_Constant_,
						factor_Linear_,
						factor_Quadratic_
					)
				};
				lightSphere = {
					radius, 0.0f, 0.0f, 0.0f,
					0.0f, radius, 0.0f, 0.0f,
					0.0f, 0.0f, radius, 0.0f,
					pointLight_.WorldPosition.X,
					pointLight_.WorldPosition.Y,
					pointLight_.WorldPosition.Z,
					1.0f,
				};
			}
		};

		ParticleListIterator it_KnockEffect{ KnockEffects_->InstanceList() };
		for (it_KnockEffect.Begin(); !it_KnockEffect.End(); it_KnockEffect.Next()) {
			auto const& knockEffect{ *it_KnockEffect };

			if (!List_PointLight_.IsFull()) {
				auto& pointLight{
					makePointLightBasedOnParticle(
						knockEffect,
						knockEffect.Scale.X * 30.0f
					)
				};
				makeLightSphereTransform(pointLight);
			}
		}

		ParticleListIterator it_PlayerEffect{ PlayerEffects_->InstanceList() };
		for (it_PlayerEffect.Begin(); !it_PlayerEffect.End(); it_PlayerEffect.Next()) {
			auto const& playerEffect{ *it_PlayerEffect };

			if (!List_PointLight_.IsFull() && (playerEffect.RenderData.DiffuseAtlasID == 4U)) {
				auto& pointLight{
					makePointLightBasedOnParticle(
						playerEffect,
						playerEffect.RenderData.RGBA.W * 100.0f
					)
				};
				makeLightSphereTransform(pointLight);
			}
		}

		ParticleListIterator it_Sparkle{ AmbientSparkles_->InstanceList() };
		for (it_Sparkle.Begin(); !it_Sparkle.End(); it_Sparkle.Next()) {
			auto const& sparkle{ *it_Sparkle };

			if (!List_PointLight_.IsFull()) {
				auto& pointLight{
					makePointLightBasedOnParticle(
						sparkle,
						sparkle.RenderData.RGBA.W * 400.0f
					)
				};
				makeLightSphereTransform(pointLight, 1024.0f, 1.0f, 1.0f, 1.0f, 0.5f);
			}
		}

		Arr_Index_ActivePointLight_.clear();
		Lumina::List<Lumina::PointLight>::Iterator it_Light{ List_PointLight_ };
		for (it_Light.Begin(); !it_Light.End(); it_Light.Next()) {
			Arr_Index_ActivePointLight_.emplace_back(it_Light.Index());
		}

		DeferredLighting_->Update(
			List_PointLight_,
			List_LocalToWorld_LightSphere_,
			Arr_Index_ActivePointLight_
		);
	}

	template<>
	void InGame::Update_<"[Debug] Editor">() {
		#if defined(_DEBUG)
		// メインメニューバー: エディタ切り替え
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Mode")) {
				if (ImGui::MenuItem("Play Prototype", nullptr, activeEditor_ == EditorTab::Play)) {
					activeEditor_ = EditorTab::Play;
					if (!playState_.IsPlaying) {
						playState_.IsPlaying = true;
						CheckAndLoadArea(0); // Load default area 0
					}
				}
				if (ImGui::MenuItem("Motion Editor", nullptr, activeEditor_ == EditorTab::Motion)) {
					activeEditor_ = EditorTab::Motion;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Obj Motion Editor", nullptr, activeEditor_ == EditorTab::ObjMotion)) {
					activeEditor_ = EditorTab::ObjMotion;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Area Editor", nullptr, activeEditor_ == EditorTab::Area)) {
					activeEditor_ = EditorTab::Area;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Enemy Editor", nullptr, activeEditor_ == EditorTab::Enemy)) {
					activeEditor_ = EditorTab::Enemy;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Enemy Action Editor", nullptr, activeEditor_ == EditorTab::EnemyAction)) {
					activeEditor_ = EditorTab::EnemyAction;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Actor Editor", nullptr, activeEditor_ == EditorTab::Actor)) {
					activeEditor_ = EditorTab::Actor;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Terrain Editor", nullptr, activeEditor_ == EditorTab::Terrain)) {
					activeEditor_ = EditorTab::Terrain;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Audio Editor", nullptr, activeEditor_ == EditorTab::Audio)) {
					activeEditor_ = EditorTab::Audio;
					playState_.IsPlaying = false;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// アクティブなエディタを描画
		switch (activeEditor_) {
		case EditorTab::Motion:
			MotionEditor::GetInstance()->NodeImGui();
			break;
		case EditorTab::ObjMotion:
			objMotionEditor_.Update();
			break;
		case EditorTab::Area:
			areaEditor_.Update();
			break;
		case EditorTab::Enemy:
			enemyEditor_.Update();
			break;
		case EditorTab::EnemyAction:
			enemyActionEditor_.Update();
			break;
		case EditorTab::Actor:
			actorEditor_.Update();
			break;
		case EditorTab::Terrain:
			if (TerrainEditor_) TerrainEditor_->Update();
			break;
		case EditorTab::Audio:
			audioEditor_.Update();
			break;
		case EditorTab::Play:
			DrawPlayMode();
			// Enemy HP ImGui removed

			// ミニマップ（エリア構成図）描画
			if (playState_.IsPlaying) {
				areaEditor_.DrawAreaMap(playState_.CurrentArea.index);
			}
			break;
		default:
			break;
		}
		#endif
	}

	template<>
	void InGame::Update_<"[Debug] Manual">() {
		#if defined(_DEBUG)
		ImGui::Begin("Manual");

		ImGui::Text("Enter Key or GamePad Start : Player Respawn");

		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& pad{ inputMngr.Pad() };
		auto textColor = [](bool cond_) -> ImVec4 {
			if (cond_) {
				return ImVec4{ 1.0f, 0.3f, 0.3f, 1.0f };
			}
			else {
				return ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
			}
		};

		auto const playerWeaponStance = Player_->GetWeaponStance();
		auto const weaponState = Player_->GetUmbrella().top_->GetUmbrellaForm();
		//auto const& playerInput = Player_->GetInput();
		// 納刀
		if (playerWeaponStance == WeaponStance::Sheathed) {
			ImGui::SeparatorText("Sheathed (Noutou)");
			
			ImGui::TextColored(
				textColor(pad.GetLeftStickX() != 0.0f || pad.GetLeftStickY() != 0.0f),
				"Left Stick : Move"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x8000)),
				"Y : Battou"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x2000)),
				"B"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x1000)),
				"A : Jump"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x4000)),
				"X : Repair Umbrella"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0100)),
				"L Button : Use Mana"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0200)),
				"R Button"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0040)),
				"L2 Button"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0080)),
				"R2 Button"
			);
		}
		// 抜刀
		else {
			ImGui::SeparatorText("Drawn (Battou)");
			ImGui::TextColored(
				textColor(pad.GetLeftStickX() != 0.0f || pad.GetLeftStickY() != 0.0f),
				"Left Stick : Move"
			);

			switch (weaponState) {
				case UmbrellaForm::Closed:
					ImGui::TextColored(
						textColor(pad.IsHold(0x8000)),
						"Y : Attack"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x2000)),
						"B"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x1000)),
						"A : Jump"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x4000)),
						"X : Noutou"
					);
					break;
				case UmbrellaForm::Opened:
					ImGui::TextColored(
						textColor(pad.IsHold(0x8000)),
						"Y"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x2000)),
						"B : Reverse Umbrella"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x1000)),
						"A : Jump"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x4000)),
						"X : Close Umbrella"
					);
					break;
				case UmbrellaForm::Reverse:
					ImGui::TextColored(
						textColor(pad.IsHold(0x8000)),
						"Y (Nagaoshi) : Charge Attack"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x2000)),
						"B"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x1000)),
						"A : Jump"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x4000)),
						"X : Reverse Umbrella Again"
					);
					break;
			}
			ImGui::TextColored(
				textColor(pad.IsHold(0x0100)),
				"L Button (Nagaoshi) : Use Mana"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0200)),
				"R Button"
			);
			if (weaponState == UmbrellaForm::Flying || weaponState == UmbrellaForm::AirStop) {
				ImGui::TextColored(
					textColor(pad.IsHold(0x0040)),
					"L2 Button : Recall Umbrella Top"
				);
			}
			else {
				ImGui::TextColored(
					textColor(pad.IsHold(0x0040)),
					"L2 Button : Aim"
				);
			}

			if (weaponState == UmbrellaForm::Closed) {
				ImGui::TextColored(
					textColor(pad.IsHold(0x0080)),
					"R2 Button: Open Umbrella"
				);
				
			}
			else if (weaponState == UmbrellaForm::Opened) {
				if (pad.IsHold(0x0040)) {
					ImGui::TextColored(
						textColor(pad.IsHold(0x0080)),
						"R2 Button: Shoot Umbrella Top"
					);
				}
				else {
					ImGui::TextColored(
						textColor(pad.IsHold(0x0080)),
						"R2 Button: Guard"
					);
				}
			}
		}
		ImGui::End();
		#endif
	}

	// ==============================
	//  ⑥ ポーズ・リスタート
	// ==============================
#if defined(_DEBUG)
	void InGame::DrawPauseMenu() {
		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		auto const& pad{ inputMngr.Pad() };
		using Lumina::OS::Windows::KEY;

		// ESC or Start ボタンでポーズ切り替え
		bool padStartNow = pad.IsHold(0x0010);
		bool padStartJust = padStartNow && !Event::PrevPadStart;
		Event::PrevPadStart = padStartNow;

		if (keyboard.IsJustPressed(KEY::ESC) || padStartJust) {
			Event::IsPaused = !Event::IsPaused;
		}

		if (!Event::IsPaused) return;

		// ポーズ画面オーバーレイ
		ImVec2 windowSize(400, 260);
		ImVec2 screenCenter(640.0f - windowSize.x * 0.5f, 360.0f - windowSize.y * 0.5f);
		ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Always);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::Begin("##PauseMenu", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

		ImGui::Spacing();
		ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("PAUSED").x) * 0.5f);
		ImGui::TextColored(ImVec4{ 1.0f, 0.85f, 0.2f, 1.0f }, "PAUSED");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		float btnWidth = 200.0f;
		float btnX = (windowSize.x - btnWidth) * 0.5f;

		ImGui::SetCursorPosX(btnX);
		if (ImGui::Button("Resume", ImVec2(btnWidth, 36))) {
			Event::IsPaused = false;
		}

		ImGui::Spacing();
		ImGui::SetCursorPosX(btnX);
		if (ImGui::Button("Restart Area", ImVec2(btnWidth, 36))) {
			Event::IsPaused = false;
			Event::ResetPhase();
			CheckAndLoadArea(playState_.CurrentArea.index);
		}

		ImGui::Spacing();
		ImGui::SetCursorPosX(btnX);
		if (ImGui::Button("Return to Title", ImVec2(btnWidth, 36))) {
			Event::IsPaused = false;
			Event::ResetPhase();
			playState_.IsPlaying = false;
			auto& sceneMngr{ Lumina::SceneManager::Instance() };
			sceneMngr.Deactivate("InGame");
			sceneMngr.Activate("Title");
		}

		ImGui::End();
	}
#endif

	// ==============================
	//  ⑦ 落下時の対処
	// ==============================
	void InGame::HandleFallDeath() {
		if (!Player_ || !playState_.IsPlaying) return;
		auto const& pos = Player_->GetPosition();

		if (pos.Y < Event::FallDeathThresholdY) {
			++Event::FallDeathCount;

			// HPを少し減らす (落下ペナルティ)
			Player_->GetStatusComponent().TakeDamage(10.0f);

			// リスポーン地点へ戻す
			Player_->SetPosition(Event::RespawnPos);
			Player_->externalVelocity_ = { 0.0f, 0.0f, 0.0f };
			Player_->myVelocity_ = { 0.0f, 0.0f, 0.0f };

			// カメラシェイク演出
			Event::CameraShakingTimer = 10;
		}
	}

	// ==============================
	//  ⑤ ゲームフェーズ管理UI
	// ==============================
	void InGame::DrawGamePhaseUI() {
		float dt = 1.0f / 60.0f;

		switch (Event::CurrentPhase) {
		case Event::GamePhase::Startup:
		{
			// カウントダウン演出を廃止し、即座にInBattleへ移行
			Event::CurrentPhase = Event::GamePhase::InBattle;
			Event::PhaseTimer = 0.0f;
			break;
		}
		case Event::GamePhase::InBattle:
		{
			Event::ElapsedBattleTime += dt;

			// 全敵撃破で勝利
			int aliveCount = 0;
			for (const auto& e : playState_.Enemies) {
				if (!e.IsDead) ++aliveCount;
			}
			if (!playState_.Enemies.empty() && aliveCount == 0) {
				Event::CurrentPhase = Event::GamePhase::Win;
				Event::PhaseTimer = 0.0f;
			}

			// プレイヤー死亡で敗北
			if (Player_ && Player_->GetStatusComponent().IsDead()) {
				Event::CurrentPhase = Event::GamePhase::Lose;
				Event::PhaseTimer = 0.0f;
			}
			break;
		}
		case Event::GamePhase::Win:
		{
			Event::PhaseTimer += dt;
			// Area Clear ImGui removed
			break;
		}
		case Event::GamePhase::Lose:
		{
			Event::PhaseTimer += dt;

			// 初回のみメニューを表示
			if (!GameOverMenu_.IsVisible()) {
				GameOverMenu_.Setup(
					{
						// Retry (緑系)
						{ { 0.15f, 0.4f, 0.15f, 0.8f }, { 0.2f, 0.8f, 0.3f, 1.0f } },
						// Return to Title (青系)
						{ { 0.15f, 0.15f, 0.4f, 0.8f }, { 0.3f, 0.3f, 0.9f, 1.0f } }
					},
					{ 0.9f, 0.15f, 0.15f, 0.9f } // タイトル矩形: 赤
				);
				GameOverMenu_.Show();
			}

			int decided = GameOverMenu_.Update(dt);
			if (decided == 0) {
				// Retry
				GameOverMenu_.Hide();
				Event::ResetPhase();
				CheckAndLoadArea(playState_.CurrentArea.index);
			}
			else if (decided == 1) {
				// Return to Title
				GameOverMenu_.Hide();
				Event::ResetPhase();
				playState_.IsPlaying = false;
				auto& sceneMngr{ Lumina::SceneManager::Instance() };
				sceneMngr.Deactivate("InGame");
				sceneMngr.Activate("Title");
			}
			break;
		}
		}
	}

	// ==============================
	//  ⑩ Enemy HP UI (ゲーム画面上のHPバー)
	// ==============================
#if defined(_DEBUG)
	void InGame::DrawEnemyHPBars() {
		if (!playState_.IsPlaying || !Camera_Player_) return;

		auto const& viewProj = *WorldToHomogeneous_;
		float vpW = 1280.0f;
		float vpH = 720.0f;

		// オーバーレイウィンドウ
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(vpW, vpH));
		ImGui::Begin("##EnemyHPOverlay", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		for (const auto& enemy : playState_.Enemies) {
			if (enemy.IsDead) continue;

			// ワールド座標 → NDC → スクリーン座標
			Lumina::Math::F32x4 worldPos{
				enemy.Position.X,
				enemy.Position.Y + 2.0f * enemy.Scale, // 頭の上に表示
				enemy.Position.Z,
				1.0f
			};
			auto clip = worldPos * viewProj;
			if (clip.W() <= 0.0f) continue; // カメラ後方

			float ndcX = clip.X() / clip.W();
			float ndcY = clip.Y() / clip.W();

			float screenX = (ndcX * 0.5f + 0.5f) * vpW;
			float screenY = (1.0f - (ndcY * 0.5f + 0.5f)) * vpH;

			// 画面外なら描画しない
			if (screenX < -50.0f || screenX > vpW + 50.0f ||
				screenY < -50.0f || screenY > vpH + 50.0f) continue;

			float barWidth = 50.0f * enemy.Scale;
			barWidth = (std::max)(30.0f, (std::min)(80.0f, barWidth));
			float barHeight = 6.0f;
			float hpRatio = static_cast<float>(enemy.CurrentHP) /
				static_cast<float>((std::max)(1, enemy.BaseData.hp));
			hpRatio = (std::max)(0.0f, (std::min)(1.0f, hpRatio));

			float left = screenX - barWidth * 0.5f;
			float top = screenY - barHeight;

			// 背景 (暗いグレー)
			drawList->AddRectFilled(
				ImVec2(left - 1, top - 1),
				ImVec2(left + barWidth + 1, top + barHeight + 1),
				MakeCol32(20, 20, 20, 180), 2.0f);

			// HP バー
			ImU32 barColor;
			if (hpRatio > 0.5f)
				barColor = MakeCol32(50, 220, 80, 230);   // 緑
			else if (hpRatio > 0.25f)
				barColor = MakeCol32(240, 200, 40, 230);   // 黄
			else
				barColor = MakeCol32(230, 50, 50, 230);    // 赤

			drawList->AddRectFilled(
				ImVec2(left, top),
				ImVec2(left + barWidth * hpRatio, top + barHeight),
				barColor, 2.0f);
		}

		ImGui::End();
	}
#endif

	void InGame::Update() {
/// dev-Kouda-4.1
        if (playState_.IsBossPresentationActive) {
			playState_.BossPresentationTimer -= 1.0f / 60.0f;
			if (playState_.BossPresentationTimer <= 0.0f) {
				playState_.IsBossPresentationActive = false;
				playState_.BossPresentationTimer = 0.0f;
				playState_.BossPresentationDuration = 0.0f;
			}
		}
///ここまで
		bool tutorialActive = false;
		if (TutorialManager_ && TutorialManager_->IsActive()) {
			tutorialActive = true;
		}

		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		auto const& pad{ inputMngr.Pad() };
		using Lumina::OS::Windows::KEY;

		bool padStartNow = pad.IsHold(0x0010);
		bool padStartJust = padStartNow && !Event::PrevPadStart;
		Event::PrevPadStart = padStartNow;

		if (keyboard.IsJustPressed(KEY::ESC) || padStartJust || keyboard.IsJustPressed(KEY::P)) {
			playState_.IsPaused = !playState_.IsPaused;
			if (playState_.IsPaused) {
				playState_.PauseSelectedIndex = 0;
				playState_.PauseAnimationTimer = 0.0f;
				playState_.PrevPauseUpHeld = true;
				playState_.PrevPauseDownHeld = true;
				playState_.PrevPauseDecideHeld = true;
			}
		}

		if (playState_.IsPaused) {
			playState_.PauseAnimationTimer += 1.0f / 60.0f;
			bool upHeld = keyboard.IsPressed(KEY::W) || keyboard.IsPressed(KEY::ARROW_UP) || pad.IsHold(0x0001);
			bool downHeld = keyboard.IsPressed(KEY::S) || keyboard.IsPressed(KEY::ARROW_DOWN) || pad.IsHold(0x0002);
			
			bool upJust = upHeld && !playState_.PrevPauseUpHeld;
			bool downJust = downHeld && !playState_.PrevPauseDownHeld;
			
			playState_.PrevPauseUpHeld = upHeld;
			playState_.PrevPauseDownHeld = downHeld;

			if (upJust) {
				playState_.PauseSelectedIndex = (playState_.PauseSelectedIndex - 1 + 3) % 3;
			}
			if (downJust) {
				playState_.PauseSelectedIndex = (playState_.PauseSelectedIndex + 1) % 3;
			}

			bool decideHeld = keyboard.IsPressed(KEY::ENTER) || keyboard.IsPressed(KEY::SPACE) || pad.IsHold(0x1000);
			bool decideJust = decideHeld && !playState_.PrevPauseDecideHeld;
			playState_.PrevPauseDecideHeld = decideHeld;

			if (decideJust) {
				if (playState_.PauseSelectedIndex == 0) {
					// Resume
					playState_.IsPaused = false;
				} else if (playState_.PauseSelectedIndex == 1) {
					// Restart from the beginning (Area 0)
					playState_.IsPaused = false;
					Event::ResetPhase();
					CheckAndLoadArea(0);
				} else if (playState_.PauseSelectedIndex == 2) {
					// Title
					playState_.IsPaused = false;
					Event::ResetPhase();
					playState_.IsPlaying = false;
					auto& sceneMngr{ Lumina::SceneManager::Instance() };
					sceneMngr.Deactivate("InGame");
					sceneMngr.Activate("Title");
				}
			}
		}

		// ポーズ中、またはボス登場演出中はゲームロジック更新をスキップ
		if (!playState_.IsPaused && !playState_.IsBossPresentationActive) {
			float deltaTime = 1.0f / 60.0f;
			
			if (Event::HitStopTimer > 0.0f) {
				Event::HitStopTimer -= deltaTime;
				if (Event::HitStopTimer < 0.0f) {
					Event::HitStopTimer = 0.0f;
				}
				deltaTime = 0.0f; // 物理等の進行を停止
			}

			if (deltaTime > 0.0f) {
				Update_<"Player">(); // プレイヤーはチュートリアル中も更新（内部で入力マスクあり）
				
				Update_<"Enemies-1">(1.0f / 60.0f);
				
				Update_<"Collision">(); // 地形との当たり判定のため実行
				
				Update_<"Enemies-2">();
			}
			
			Update_<"[Debug] Area">();
		}

		// ツール系の更新はポーズ等に関わらず実行
		Update_<"[Debug] TerrainEditor">();

		// プレイヤー入力処理を終えた後でチュートリアルを進行させる
		// ポーズ中やボス登場演出中はチュートリアルも進めない
		if (tutorialActive && !playState_.IsPaused && !playState_.IsBossPresentationActive) {
			TutorialManager_->Update(1.0f / 60.0f);
		}

/// dev-Takanaga-temporary
		Update_<"Camera">();
		Update_<"Lighting">();
		Update_<"[Debug] Editor">();
		Update_<"[Debug] Manual">();

		// ⑦ 落下処理 & ⑤ ゲームフェーズUI（Release でも動作）
#if defined(_DEBUG)
		if (activeEditor_ == EditorTab::Play && playState_.IsPlaying) {
#else
		if (playState_.IsPlaying) {
#endif
			HandleFallDeath();
			DrawGamePhaseUI();
		}
	}
}