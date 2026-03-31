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

		ImGui::Begin("Camera");
		static Lumina::Math::F32x3 eye{ 0.0f, 0.0f, -30.0f };
		static Lumina::Math::F32x3 target{ 0.0f, 0.0f, 0.0f };
		ImGui::DragFloat3("Eye", &eye.X, 0.1f);
		ImGui::DragFloat3("Target", &target.X, 0.1f);
		Camera_->LookAt(eye, target, { 0.0f, 1.0f, 0.0f });
		ImGui::End();

		*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
	}
}

namespace Game::Scene {
	void InGame::Update() {
		Impl_->Update();
	}
}