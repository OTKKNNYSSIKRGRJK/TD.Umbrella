module Game.Scene.InGame;

import : Impl;

import Lumina;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import MotionManager;

namespace Game::Scene::Impl {
	void InGame::Update() {
#if defined(_DEBUG)
		// メインメニューバー: エディタ切り替え
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Editor")) {
				if (ImGui::MenuItem("Motion Editor", nullptr, activeEditor_ == EditorTab::Motion)) {
					activeEditor_ = EditorTab::Motion;
				}
				if (ImGui::MenuItem("Area Editor", nullptr, activeEditor_ == EditorTab::Area)) {
					activeEditor_ = EditorTab::Area;
				}
				if (ImGui::MenuItem("Enemy Editor", nullptr, activeEditor_ == EditorTab::Enemy)) {
					activeEditor_ = EditorTab::Enemy;
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