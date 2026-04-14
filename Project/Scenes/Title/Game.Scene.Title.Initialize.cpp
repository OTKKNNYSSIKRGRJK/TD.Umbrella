module Game.Scene.Title;

import nlohmann.json;

//import Lumina;

import Lumina.Utils.Data;
import Lumina.Main;
import Lumina.D3D12;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import : Impl;

import Game.MotionManager;
import Game.Player;

namespace Game::Scene::Impl {
	

	void Title::Initialize() {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };
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