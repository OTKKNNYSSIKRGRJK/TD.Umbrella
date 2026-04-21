module Game.Scene.Title;

import <cmath>;

import : Impl;

import Lumina.Core.Common;
import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.CG3D.Animation;

import Lumina.Utils.ImGui;

import Game.MathUtils;

namespace {
	constexpr Lumina::F32 INV_60{ 1.0f / 60.0f };
}

namespace Game::Scene::Impl {
	template<>
	auto Title::Update_<"Camera">() -> void {
		#if defined(_DEBUG)
		ImGui::Begin("Title::Camera");
		static Lumina::Math::F32x3 eye{ 5.0f, 1.5f, 5.0f };
		static Lumina::Math::F32x3 target{ 0.0f, 1.5f, 0.0f };
		ImGui::DragFloat3("Eye", &eye.X, 0.1f);
		ImGui::DragFloat3("Target", &target.X, 0.1f);
		Camera_->LookAt(eye, target, { 0.0f, 1.0f, 0.0f });
		ImGui::End();
		#endif

		*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
	}

	void Title::Update() {
		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		using Lumina::OS::Windows::KEY;

		Update_<"Camera">();

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