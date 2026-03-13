module Game.Scene.InGame;

import : Impl;

import Lumina;
import Lumina.Utils.ImGui;

import MotionManager;

namespace Game::Scene::Impl {
	void InGame::Update() {
		ImGui::Begin("Test");
		ImGui::Text("12345");
		ImGui::End();

		MotionEditor::GetInstance()->NodeImGui();
	}
}

namespace Game::Scene {
	void InGame::Update() {
		Impl_->Update();
	}
}