module Game.Scene.Result;

import Lumina.Main;

import : Impl;

namespace Game::Scene::Impl {
	void Result::Initialize() {
		// Result シーンは ImGui のみで表示するため、GPU リソースの初期化は不要
	}

	Result::Result() = default;
	Result::~Result() = default;
}

namespace Game::Scene {
	template<>
	void Result::Initialize() {
		Impl_ = std::make_unique<Impl::Result>();
		Impl_->Initialize();
	}

	Result::Result() { Initialize(); }
	Result::~Result() = default;
}
