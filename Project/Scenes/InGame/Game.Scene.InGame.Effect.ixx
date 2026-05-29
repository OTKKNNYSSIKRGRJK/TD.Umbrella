export module Game.Scene.InGame : Impl.Effect;

import : Impl;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Utils.Color;

namespace Game::Scene::Impl {
	export namespace Effect {
		constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };
		constexpr float Inv_900{ 1.0f / 900.0f };
		using Lumina::Math::Constant::Pi;
		using Lumina::Math::Constant::Inv_Pi;

		int PlayerIdleEffectEmitFrameCount{ 0 };
		int PlayerMoveEffectEmitFrameCount{ 0 };
		int PlayerJumpEffectEmitFrameCount{ 0 };
		int PlayerAttackEffectEmitFrameCount{ 0 };
		int PlayerWarpEffectEmitFrameCount{ 0 };

		Lumina::Utils::Color::RGB RGB_Gaming{};

		auto& RNDEngine{ Lumina::Math::Random::Generator() };

		float TimeFactor{ 0.0f };

		Lumina::Math::F32x4x4 ViewToWorld{};

		Lumina::Math::F32x3 PlayerVelocity{};
	}

	template<>
	auto InGame::Update_<"EffectVariables">() -> void {
		// * Colors

		Effect::RGB_Gaming = Lumina::Utils::Color::Convert(
			Lumina::Utils::Color::HSV{
				Effect::RNDEngine() * Effect::Inv_0xFFFFFFFF * 60.0f +
				Effect::TimeFactor +
				Effect::TimeFactor * 0.1f * 180.0f * Effect::Inv_Pi,
				Effect::RNDEngine() * Effect::Inv_0xFFFFFFFF * 0.3f + 0.5f,
				0.95f
			}
		);

		Effect::TimeFactor += 0.5f;

		Effect::ViewToWorld = Camera_Player_->ViewInverse();
	}
}