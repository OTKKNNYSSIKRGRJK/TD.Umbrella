module Game.Scene.InGame;

import <vector>;

import nlohmann.json;

import Lumina;

import Lumina.Utils.Data;

import : Impl;

import MotionManager;

namespace Game::Scene::Impl {
	namespace {
	}

	template<>
	void InGame::Initialize() {
		MotionManager::GetInstance()->LoadMotions("Assets/Data/Motion/");
		Test_.Initialize();
	}

	InGame::InGame() = default;
	InGame::~InGame() = default;
}

namespace Game::Scene {
	template<>
	void InGame::Initialize() {
		Impl_ = std::make_unique<Impl::InGame>();
		Impl_->Initialize();
	}

	InGame::InGame() { Initialize(); }
	InGame::~InGame() = default;
}