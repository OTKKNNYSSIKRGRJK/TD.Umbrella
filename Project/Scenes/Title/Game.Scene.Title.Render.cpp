module Game.Scene.Title;

import : Impl;

namespace Game::Scene::Impl {
	void Title::Render() {
	}
}

namespace Game::Scene {
	void Title::Render() {
		Impl_->Render();
	}
}