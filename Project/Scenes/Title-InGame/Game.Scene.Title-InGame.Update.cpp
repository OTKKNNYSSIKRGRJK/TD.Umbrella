module Game.Scene.Title2InGame;

import : Impl;

import Lumina.Main;
import Lumina.OS.Windows.RawInput;

namespace Game::Scene::Impl {
	void Title2InGame::Update() {
		//auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		//auto const& keyboard{ inputMngr.Keyboard() };
		//using Lumina::OS::Windows::KEY;

		static int cnt = 0;

		if (cnt > 3) {
			auto& sceneMngr{ Lumina::SceneManager::Instance() };
			sceneMngr.Deactivate("Title");
			sceneMngr.Deactivate("Title->InGame");

			cnt = 0;
		}
		else {
			++cnt;
		}
	}
}

namespace Game::Scene {
	void Title2InGame::Update() {
		Impl_->Update();
	}
}