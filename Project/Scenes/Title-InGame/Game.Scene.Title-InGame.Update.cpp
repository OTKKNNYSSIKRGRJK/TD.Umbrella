module Game.Scene.Title2InGame;

import : Impl;

namespace Game::Scene::Impl {
	void Title2InGame::Update() {
	}
}

namespace Game::Scene {
	void Title2InGame::Update() {
		Impl_->Update();
	}
}