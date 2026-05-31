module;

#include<Windows.h>

module Game.Scene.Title;

import <cmath>;
import <numbers>;

import : Impl;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.CG3D.Animation;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif
import Lumina.Utils.Color;

import Game.MathUtils;
import Game.BGMManager;

namespace {
	constexpr Lumina::F32 INV_60{ 1.0f / 60.0f };
	constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };

	static Lumina::Math::F32x4x4<> const INV_Viewport{
		1.0f / 640.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f / 360.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
	};
}

namespace Game::Scene::Impl {
	template<>
	auto Title::Update_<"Umbrella">() -> void {
		static Lumina::F32 time{ 0.0f };

		UmbrellaRotation_.X = std::sin(time * 0.4f) * 0.25f;
		UmbrellaRotation_.Y = time * 0.3f;
		UmbrellaRotation_.Z = std::sin(time * 0.2f) * 0.25f;

		RootWorldPos_.Y = 2.0f + std::sin(time) * 0.25f;

		static Lumina::Math::F32x4x4<> const rootToTip{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 1.89f, 0.0f, 1.0f,
		};

		time += 0.05f;

		*UmbrellaRootWorld_ = Game::MathUtils::SRT(
			{ 1.0f, 1.0f, 1.0f },
			UmbrellaRotation_,
			RootWorldPos_
		);
		*UmbrellaTipWorld_ = rootToTip * (*UmbrellaRootWorld_);
	}

	template<>
	auto Title::Update_<"Camera">() -> void {
		static Lumina::Math::F32x3 eye{ 7.5f, 4.0f, 6.0f };
		static Lumina::Math::F32x3 target{ 0.0f, 3.5f, 0.0f };
		#if defined(_DEBUG)
		ImGui::Begin("Title::Camera");
		ImGui::DragFloat3("Eye", &eye.X, 0.1f);
		ImGui::DragFloat3("Target", &target.X, 0.1f);
		ImGui::End();
		#endif

		static float time = 0.0f;
		time += 0.01f;
		eye.X += std::sin(time) * 0.01f;
		eye.Y += std::sin(time * 1.2f) * 0.005f;

		Camera_->LookAt(eye, target, { 0.0f, 1.0f, 0.0f });
		*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
		*ScreenToWorld_ = INV_Viewport * WorldToHomogeneous_->Inverse();
	}

	template<>
	auto Title::Update_<"UI">() -> void {
		constexpr static int timer{ 72 };
		constexpr static float inv_Timer{ 1.0f / static_cast<float>(timer) };

		++UITimer2_;
		if (UITimer2_ >= timer) {
			if (UITimer_ < 3) {
				++UITimer_;
			}
			UITimer2_ = 0;
		}

		float t = inv_Timer * UITimer2_;
		float const easedT = (t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f));

		if (UITimer_ == 1) {
			auto buttonFadeIn{
				[&, this](Lumina::Sprite& button_, int buttonID_) -> void {
					button_.RGBA.W = ((SelectedButton_ == buttonID_) ? (0.5f) : (0.0625f)) * easedT;
					button_.Translate.X = 1400.0f + (-easedT) * 200.0f;
					button_.Translate.X -= !!(SelectedButton_ == buttonID_) * 10.0f;
				}
			};
			auto titleFadeIn{
				[&, this]() -> void {
					TitleCaption_.RGBA.W = 0.85f * easedT;
					TitleCaption_.Translate.Y = -100.0f + easedT * 100.0f;
				}
			};
			buttonFadeIn(UI_StartButton_, 0);
			buttonFadeIn(UI_ExitButton_, 1);

			titleFadeIn();
		}
		else if (UITimer_ > 1) {
			auto setTranslate{
				[this] (Lumina::Sprite& button_, int buttonID_) -> void {
					button_.Translate.X += (SelectedButton_ == buttonID_) ? (-0.5f) : (0.5f);
					button_.Translate.X = std::clamp<float>(button_.Translate.X, 1190.0f, 1200.0f);
				}
			};
			auto setColor{
				[this] (Lumina::Sprite& button_, int buttonID_) -> void {
					button_.RGBA.W *= (SelectedButton_ == buttonID_) ? (1.05f) : (0.95f);
					button_.RGBA.W = std::clamp<float>(button_.RGBA.W, 0.0625f, 0.75f);
				}
			};
			setTranslate(UI_StartButton_, 0);
			setTranslate(UI_ExitButton_, 1);
			setColor(UI_StartButton_, 0);
			setColor(UI_ExitButton_, 1);
		}
	}

	void Title::Update() {
		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		using Lumina::OS::Windows::KEY;
		
		Update_<"Umbrella">();
		Update_<"Camera">();

		Update_<"Effect.Umbrella.Perpetual">();
		Update_<"Effect.Ambient.Raindrops">();
		Update_<"Effect.Ambient.Sparkle">();

		Update_<"Lighting">();

		Update_<"UI">();

        // タイトル画面でスペースキーまたはXBOXのAボタンが押されたらゲーム開始
		// XBOX Aボタンは GamePad のボタンマスク 0x1000（GamePadButton::A）を使用
        if (keyboard.IsJustPressed(KEY::SPACE) || inputMngr.Pad().IsPressed(0x1000)) {
			if (SelectedButton_ == 0) {
				auto& sceneMngr{ Lumina::SceneManager::Instance() };
				sceneMngr.Unload("Title->InGame");
				sceneMngr.Load<"Title->InGame">();
				sceneMngr.Activate("Title->InGame");
				Game::BGMManager::GetInstance()->PlaySceneBGM("InGame");
			}
			else if (SelectedButton_ == 1) {
				auto const& winAppContext{ Lumina::Context::Instance().WinAppContext() };
				::SendMessage(winAppContext.WindowInstance(L"Main").Handle(), WM_CLOSE, 0, 0);
			}
		}

		if (keyboard.IsJustPressed(KEY::W) || keyboard.IsJustPressed(KEY::ARROW_UP)) {
			--SelectedButton_;
		}
		if (keyboard.IsJustPressed(KEY::S) || keyboard.IsJustPressed(KEY::ARROW_DOWN)) {
			++SelectedButton_;
		}
		SelectedButton_ = std::clamp<int>(SelectedButton_, 0, 1);

		constexpr float deltaTime{ 1.0f / 60.0f };
		Watercolor_->Update(deltaTime);
	}
}

namespace Game::Scene {
	void Title::Update() {
		Impl_->Update();
	}
}