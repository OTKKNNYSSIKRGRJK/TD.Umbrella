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

		// Pキーでポーズトグル
		{
			auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
			auto const& keyboard{ inputMngr.Keyboard() };
			using Lumina::OS::Windows::KEY;
			if (keyboard.IsJustPressed(KEY::P)) {
				playState_.IsPaused = !playState_.IsPaused;
			}
		}

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
			ImGui::SetNextWindowPos(ImVec2(10, 140), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(320, 220), ImGuiCond_FirstUseEver);
			ImGui::Begin("Enemy HP");
			for (size_t i = 0; i < playState_.Enemies.size(); ++i) {
				const auto& enemy = playState_.Enemies[i];
			ImGui::Text("Enemy[%d] HP: %d / %d %s  Walk:%s Motion:%s Node:%d",
				static_cast<int>(i),
				enemy.CurrentHP,
				enemy.BaseData.hp,
				enemy.IsDead ? "(Dead)" : "",
				enemy.WalkActive ? "true" : "false",
				enemy.MotionPlaying ? "playing" : "stopped",
				enemy.ActiveNodeIndex);
			}
			ImGui::End();

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

		// ポーズ中、またはボス登場演出中はゲームロジック更新をスキップ
		if (!playState_.IsPaused && !playState_.IsBossPresentationActive) {
			Update_<"Player">(); // プレイヤーはチュートリアル中も更新（内部で入力マスクあり）
			
			Update_<"Enemies-1">(1.0f / 60.0f);
			
			Update_<"Collision">(); // 地形との当たり判定のため実行
			
			Update_<"Enemies-2">();
			
			Update_<"[Debug] Area">();
		}

		// ツール系の更新はポーズ等に関わらず実行
		Update_<"[Debug] TerrainEditor">();

		// プレイヤー入力処理を終えた後でチュートリアルを進行させる
		if (tutorialActive) {
			TutorialManager_->Update(1.0f / 60.0f);
		}

/// dev-Takanaga-temporary
		Update_<"Camera">();
		Update_<"Lighting">();
		Update_<"[Debug] Editor">();
		Update_<"[Debug] Manual">();
	}
}