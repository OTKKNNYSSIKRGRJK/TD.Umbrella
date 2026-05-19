module Game.Scene.Title;

import <cmath>;
import <numbers>;

import : Impl;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.CG3D.Animation;

import Lumina.Utils.ImGui;
import Lumina.Utils.Color;

import Game.MathUtils;

namespace {
	constexpr Lumina::F32 INV_60{ 1.0f / 60.0f };
	constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };
}

namespace Game::Scene::Impl {
	template<>
	auto Title::Update_<"Camera">() -> void {
		static Lumina::Math::F32x3 eye{ 7.5f, 2.5f, 5.0f };
		static Lumina::Math::F32x3 target{ 0.0f, 2.0f, 0.0f };
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
	}

	template<>
	auto Title::Update_<"Raindrops">() -> void {
		auto& rndEngine{ Lumina::Math::Random::Generator() };

		auto emitRaindrops{
			[&, this](Lumina::F32 hueFactor_) -> void {
				static float effectTimeFactor{ 0.0f };
				effectTimeFactor += 0.5f;

				for (Lumina::I32 i = 0; i < 32; ++i) {
					Lumina::Particle p{};
					{
						p.Translate = {
							std::cos(effectTimeFactor * 0.3f + i * 3.6f) * 5.0f,
							std::sin(effectTimeFactor * 0.4f * i) * 1.5f + 10.0f,
							std::sin(effectTimeFactor * 0.5f - i * 1.2f) * 5.0f
						};

						p.Velocity.X = p.Translate.Z * 0.01f;
						p.Velocity.Y = p.Translate.Y * (-0.1f);
						p.Velocity.Z = p.Translate.X * 0.01f;

						p.Scale.X = 15.0f;
						p.Scale.Y = 15.0f;

						p.Rotate.Z = rndEngine() * Inv_0xFFFFFFFF * std::numbers::pi_v<float> *2.0f;

						p.Life = 180.0f;

						auto const rgb_Base = Lumina::Utils::Color::Convert(
							Lumina::Utils::Color::HSV{
								rndEngine() * Inv_0xFFFFFFFF * 45.0f + hueFactor_,
								rndEngine() * Inv_0xFFFFFFFF * 0.5f + 0.5f,
								0.75f
							}
						);
						p.RenderData.RGBA = {
							rgb_Base.R,
							rgb_Base.G,
							rgb_Base.B,
							0.05f
						};
						p.RenderData.DiffuseID = 0U;
						p.RenderData.DiffuseAtlasID = (rndEngine() % 5U) + 2U;
						Raindrops_->Emit(std::move(p));
					}
				}
			}
		};

		emitRaindrops(180.0f);
	}

	void Title::Update() {
		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		using Lumina::OS::Windows::KEY;

		Update_<"Camera">();
		Update_<"Raindrops">();

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

		constexpr float deltaTime{ 1.0f / 60.0f };
		Watercolor_->Update(deltaTime);
	}
}

namespace Game::Scene {
	void Title::Update() {
		Impl_->Update();
	}
}