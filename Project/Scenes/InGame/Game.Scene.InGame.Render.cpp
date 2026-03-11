module Game.Scene.InGame;

import : Impl;

import Lumina;

namespace Game::Scene::Impl {
	void InGame::Render() {
	}
}

namespace Game::Scene {
	void InGame::Render() {
		Impl_->Render();
	}
}