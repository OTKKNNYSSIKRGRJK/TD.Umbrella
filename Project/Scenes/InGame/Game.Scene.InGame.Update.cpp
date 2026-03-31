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
		}
		
		// Reset player position when entering area
		playState_.Player.Position.Y = 0.0f;
		playState_.Player.Position.Z = 0.0f;
		playState_.Player.Velocity = {0.f, 0.f, 0.f};

		// Spawn location logic
		bool spawnedAtConnection = false;
		if (previousAreaIndex != -1) {
			for (const auto& conn : playState_.CurrentArea.connections) {
				if (conn.targetAreaIndex == previousAreaIndex) {
					// Spawn at the center of the connection linking back to where we came from
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
		
		playState_.TransitionCooldownTimer = 0.5f; // Add delay
		
		if (areaIndex == 1) {
			playState_.IsGoalReached = true;
		} else {
			playState_.IsGoalReached = false;
		}
	}

	void InGame::UpdatePlayLogic() {
		if (!playState_.IsPlaying) return;
		
		auto const& keyboard = Lumina::Context::Instance().RawInputContext().Keyboard();
		
		// Physics & Movement
		float dt = 1.0f / 60.0f;
		float speed = 300.0f * playState_.PlayerSpeedMultiplier * dt;
		
		if (keyboard.IsPressed(Lumina::OS::Windows::KEY::A)) {
			playState_.Player.Position.X -= speed;
			playState_.Player.FacingRight = false;
		}
		if (keyboard.IsPressed(Lumina::OS::Windows::KEY::D)) {
			playState_.Player.Position.X += speed;
			playState_.Player.FacingRight = true;
		}
		
		auto getTerrainY = [&](float px) -> float {
			float yFromTop = static_cast<float>(playState_.CurrentArea.height);
			if (TerrainScreenData_) {
				auto& vertices = TerrainScreenData_->GroundData().Vertices;
				bool foundMapGround = false;
				
				Lumina::List<Game::Ground::Vertex>::Iterator it{ vertices };
				for (it.Begin(); !it.End(); it.Next()) {
					auto const& v1 = *it;
					if (v1.NextID != -1) {
						auto const* v2_ptr = (Game::Ground::Vertex const*)nullptr;
						
						Lumina::List<Game::Ground::Vertex>::Iterator jt{ vertices };
						for (jt.Begin(); !jt.End(); jt.Next()) {
							if ((*jt).ID == v1.NextID) { v2_ptr = &(*jt); break; }
						}
						
						if (v2_ptr) {
							float x1 = v1.Pos.X; float y1 = v1.Pos.Y;
							float x2 = v2_ptr->Pos.X; float y2 = v2_ptr->Pos.Y;
							if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
							
							if (px >= x1 && px <= x2) {
								if (x2 - x1 > 0.001f) {
									float t = (px - x1) / (x2 - x1);
									yFromTop = y1 + t * (y2 - y1);
								} else {
									yFromTop = y1;
								}
								foundMapGround = true;
								break;
							}
						}
					}
				}
				if (!foundMapGround && vertices.Size() > 0) {
					float minX = 999999.0f, maxX = -999999.0f;
					float yAtMinX = 0, yAtMaxX = 0;
					
					Lumina::List<Game::Ground::Vertex>::Iterator it2{ vertices };
					for (it2.Begin(); !it2.End(); it2.Next()) {
						if ((*it2).Pos.X < minX) { minX = (*it2).Pos.X; yAtMinX = (*it2).Pos.Y; }
						if ((*it2).Pos.X > maxX) { maxX = (*it2).Pos.X; yAtMaxX = (*it2).Pos.Y; }
					}
					if (px <= minX) yFromTop = yAtMinX;
					else if (px >= maxX) yFromTop = yAtMaxX;
				}
			}
			return static_cast<float>(playState_.CurrentArea.height) - yFromTop;
		};

		// Clamp player inside area bounds (X)
		if (playState_.Player.Position.X < 20.0f) {
			playState_.Player.Position.X = 20.0f;
		}
		if (playState_.Player.Position.X > playState_.CurrentArea.width - 20.0f) {
			playState_.Player.Position.X = playState_.CurrentArea.width - 20.0f;
		}

		float groundY = getTerrainY(playState_.Player.Position.X);
		
		// 2D Jump
		if (keyboard.IsPressed(Lumina::OS::Windows::KEY::SPACE) && std::abs(playState_.Player.Position.Y - groundY) <= 0.1f) {
			playState_.Player.Velocity.Y = 600.0f;
		}
		
		// Gravity
		playState_.Player.Velocity.Y -= 1500.0f * dt;
		playState_.Player.Position.Y += playState_.Player.Velocity.Y * dt;
		
		if (playState_.Player.Position.Y <= groundY) {
			playState_.Player.Position.Y = groundY;
			playState_.Player.Velocity.Y = 0.0f;
		}
		
		// Attack
		if (playState_.PlayerAttackTimer > 0.0f) {
			playState_.PlayerAttackTimer -= dt;
		}
		
		if (keyboard.IsPressed(Lumina::OS::Windows::KEY::ENTER) && playState_.PlayerAttackTimer <= 0.0f) {
			playState_.PlayerAttackTimer = 0.3f; // Cooldown
			
			// Hit detection (2D Horizontal + Vertical distance)
			for (auto& e : playState_.Enemies) {
				if (e.IsDead) continue;
				float dx = e.Position.X - playState_.Player.Position.X;
				float dy = e.Position.Y - playState_.Player.Position.Y;
				float dist = std::sqrt(dx*dx + dy*dy);
				
				// Facing check and distance
				if (dist < 150.0f) {
					if ((playState_.Player.FacingRight && dx >= -50.0f) || (!playState_.Player.FacingRight && dx <= 50.0f)) {
						e.CurrentHP -= static_cast<int>(playState_.PlayerAttackPower);
						e.HurtTimer = 0.2f;
						if (e.CurrentHP <= 0) {
							e.IsDead = true;
							playState_.Player.Mana += 10; // Gain Mana
						}
					}
				}
			}
		}
		
		if (playState_.Player.HurtTimer > 0.0f) playState_.Player.HurtTimer -= dt;
		for (auto& e : playState_.Enemies) {
			if (e.HurtTimer > 0.0f) e.HurtTimer -= dt;
		}
		
		// Mana Drain Over Time (lose 1 Mana every 0.5 seconds -> 2 Mana/sec)
		if (playState_.Player.Mana > 0) {
			playState_.Player.ManaTimer += dt;
			if (playState_.Player.ManaTimer >= 0.5f) {
				playState_.Player.ManaTimer -= 0.5f;
				playState_.Player.Mana -= 1;
			}
		} else {
			playState_.Player.ManaTimer = 0.0f;
		}
		
		// Buff Timers
		if (playState_.BuffSpeedTimer > 0.0f) {
			playState_.BuffSpeedTimer -= dt;
			if (playState_.BuffSpeedTimer <= 0.0f) {
				playState_.PlayerSpeedMultiplier = 1.0f;
			}
		}
		if (playState_.BuffAttackTimer > 0.0f) {
			playState_.BuffAttackTimer -= dt;
			if (playState_.BuffAttackTimer <= 0.0f) {
				playState_.PlayerAttackPower = 10.0f;
			}
		}
		
		if (playState_.TransitionCooldownTimer > 0.0f) {
			playState_.TransitionCooldownTimer -= dt;
		}
		
		// Enemy Logic (simple track player in 2D)
		for (auto& e : playState_.Enemies) {
			if (e.IsDead) continue;
			
			// Puppet Auto-Regen
			if (e.BaseData.name == "Puppet") {
				if (e.HurtTimer <= 0.0f && e.CurrentHP < e.BaseData.hp) {
					e.CurrentHP += 10; // Extremely high regeneration (600 HP / sec)
					if (e.CurrentHP > e.BaseData.hp) e.CurrentHP = e.BaseData.hp;
				}
				continue; // Puppets don't move or attack
			}
			
			float dx = playState_.Player.Position.X - e.Position.X;
			float dy = playState_.Player.Position.Y - e.Position.Y;
			float dist = std::sqrt(dx*dx + dy*dy);
			
			if (dist > 50.0f && dist < e.BaseData.aggroRadius * 50.0f) {
				e.Position.X += (dx > 0 ? 1.0f : -1.0f) * e.BaseData.moveSpeed * 60.0f * dt;
			}
			
			e.Position.Y = getTerrainY(e.Position.X);

			if (dist < 50.0f && playState_.Player.HurtTimer <= 0.0f) {
				playState_.Player.HP -= static_cast<int>(e.BaseData.power);
				playState_.Player.HurtTimer = 1.0f; // Invincibility frame
			}
		}
		
		// Area Transition (2D Rect check with W key)
		if (!playState_.IsGoalReached && playState_.TransitionCooldownTimer <= 0.0f && keyboard.IsPressed(Lumina::OS::Windows::KEY::W)) {
			float px = playState_.Player.Position.X;
			float py = playState_.Player.Position.Y; 
			
			for (const auto& conn : playState_.CurrentArea.connections) {
				// Player bounding box assumes Width=40 [-20~+20], Height=40 [0~40] from base position
				if (px + 20.0f >= conn.trigger.position.x && px - 20.0f <= conn.trigger.position.x + conn.trigger.size.x &&
				    py + 40.0f >= conn.trigger.position.y && py <= conn.trigger.position.y + conn.trigger.size.y) {
					CheckAndLoadArea(conn.targetAreaIndex, playState_.CurrentArea.index);
					break;
				}
			}
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
		// Check!
		CollisionManager_->CheckAllCollisions();

		// Collisionの更新処理↑↑↑

		#if defined(_DEBUG)
		ImGui::Begin("Camera");
		static Lumina::Math::F32x3 eye{ 0.0f, 0.0f, -30.0f };
		static Lumina::Math::F32x3 target{ 0.0f, 0.0f, 0.0f };
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
			UpdatePlayLogic();
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