module Game.Scene.InGame;

import : Impl;

import <cmath>;
import <algorithm>;
import <string>;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import Lumina.Main;
import Lumina.Utils.Data;
import nlohmann.json;

import Game.MotionManager;
import Game.EnemyManager;

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
				Lumina::Utils::LoadFromFile<nlohmann::json>(filename, "./")
			);
			
			Terrain_ = std::make_unique<TerrainShapeCollection>();
			TerrainScreenData_->ConvertToWorldCoordinate(
				*Terrain_,
				*Camera_,
				{ 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f }
			);
		} catch (...) {
			// Fallback or empty terrain if file has no terrain data yet
			TerrainScreenData_ = std::make_unique<TerrainShapeCollection>();
			Terrain_ = std::make_unique<TerrainShapeCollection>();
		}
		
		playState_.Enemies.clear();
		Game::EnemyManager::GetInstance()->ClearInstances();

		for (auto& ep : playState_.CurrentArea.enemies) {
			PlayEnemy pe;
			enemyEditor_.LoadEnemy(pe.BaseData, ep.enemyName + ".json");
			pe.Position.X = ep.position.x;
			pe.Position.Y = 0.0f; // Y=0 is ground
			pe.Position.Z = 0.0f;
			pe.BaseData.hp = 50; // Force normal enemies to 50 HP
			pe.CurrentHP = pe.BaseData.hp;
			pe.IsDead = false;
			pe.FacingRight = ep.facingRight;
			playState_.Enemies.push_back(pe);

			// EnemyManager側にも生成
			Game::EnemyManager::GetInstance()->SpawnFromData(pe.BaseData, pe.Position, pe.FacingRight);
		}
		
		// Reset player position when entering area
		playState_.Player.Position.Y = 0.0f;
		playState_.Player.Position.Z = 0.0f;
		playState_.Player.Velocity = {0.f, 0.f, 0.f};

		// Spawn location logic
		bool spawnedAtConnection = false;
		if (previousAreaIndex != -1) {
			for (const auto& conn : playState_.CurrentArea.connections) {
				if (conn.targetAreaIndex == previousAreaIndex && !(areaIndex == 0 && previousAreaIndex == 0)) {
					// Spawn at the center of the connection linking back to where we came from
					playState_.Player.Position.X = conn.trigger.position.x + conn.trigger.size.x / 2.0f;
					playState_.Player.Position.Y = conn.trigger.position.y;
					spawnedAtConnection = true;
					break;
				}
			}
		}

		if (!spawnedAtConnection && areaIndex == 0) {
			for (const auto& conn : playState_.CurrentArea.connections) {
				if (conn.targetAreaIndex == 0) {
					playState_.Player.Position.X = conn.trigger.position.x + conn.trigger.size.x / 2.0f;
					playState_.Player.Position.Y = conn.trigger.position.y;
					spawnedAtConnection = true;
					break;
				}
			}
		}

		if (!spawnedAtConnection) {
			playState_.Player.Position.X = 100.0f; // Fallback / Start location
		}

		if (Player_ && Camera_) {
			float start2DX = playState_.Player.Position.X;
			float rawScreenY = playState_.CurrentArea.height - playState_.Player.Position.Y; 

			auto const worldToHomogeneous_c = Camera_->View() * Camera_->Projection();
			auto tmp{ Lumina::Math::F32x4{ 0.0f, 0.0f, 0.0f, 1.0f } * worldToHomogeneous_c };
			tmp /= tmp.W();

			Lumina::F32 const inv_ViewportWidth{ 1.0f / 1280.0f };
			Lumina::F32 const inv_ViewportHeight{ 1.0f / 720.0f };
			
			auto const& inv_View{ Camera_->ViewInverse() };
			auto const inv_Proj{ Camera_->Projection().Inverse() };
			auto const ndcToWorld{ inv_Proj * inv_View };

			Lumina::Math::F32x4 ndcPos{
				(start2DX * inv_ViewportWidth) * 2.0f - 1.0f,
				1.0f - (rawScreenY * inv_ViewportHeight) * 2.0f,
				tmp.Z(),
				1.0f
			};
			auto worldPos = ndcPos * ndcToWorld;
			worldPos /= worldPos.W();

			Player_->SetPosition({ worldPos.X(), worldPos.Y(), 0.0f });
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
		MotionEditor::GetInstance()->NodeImGui();
		//TerrainEditor_->Update();
		Player_->Update(1.0f / 60.0f);
		Game::EnemyManager::GetInstance()->Update(1.0f / 60.0f, Player_->GetPosition());

		// Collision の更新処理↓↓↓
		
		// 中身をclear
		CollisionManager_->Begin();

		// ここからColliderを設定
		CollisionManager_->SetColliders(Player_->GetCollider());
		CollisionManager_->SetColliders(Player_->GetUmbrella().top_->GetCollider());
		auto const& groundColliders = Terrain_->GroundData().Colliders;
		for (auto const& col : groundColliders) {
			CollisionManager_->SetColliders(col.get());
		}

		Game::EnemyManager::GetInstance()->RegisterCollidersTo(*CollisionManager_);

		// Check!
		CollisionManager_->CheckAllCollisions();

		// Sync positions from EnemyManager back to playState_.Enemies for Render
		const auto& enemyInstances = Game::EnemyManager::GetInstance()->GetAllInstances();
		for (size_t i = 0; i < enemyInstances.size() && i < playState_.Enemies.size(); ++i) {
			playState_.Enemies[i].Position = enemyInstances[i].position;
			playState_.Enemies[i].FacingRight = enemyInstances[i].facingRight;
			playState_.Enemies[i].IsDead = enemyInstances[i].isDead;
			if(enemyInstances[i].isDead) { // 死亡していたら同期して表示を消すように
				playState_.Enemies[i].CurrentHP = 0;
			}
		}

	void InGame::Update() {
		MotionEditor::GetInstance()->NodeImGui();
		TerrainEditor_->Update();
		Player_->Update(1.0f);

		#if defined(_DEBUG)
		ImGui::Begin("Camera");
		static Lumina::Math::F32x3 eye{ 0.0f, 5.0f, -30.0f };
		static Lumina::Math::F32x3 target{ 0.0f, 5.0f, 0.0f };
		ImGui::DragFloat3("Eye", &eye.X, 0.1f);
		ImGui::DragFloat3("Target", &target.X, 0.1f);
		Camera_->LookAt(eye, target, { 0.0f, 1.0f, 0.0f });
		ImGui::End();
		#endif

		*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();

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
				if (ImGui::MenuItem("Actor Editor", nullptr, activeEditor_ == EditorTab::Actor)) {
					activeEditor_ = EditorTab::Actor;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Terrain Editor", nullptr, activeEditor_ == EditorTab::Terrain)) {
					activeEditor_ = EditorTab::Terrain;
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
		case EditorTab::Actor:
			actorEditor_.Update();
			break;
		case EditorTab::Terrain:
			if (TerrainEditor_) TerrainEditor_->Update();
			break;
		case EditorTab::Play:
			//UpdatePlayLogic();
			DrawPlayMode();
			break;
		default:
			break;
		}
		#endif
	}
}

namespace Game::Scene {
	void InGame::Update() {
		Impl_->Update();
	}
}