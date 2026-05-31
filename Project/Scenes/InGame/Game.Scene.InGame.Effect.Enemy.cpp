module Game.Scene.InGame;

import : Impl;
import : Impl.Effect;

import Lumina.Utils.Color;
import Lumina.Core.Math;
import Game.EnemyManager;

namespace Game::Scene::Impl {
	namespace {
		using Effect::Inv_0xFFFFFFFF;
		using Lumina::Math::Constant::Pi;
		using Lumina::Math::Constant::Inv_Pi;

		using Effect::PlayerIdleEffectEmitFrameCount;
		using Effect::PlayerMoveEffectEmitFrameCount;
		using Effect::PlayerJumpEffectEmitFrameCount;
		using Effect::PlayerAttackEffectEmitFrameCount;
		using Effect::PlayerWarpEffectEmitFrameCount;

		Lumina::F32 PlayerEffectTimeFactor{ 0.0f };

		Lumina::Math::F32x4x4<> World_Hands[2]{};
		Lumina::Math::F32x4x4<> World_Feet[2]{};

		Lumina::Math::F32x4x4<> World_Hip{};

		Lumina::Math::F32x3 WorldPos_UmbrellaRoot{};
		Lumina::Math::F32x3 WorldPos_UmbrellaTip{};
		Lumina::F32 TimeFactor_UmbrellaEffect{};

		using Effect::RGB_Gaming;
		using Effect::RNDEngine;
	}
}

namespace Game::Scene::Impl {
	template<>
	auto InGame::Update_<"Effect.Enemy.Perpetual">(
		Lumina::Math::F32x3&& worldPos_
	) -> void {
		for (int i{ 0 }; i < 2; ++i) {
			Lumina::Particle p{};
			{
				p.Translate = {
					std::cos(PlayerEffectTimeFactor * 0.3f + i * 2.4f) * 0.1f,
					std::sin(PlayerEffectTimeFactor * 0.4f + i * 3.6f) * 0.1f,
					std::sin(PlayerEffectTimeFactor * 0.5f - i * 1.2f) * 0.1f
				};

				p.Velocity.X = p.Translate.Y * (-0.25f);
				p.Velocity.Y = p.Translate.Z * (-0.25f);
				p.Velocity.Z = p.Translate.X * (-0.25f);

				p.Translate.X += worldPos_.X;
				p.Translate.Y += worldPos_.Y;
				p.Translate.Z += worldPos_.Z;

				p.Scale.X = 0.75f;
				p.Scale.Y = 0.75f;

				p.Rotate.Z = RNDEngine() * Inv_0xFFFFFFFF * Pi * 2.0f;

				p.Life = 36.0f;

				p.RenderData.RGBA = {
					RGB_Gaming.R * 0.9f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
					RGB_Gaming.G * 0.9f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
					RGB_Gaming.B * 0.9f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
					0.375f
				};
				p.RenderData.DiffuseID = 1U;
				p.RenderData.DiffuseAtlasID = (RNDEngine() & 3) ? (5U) : (4U);
				PlayerEffects_->Emit(std::move(p));
			}
		}
	}

	template<>
	auto InGame::Update_<"Effect.Enemies">() -> void {
		auto const& enemyMngr{ *Game::EnemyManager::GetInstance() };
		auto const& enemies{ enemyMngr.GetAllInstances() };
		for (auto const& enemy : enemies) {
			if (!enemy.isDead) {
				Update_<"Effect.Enemy.Perpetual">(Lumina::Math::F32x3{ enemy.position });
			}
		}
	}
}

namespace Game::Scene::Impl {
	template<>
	auto InGame::Update_<"EnemyEffectParticle">(Lumina::Particle& p_) -> void {
		p_.Translate.X += p_.Velocity.X;
		p_.Translate.Y += p_.Velocity.Y;
		p_.RenderData.RGBA.W *= 0.95f;
		p_.Scale.X *= 0.93f;
		p_.Scale.Y *= 0.93f;
		p_.Rotate.Z += p_.Velocity.Z * 0.01f;
		p_.Life -= 1.0f;
	}
}