module Game.Scene.Title2InGame;

import : Impl;

import Lumina.Main;
import Lumina.OS.Windows.RawInput;

namespace Game::Scene::Impl {
	void Title2InGame::Update() {
		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		using Lumina::OS::Windows::KEY;

		if (keyboard.IsJustPressed(KEY::NUM_0)) {
			auto& sceneMngr{ Lumina::SceneManager::Instance() };
			sceneMngr.Unload("Title");
			sceneMngr.Deactivate("Title->InGame");
		}
	}
}

namespace Game::Scene {
	void Title2InGame::Update() {
		Impl_->Update();
	}
}