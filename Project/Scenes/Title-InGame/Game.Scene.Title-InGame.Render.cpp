module Game.Scene.Title2InGame;

import : Impl;

namespace Game::Scene::Impl {
	void Title2InGame::Render() {
	}
}

namespace Game::Scene {
	void Title2InGame::Render() {
		Impl_->Render();
	}
}