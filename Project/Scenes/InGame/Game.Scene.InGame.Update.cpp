module Game.Scene.InGame;

import : Impl;

//import Lumina;
import Lumina.Utils.ImGui;

import Game.MotionManager;

namespace Game::Scene::Impl {
	void InGame::Update() {
		MotionEditor::GetInstance()->NodeImGui();
		//TerrainEditor_->Update();
		Player_->Update(1.0f);
	}
}

namespace Game::Scene {
	void InGame::Update() {
		Impl_->Update();
	}
}