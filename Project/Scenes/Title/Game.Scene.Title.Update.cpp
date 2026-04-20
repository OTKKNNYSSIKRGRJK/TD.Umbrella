module Game.Scene.Title;

import <cmath>;

import : Impl;

import Lumina.Core.Common;
import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.CG3D.Animation;

import Game.MathUtils;

namespace {
	constexpr Lumina::F32 INV_60{ 1.0f / 60.0f };
}

namespace Game::Scene::Impl {
	void Title::Update() {
		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		using Lumina::OS::Windows::KEY;

		/// TODO : accumulate `AnimationTimer_` by actual delta time
		AnimationTimer_ += INV_60;
		/// Makes the animation repeating
		AnimationTimer_ = std::fmod(AnimationTimer_, Animation_.DurationInSeconds);
		Lumina::CG3D::Update(SkinCluster_, Skeleton_, Animation_, AnimationTimer_);

		if (keyboard.IsJustPressed(KEY::NUM_0)) {
			auto& sceneMngr{ Lumina::SceneManager::Instance() };
			sceneMngr.Load<"Title->InGame">();
			sceneMngr.Activate("Title->InGame");
		}
	}
}

namespace Game::Scene {
	void Title::Update() {
		Impl_->Update();
	}
}