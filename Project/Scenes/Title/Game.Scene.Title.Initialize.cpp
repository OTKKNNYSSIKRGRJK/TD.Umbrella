module Game.Scene.Title;

import Lumina.Main;

import : Impl;

namespace Game::Scene::Impl {
	void Title::Initialize() {
		// Title シーンは ImGui のみで表示するため、GPU リソースの初期化は不要
	}

	Title::Title() = default;
	Title::~Title() = default;
}

namespace Game::Scene {
	template<>
	void Title::Initialize() {
		Impl_ = std::make_unique<Impl::Title>();
		Impl_->Initialize();
	}

	Title::Title() { Initialize(); }
	Title::~Title() = default;
}
