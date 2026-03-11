module Game.Scene.InGame;

import <vector>;

import nlohmann.json;

import Lumina;

import Lumina.Utils.Data;

import : Impl;

namespace Game::Scene::Impl {
	namespace {
	}

	template<>
	void InGame::Initialize(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::ResourceManager const& resMngr_
	) {
		[[maybe_unused]] auto const& device{ d3d12Context_.Device() };
		resMngr_;
	}

	InGame::InGame() = default;
	InGame::~InGame() = default;
}

namespace Game::Scene {
	template<>
	void InGame::Initialize(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::ResourceManager const& resMngr_
	) {
		Impl_ = std::make_unique<Impl::InGame>();
		Impl_->Initialize(d3d12Context_, resMngr_);
	}

	InGame::InGame() = default;
	InGame::~InGame() = default;
}