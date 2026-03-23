module Game.Scene.InGame;

import : Impl;

import <cmath>;
import <algorithm>;
import <string>;

import Lumina;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import MotionManager;

#if defined(_DEBUG)
namespace {
	constexpr ImU32 MakeCol32(int r, int g, int b, int a) {
		return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r);
	}
}
#endif

namespace Game::Scene::Impl {
#if defined(_DEBUG)
	void InGame::CheckAndLoadArea(int areaIndex) {
		std::string filename = "area" + std::to_string(areaIndex) + ".json";
		areaEditor_.LoadArea(playState_.CurrentArea, filename);
		
		playState_.Enemies.clear();
		for (auto& ep : playState_.CurrentArea.enemies) {
			PlayEnemy pe;
			enemyEditor_.LoadEnemy(pe.BaseData, ep.enemyName + ".json");
			pe.Position.X = ep.position.x;
			pe.Position.Y = 0.0f; // Y=0 is ground
			pe.Position.Z = 0.0f;
			pe.CurrentHP = pe.BaseData.hp;
			pe.IsDead = false;
			playState_.Enemies.push_back(pe);
		}
		
		// Reset player position when entering area
		playState_.Player.Position.Y = 0.0f; 
		playState_.Player.Position.X = 100.0f; // Start at left side of 2D area
		playState_.Player.Position.Z = 0.0f;
		playState_.Player.Velocity = {0.f, 0.f, 0.f};
		
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
		
		// 2D Jump
		if (keyboard.IsPressed(Lumina::OS::Windows::KEY::SPACE) && playState_.Player.Position.Y <= 0.0f) {
			playState_.Player.Velocity.Y = 600.0f;
		}
		
		// Gravity
		playState_.Player.Velocity.Y -= 1500.0f * dt;
		playState_.Player.Position.Y += playState_.Player.Velocity.Y * dt;
		
		if (playState_.Player.Position.Y <= 0.0f) {
			playState_.Player.Position.Y = 0.0f;
			playState_.Player.Velocity.Y = 0.0f;
		}
		
		// Clamp player inside area bounds
		if (playState_.Player.Position.X < 20.0f) {
			playState_.Player.Position.X = 20.0f;
		}
		if (playState_.Player.Position.X > playState_.CurrentArea.width - 20.0f) {
			playState_.Player.Position.X = playState_.CurrentArea.width - 20.0f;
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
		
		// Enemy Logic (simple track player in 2D)
		for (auto& e : playState_.Enemies) {
			if (e.IsDead) continue;
			
			float dx = playState_.Player.Position.X - e.Position.X;
			float dy = playState_.Player.Position.Y - e.Position.Y;
			float dist = std::sqrt(dx*dx + dy*dy);
			
			if (dist > 50.0f && dist < e.BaseData.aggroRadius * 50.0f) {
				e.Position.X += (dx > 0 ? 1.0f : -1.0f) * e.BaseData.moveSpeed * 60.0f * dt;
			}
			
			if (dist < 50.0f && playState_.Player.HurtTimer <= 0.0f) {
				playState_.Player.HP -= static_cast<int>(e.BaseData.power);
				playState_.Player.HurtTimer = 1.0f; // Invincibility frame
			}
		}
		
		// Area Transition (2D Rect check)
		if (!playState_.IsGoalReached) {
			float px = playState_.Player.Position.X;
			float py = playState_.Player.Position.Y; 
			
			for (const auto& conn : playState_.CurrentArea.connections) {
				// Player bounding box assumes Width=40 [-20~+20], Height=40 [0~40] from base position
				if (px + 20.0f >= conn.trigger.position.x && px - 20.0f <= conn.trigger.position.x + conn.trigger.size.x &&
				    py + 40.0f >= conn.trigger.position.y && py <= conn.trigger.position.y + conn.trigger.size.y) {
					CheckAndLoadArea(conn.targetAreaIndex);
					break;
				}
			}
		}
	}

	void InGame::DrawPlayMode() {
		ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
		ImGui::Begin("Play Mode Debug UI");
		ImGui::Text("Area: %d", playState_.CurrentArea.index);
		ImGui::Text("Player HP: %d / %d", playState_.Player.HP, playState_.Player.MaxHP);
		ImGui::Text("Player Mana: %d", playState_.Player.Mana);
		ImGui::Separator();
		ImGui::Text("--- Enhancements ---");
		if (ImGui::Button("Speed UP (10 Mana)") && playState_.Player.Mana >= 10) {
			playState_.Player.Mana -= 10;
			playState_.PlayerSpeedMultiplier += 0.5f;
		}
		ImGui::Text("Current Speed Mult: %.1f", playState_.PlayerSpeedMultiplier);
		
		if (ImGui::Button("Attack UP (15 Mana)") && playState_.Player.Mana >= 15) {
			playState_.Player.Mana -= 15;
			playState_.PlayerAttackPower += 5.0f;
		}
		ImGui::Text("Current Attack Power: %.1f", playState_.PlayerAttackPower);
		
		if (ImGui::Button("Heal (5 Mana)") && playState_.Player.Mana >= 5) {
			playState_.Player.Mana -= 5;
			playState_.Player.HP = std::min(playState_.Player.MaxHP, playState_.Player.HP + 20);
		}
		ImGui::End();

		// Foreground draw for game world (2D Action Side Scroller)
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		
		// Center camera horizontally on player, Y=0 is explicitly drawn near bottom
		float cx = 1280.0f / 2.0f - playState_.Player.Position.X;
		float groundScreenY = 600.0f; // explicit screen coordinate for floor
		
		auto WorldToScreen = [&](const Lumina::Math::F32x3& p) -> ImVec2 {
			return ImVec2(
				cx + p.X,
				groundScreenY - p.Y
			);
		};
		
		// Draw ground line
		drawList->AddLine(ImVec2(0.0f, groundScreenY), ImVec2(1280.0f, groundScreenY), MakeCol32(255, 255, 255, 255), 2.0f);
		
		// Draw area limits (width)
		drawList->AddRectFilled(ImVec2(cx, groundScreenY - playState_.CurrentArea.height), ImVec2(cx + playState_.CurrentArea.width, groundScreenY), MakeCol32(40, 40, 40, 150));
		drawList->AddLine(ImVec2(cx, groundScreenY - playState_.CurrentArea.height), ImVec2(cx, groundScreenY), MakeCol32(255, 100, 100, 255), 2.0f);
		drawList->AddLine(ImVec2(cx + playState_.CurrentArea.width, groundScreenY - playState_.CurrentArea.height), ImVec2(cx + playState_.CurrentArea.width, groundScreenY), MakeCol32(255, 100, 100, 255), 2.0f);
			
		// Draw triggers (assuming AreaEditor Y is height-based, adjusting as needed)
		for (const auto& conn : playState_.CurrentArea.connections) {
			// In 2D Side Scroller, usually trigger is just an X range and height
			ImVec2 minVec(cx + conn.trigger.position.x, groundScreenY - conn.trigger.position.y - conn.trigger.size.y);
			ImVec2 maxVec(cx + conn.trigger.position.x + conn.trigger.size.x, groundScreenY - conn.trigger.position.y);
			drawList->AddRectFilled(minVec, maxVec, MakeCol32(0, 150, 255, 100));
		}
		
		// Draw grid lines to help feeling horizontal movement
		for (float gx = 0; gx < playState_.CurrentArea.width; gx += 100.0f) {
			drawList->AddLine(ImVec2(cx + gx, groundScreenY), ImVec2(cx + gx, groundScreenY - playState_.CurrentArea.height), MakeCol32(80, 80, 80, 150));
		}
		
		// Draw Enemies
		for (const auto& e : playState_.Enemies) {
			if (e.IsDead) continue;
			ImVec2 sp = WorldToScreen(e.Position);
			
			ImU32 ec = e.HurtTimer > 0.0f ? MakeCol32(255, 255, 255, 255) : MakeCol32(255, 50, 50, 255);
			drawList->AddRectFilled(ImVec2(sp.x - 20, sp.y - 40), ImVec2(sp.x + 20, sp.y), ec);
			// HP bar
			drawList->AddRectFilled(ImVec2(sp.x - 20, sp.y - 50), ImVec2(sp.x - 20 + 40 * ((float)e.CurrentHP / e.BaseData.hp), sp.y - 45), MakeCol32(0, 255, 0, 255));
		}
		
		// Draw Player
		ImVec2 psp = WorldToScreen(playState_.Player.Position);
		ImU32 pc = playState_.Player.HurtTimer > 0.0f ? MakeCol32(255, 255, 255, 150) : MakeCol32(50, 255, 50, 255);
		drawList->AddRectFilled(ImVec2(psp.x - 20, psp.y - 40), ImVec2(psp.x + 20, psp.y), pc);
		
		// Player direction/attack
		if (playState_.PlayerAttackTimer > 0.0f) {
			ImVec2 asp = playState_.Player.FacingRight ? ImVec2(psp.x + 20, psp.y - 20) : ImVec2(psp.x - 20, psp.y - 20);
			drawList->AddCircleFilled(asp, 30.0f, MakeCol32(255, 255, 0, 150));
		}
		
		if (playState_.IsGoalReached) {
			drawList->AddRectFilled(ImVec2(cx + 1280.0f/2.0f - 200.0f, groundScreenY - 300.0f), ImVec2(cx + 1280.0f/2.0f + 200.0f, groundScreenY - 200.0f), MakeCol32(0, 0, 0, 200));
			// Basic rectangle and text for clear
			drawList->AddText(ImVec2(cx + 1280.0f/2.0f - 80.0f, groundScreenY - 260.0f), MakeCol32(255, 255, 0, 255), "AREA 1 REACHED - GOAL!!!");
		}
	}
#endif

	void InGame::Update() {
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
		case EditorTab::Play:
			UpdatePlayLogic();
			DrawPlayMode();
			break;
		default:
			break;
		}
#endif

		Test_.Update();
	}
}

namespace Game::Scene {
	void InGame::Update() {
		Impl_->Update();
	}
}