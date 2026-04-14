module Game.Scene.Title;

import : Impl;

namespace Game::Scene::Impl {
	void Title::Update() {
	}
}

namespace Game::Scene {
	void Title::Update() {
		Impl_->Update();
	}
}