export module Game.Scene.Title : Impl.Effect;

import : Impl;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Utils.Color;

namespace Game::Scene::Impl {
	export namespace TitleEffect {
		constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };
		constexpr float Inv_900{ 1.0f / 900.0f };
		using Lumina::Math::Constant::Pi;
		using Lumina::Math::Constant::Inv_Pi;

		Lumina::Utils::Color::RGB RGB_Gaming{};

		auto& RNDEngine{ Lumina::Math::Random::Generator() };

		float TimeFactor{ 0.0f };

		Lumina::Math::F32x4x4 ViewToWorld{};
	}

	template<>
	auto Title::Update_<"EffectVariables">() -> void {
		// * Colors

		TitleEffect::RGB_Gaming = Lumina::Utils::Color::Convert(
			Lumina::Utils::Color::HSV{
				TitleEffect::RNDEngine() * TitleEffect::Inv_0xFFFFFFFF * 60.0f +
				TitleEffect::TimeFactor +
				TitleEffect::TimeFactor * 0.1f * 180.0f * TitleEffect::Inv_Pi,
				TitleEffect::RNDEngine() * TitleEffect::Inv_0xFFFFFFFF * 0.3f + 0.5f,
				0.95f
			}
		);

		TitleEffect::TimeFactor += 0.5f;

		TitleEffect::ViewToWorld = Camera_->ViewInverse();
	}
}