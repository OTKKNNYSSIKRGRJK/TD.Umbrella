module Game.Scene.Title;

import : Impl;

import Lumina.Main;
import Lumina.OS.Windows.RawInput;

namespace Game::Scene::Impl {
	void Title::Update() {
		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		using Lumina::OS::Windows::KEY;

		if (keyboard.IsJustPressed(KEY::NUM_0)) {
			auto& sceneMngr{ Lumina::SceneManager::Instance() };
			sceneMngr.Load<"Title->InGame">();
			sceneMngr.Activate("Title->InGame");
		}
	}
}

namespace Game::Scene {
	void Title::Update() {
		Impl_->Update();
	}
}