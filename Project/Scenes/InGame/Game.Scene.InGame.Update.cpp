module Game.Scene.InGame;

import : Impl;

import <cmath>;
import <algorithm>;
import <string>;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.Utils.Data;
import nlohmann.json;

import Game.MotionManager;
import Game.EnemyManager;
import Game.ProjectileManager;

import Game.Events;

#if defined(_DEBUG)
namespace {
	constexpr ImU32 MakeCol32(int r, int g, int b, int a) {
		return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r);
	}
}
#endif

namespace Game::Scene::Impl {
#if defined(_DEBUG)
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
			TerrainEditor_->SetShapes(*Terrain_);
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
			playState_.Enemies.push_back(pe);

			// EnemyManager側にも生成
			Game::EnemyManager::GetInstance()->SpawnFromData(pe.BaseData, pe.Position, pe.FacingRight, pe.Scale, pe.SizeTier);
		}

		if (Player_) {
			Player_->SetPosition({ playState_.Player.Position.X, playState_.Player.Position.Y, 0.0f });
			Event::RespawnPos = Player_->GetPosition();
		}
		
		playState_.TransitionCooldownTimer = 0.5f; // Add delay
		
		if (areaIndex == 1) {
			playState_.IsGoalReached = true;
		} else {
			playState_.IsGoalReached = false;
		}
	}


	void InGame::DrawPlayMode() {
		ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin("Player Info");
		if (Player_) {
			auto const& pos = Player_->GetPosition();
			ImGui::Text("Player 3D Position: %.2f, %.2f, %.2f", pos.X, pos.Y, pos.Z);
		}
		ImGui::End();
	}
#endif

	void InGame::Update() {
		Player_->Update(1.0f / 60.0f);

		playState_.Player.Position.X = Player_->GetPosition().X;
		playState_.Player.Position.Y = Player_->GetPosition().Y;
		playState_.Player.Position.Z = Player_->GetPosition().Z;

		Game::EnemyManager::GetInstance()->Update(1.0f / 60.0f, Player_->GetPosition());

		// プロジェクタイル更新
		Game::ProjectileManager::GetInstance()->Update(1.0f / 60.0f, Player_->GetPosition());

		// Collision の更新処理↓↓↓
		
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
			pe.FacingRight = inst.facingRight;
			pe.SizeTier = inst.sizeTier;
			pe.Scale = inst.modelScale;
			playState_.Enemies.push_back(std::move(pe));
		}

		//TerrainEditor_->Update();

		#if defined(_DEBUG)
		// エリアの移動処理
		if (activeEditor_ == EditorTab::Play && playState_.IsPlaying) {
			if (playState_.TransitionCooldownTimer > 0.0f) {
				playState_.TransitionCooldownTimer -= 1.0f / 60.0f;
			} else {
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
		Lumina::Math::F32x3 newCameraPos{
			cameraPos.X * 0.95f + playerPos.X * 0.05f,
			cameraPos.Y * 0.95f + playerPos.Y * 0.05f,
			-30.0f
		};
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

		if (!isUsingDebugCamera) {
			*WorldToHomogeneous_ = Camera_Player_->View() * Camera_->Projection();
		}
		else {
			*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
		}

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
				ImGui::Text("Enemy[%d] HP: %d / %d %s",
					static_cast<int>(i),
					enemy.CurrentHP,
					enemy.BaseData.hp,
					enemy.IsDead ? "(Dead)" : "");
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

		ImGui::Begin("Manual");

		auto const& pad = inputMngr.Pad();
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
	}
}

namespace Game::Scene {
	void InGame::Update() {
		Impl_->Update();
	}
}