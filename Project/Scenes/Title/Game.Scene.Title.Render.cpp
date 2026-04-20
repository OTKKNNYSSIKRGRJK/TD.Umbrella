module Game.Scene.Title;

import : Impl;

import Lumina.Main;

namespace Game::Scene::Impl {
	void Title::Render() {
		// Title シーンの描画は ImGui で行うため、ここでは特に何もしない
		// エンジン側でバックバッファのクリアと ImGui の描画は自動的に行われる
	}
}

namespace Game::Scene {
	void Title::Render() {
		Impl_->Render();
	}
}
