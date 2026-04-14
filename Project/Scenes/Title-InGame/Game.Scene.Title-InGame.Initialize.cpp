module Game.Scene.Title2InGame;

import nlohmann.json;

//import Lumina;

import Lumina.Utils.Data;
import Lumina.Main;
import Lumina.D3D12;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import : Impl;

namespace Game::Scene::Impl {
	

	void Title2InGame::Initialize() {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };
	}

	Title2InGame::Title2InGame() = default;
	Title2InGame::~Title2InGame() = default;
}

namespace Game::Scene {
	template<>
	void Title2InGame::Initialize() {
		Impl_ = std::make_unique<Impl::Title2InGame>();
		Impl_->Initialize();
	}

	Title2InGame::Title2InGame() { Initialize(); }
	Title2InGame::~Title2InGame() = default;
}