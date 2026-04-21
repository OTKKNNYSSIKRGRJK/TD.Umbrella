module Game.Scene.InGame;

import : Impl;

namespace Game::Scene {
	void InGame::Update() { Impl_->Update(); }

	void InGame::Render() { Impl_->Render(); }

	template<>
	void InGame::Initialize() {
		Impl_ = std::make_unique<Impl::InGame>();
		Impl_->Initialize();
	}

	InGame::InGame() { Initialize(); }
	InGame::~InGame() = default;
}