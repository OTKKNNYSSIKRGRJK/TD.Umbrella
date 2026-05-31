module Game.Scene.Clear;

import <vector>;
import <cmath>;
import <numbers>;
import <algorithm>;

import : Impl;
import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.Core.Math;
import Lumina.Utils.Color;

namespace Game::Scene::Impl {
	namespace {
		constexpr float Inv = 1.0f / 72.0f;
		constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };
	}

	void Clear::Update() {
		BackgroundCenter_Prev_ = BackgroundCenter_;

		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };

		// Input
		{
			if (Count_FadeIn_ < 0) {
				auto updateKeyState{
					[&](
						uint32_t& keyState_,
						Lumina::OS::Windows::KEY keyCode_
					) -> void {
						keyState_ = (keyState_ << 1U) + !!keyboard.IsPressed(keyCode_);
					}
				};
				auto isKeyJustPressed{
					[&](uint32_t keyState_) constexpr -> bool {
						return ((keyState_ & 1U) && !(keyState_ & 2U));
					}
				};
				updateKeyState(KeyState_Space_, Lumina::OS::Windows::KEY::SPACE);
				if (isKeyJustPressed(KeyState_Space_)) {
					auto& sceneMngr{ Lumina::SceneManager::Instance() };
					sceneMngr.Activate("Title");
					sceneMngr.Deactivate("Clear");
				}
			}
		}

		auto& rndEngine{ Lumina::Math::Random::Generator() };

		{
			static float sparkleTimeFactor{ 0.0f };
			static int sparkleTimeFactor2{ 144 };
			static constexpr float inv_48{ 1.0f / 48.0f };
			sparkleTimeFactor += 0.75f;

			if (sparkleTimeFactor2 >= 144) {
				float const rad = static_cast<float>(rndEngine() % 20) + 2.5f;
				Lumina::F32x2 const center = {
					rad * 24.0f * std::cos(SceneRotation_ * 2.4f) + 640.0f,
					rad * 13.5f * std::sin(SceneRotation_ * 3.6f) + 360.0f
				};

				for (int i = 0; i < 96; ++i) {
					float const theta{ i * std::numbers::pi_v<float> *inv_48 };
					float const vx{ std::cos(theta) };
					float const vy{ std::sin(theta) };
					Lumina::Particle sparkle{};
					{
						float v = 5.0f;
						if (i & 1) {
							v *= 0.25f + 0.75f * std::abs(std::cos(theta * 2.0f + rad));
						}
						else {
							v *= 0.125f + 0.375f * std::abs(std::sin(theta * 2.0f + rad));
						}
						sparkle.Translate = { center.X, center.Y, 0.0f };
						sparkle.Velocity = { vx * v, vy * v - 1.5f, 0.0f };
						sparkle.Scale.X = 50.0f;
						sparkle.Scale.Y = 50.0f;
						sparkle.Life = 216.0f;
						auto const rgb_Base = Lumina::Utils::Color::Convert(
							Lumina::Utils::Color::HSV{
								rndEngine() * Inv_0xFFFFFFFF * 45.0f + 15.0f,
								rndEngine() * Inv_0xFFFFFFFF * 0.5f + 0.5f,
								0.75f
							}
						);
						auto const rgb_Gaming = Lumina::Utils::Color::Convert(
							Lumina::Utils::Color::HSV{
								rndEngine() * Inv_0xFFFFFFFF * 45.0f +
								sparkleTimeFactor +
								i * 360.0f * inv_48,
								rndEngine() * Inv_0xFFFFFFFF * 0.3f + 0.3f,
								0.95f
							}
						);
						sparkle.RenderData.RGBA = {
							rgb_Base.R * (0.1f + rgb_Gaming.R * 0.9f),
							rgb_Base.G * (0.1f + rgb_Gaming.G * 0.9f),
							rgb_Base.B * (0.1f + rgb_Gaming.B * 0.9f),
							0.0f
						};
						sparkle.RenderData.DiffuseID = 2U;
						sparkle.RenderData.DiffuseAtlasID = (i & 1) ? (2U) : (3U);
					}
					CircularSparkles_->Emit(std::move(sparkle));
				}
				sparkleTimeFactor2 = 0;
			}
			++sparkleTimeFactor2;
		}

		List_PointLight_.Clear();
		List_Matrix_World_LightSphere_.Clear();

		Lumina::List<Lumina::Particle>::Iterator it_Sparkle{ CircularSparkles_->InstanceList() };
		for (it_Sparkle.Begin(); !it_Sparkle.End(); it_Sparkle.Next()) {
			auto const& sparkle{ *it_Sparkle };

			if (!List_PointLight_.IsFull()) {
				auto& light{ List_PointLight_.New() };

				light.WorldPosition = {
					sparkle.Translate.X,
					sparkle.Translate.Y,
					sparkle.Translate.Z,
					1.0f
				};
				light.RGB = {
					sparkle.RenderData.RGBA.X,
					sparkle.RenderData.RGBA.Y,
					sparkle.RenderData.RGBA.Z
				};
				light.Intensity = sparkle.RenderData.RGBA.W * sparkle.Scale.X * 15.0f;

				auto& lightSphere{ List_Matrix_World_LightSphere_.New() };

				float const radius{ 120.0f };
				lightSphere = {
					radius, 0.0f, 0.0f, 0.0f,
					0.0f, radius, 0.0f, 0.0f,
					0.0f, 0.0f, radius, 0.0f,
					light.WorldPosition.X,
					light.WorldPosition.Y,
					light.WorldPosition.Z,
					1.0f,
				};
			}
		}

		std::vector<uint32_t> lightIndices{};
		Lumina::List<Lumina::PointLight>::Iterator it_Light{ List_PointLight_ };
		for (it_Light.Begin(); !it_Light.End(); it_Light.Next()) {
			lightIndices.emplace_back(it_Light.Index());
		}
		DeferredLighting_->Update(List_PointLight_, List_Matrix_World_LightSphere_, lightIndices);

		{
			static float bloomFactor = 0.0f;
			bloomFactor += 0.01f;
			ClearPostProcessingConstants_.BloomRadius = 0.5f + 0.125f * std::sin(bloomFactor * 2.0f);
			ClearPostProcessingConstants_.BloomIntensity = 0.875f + 0.125f * std::sin(bloomFactor * 1.75f);
			ClearPostProcessingConstants_.BloomAttenuation = 0.25f - 0.125f * std::sin(bloomFactor * 1.5f);
		}
		UB_PostProcessingConstants_.Store(
			&ClearPostProcessingConstants_,
			sizeof(ClearPostProcessingConstants),
			0LLU
		);

		{
			UIFadeIn_ += 0.05f;
			float const factor = std::sin(UIFadeIn_);

			auto const rgb_Base = Lumina::Utils::Color::Convert(
				Lumina::Utils::Color::HSV{
					rndEngine() * Inv_0xFFFFFFFF * 3.0f + 7.5f + factor * 15.0f,
					rndEngine() * Inv_0xFFFFFFFF * 0.1f + 0.1f,
					0.75f
				}
			);

			UI_Label_StageClear_.Scale.X = 720.0f * (1.0f + factor * 0.025f) * 1.25f;
			UI_Label_StageClear_.Scale.Y = 180.0f * (1.0f + factor * 0.025f) * 1.25f;
			UI_Label_StageClear_.RGBA.X = rgb_Base.R;
			UI_Label_StageClear_.RGBA.Y = rgb_Base.G;
			UI_Label_StageClear_.RGBA.Z = rgb_Base.B;

			UI_Label_PressSpaceKey_.Scale.X = 720.0f * (1.0f + factor * 0.0125f);
			UI_Label_PressSpaceKey_.Scale.Y = 180.0f * (1.0f + factor * 0.0125f);
			UI_Label_PressSpaceKey_.RGBA.X = rgb_Base.R;
			UI_Label_PressSpaceKey_.RGBA.Y = rgb_Base.G;
			UI_Label_PressSpaceKey_.RGBA.Z = rgb_Base.B;
		}

		if (Count_FadeIn_ >= 0) {
			{
				float const t{ static_cast<float>(72 - Count_FadeIn_) / 72.0f };
				float const factor = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
				UI_Label_StageClear_.Translate.Y = 390.0f - 60.0f * factor;
				UI_Label_StageClear_.Scale.X *= (0.9f + 0.1f * factor);
				UI_Label_StageClear_.Scale.Y *= (0.9f + 0.1f * factor);
				UI_Label_StageClear_.RGBA.W = 0.5f * factor * factor;
			}
			{
				float const t{ static_cast<float>(std::max<int>(60 - Count_FadeIn_, 0)) / 60.0f };
				float const factor = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
				UI_Label_PressSpaceKey_.Translate.Y = 510.0f - 60.0f * factor;
				UI_Label_PressSpaceKey_.Scale.X *= (0.8f + 0.2f * factor);
				UI_Label_PressSpaceKey_.Scale.Y *= (0.8f + 0.2f * factor);
				UI_Label_PressSpaceKey_.RGBA.W = 0.125f * factor * factor;
			}
			--Count_FadeIn_;
		}

		if (Count_FadeOut_ >= 72) {
			BackgroundCenter_ = { 640.0f, 360.0f };

			IterationTime_ = 0;
			NextIterationTime_ = 0;

			KeyState_Space_ = 0U;

			Count_FadeOut_ = 0;
			ClearPostProcessingConstants_.IsFadingOut = 0U;
			UIFadeIn_ = 0.0f;
			Count_FadeIn_ = 72;
		}
		if (Count_FadeOut_ > 0) {
			++Count_FadeOut_;
		}

		SceneRotation_ += 0.1f;
	}
}

namespace Game::Scene {
	void Clear::Update() {
		Impl_->Update();
	}
}