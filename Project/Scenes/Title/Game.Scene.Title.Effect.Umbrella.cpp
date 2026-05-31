module Game.Scene.Title;

import : Impl;
import : Impl.Effect;

import Lumina.Utils.Color;
import Lumina.Core.Math;

namespace Game::Scene::Impl {
	namespace {
		using TitleEffect::Inv_0xFFFFFFFF;
		using Lumina::Math::Constant::Pi;
		using Lumina::Math::Constant::Inv_Pi;

		Lumina::Math::F32x3 WorldPos_UmbrellaRoot{};
		Lumina::Math::F32x3 WorldPos_UmbrellaTip{};
		Lumina::F32 TimeFactor_UmbrellaEffect{};

		using TitleEffect::RGB_Gaming;
		using TitleEffect::RNDEngine;
	}
}

namespace Game::Scene::Impl {
	template<>
	auto Title::Update_<"Effect.Umbrella.Perpetual">() -> void {
		TimeFactor_UmbrellaEffect += 0.1f;

		auto const& rootPos{ (*UmbrellaRootWorld_)[3] };
		WorldPos_UmbrellaRoot = { rootPos.X(), rootPos.Y(), rootPos.Z() };
		auto const& tipPos{ (*UmbrellaTipWorld_)[3] };
		WorldPos_UmbrellaTip = { tipPos.X(), tipPos.Y(), tipPos.Z() };
		auto const emitPos{
			WorldPos_UmbrellaRoot + (WorldPos_UmbrellaTip - WorldPos_UmbrellaRoot) * 1.2f
		};

		auto emitParticles{
			[&, this] (int type_) -> void {
				Lumina::F32 const rnd{ RNDEngine() * Inv_0xFFFFFFFF };
				Lumina::F32 const theta{ rnd * 2.0f * Pi };
				Lumina::F32 const rho{ RNDEngine() * Inv_0xFFFFFFFF * 0.5f * Pi };
				Lumina::F32 const cos_Theta{ std::cos(theta) };
				Lumina::F32 const sin_Theta{ std::sin(theta) };
				Lumina::F32 const cos_Rho{ std::cos(rho) };
				Lumina::F32 const sin_Rho{ std::sin(rho) };

				Lumina::Particle p{};
				{
					p.Velocity.X = cos_Theta * cos_Rho * 0.05f;
					p.Velocity.Y = sin_Theta * cos_Rho * 0.05f;
					p.Velocity.Z = sin_Rho * 0.05f;

					p.Translate.X = emitPos.X;
					p.Translate.Y = emitPos.Y;
					p.Translate.Z = emitPos.Z;

					if (type_ == 0) {
						p.Scale.X = 0.5f;
						p.Scale.Y = 0.5f;

						p.Scale.Z = 0.0f;

						p.Rotate.X = rho * 0.5f;
						p.Rotate.Y = rho * (-0.5f);
						p.Rotate.Z = theta;
					}
					else {
						p.Scale.X = 0.75f;
						p.Scale.Y = 0.75f;

						p.Scale.Z = (type_ > 0) ? (0.001f) : (-0.001f);
					}

					p.Rotate.X = rho * 0.5f;
					p.Rotate.Y = rho * (-0.5f);
					p.Rotate.Z = theta;

					p.Life = 36.0f;

					Lumina::F32 const saturation{
						(type_ == 0) ?
						(RNDEngine() * Inv_0xFFFFFFFF * 0.2f + 0.1f) :
						(RNDEngine() * Inv_0xFFFFFFFF * 0.3f + 0.5f)
					};
					auto const rgb_Gaming = Lumina::Utils::Color::Convert(
						Lumina::Utils::Color::HSV{
							RNDEngine() * Inv_0xFFFFFFFF * 25.0f + rnd * 360.0f + TimeFactor_UmbrellaEffect,
							saturation,
							0.75f
						}
					);
					p.RenderData.RGBA = {
						rgb_Gaming.R * 0.2f + 0.8f,
						rgb_Gaming.G * 0.8f + 0.2f,
						rgb_Gaming.B * 0.75f + 0.25f,
						0.0f
					};
					p.RenderData.DiffuseID = 0U;
					p.RenderData.DiffuseAtlasID = 5U;
					UmbrellaEffects_->Emit(std::move(p));
				}
			}
		};

		emitParticles(0);
		emitParticles(1);
		emitParticles(-1);
	}
}

namespace Game::Scene::Impl {
	template<>
	auto Title::Update_<"UmbrellaEffectParticle">(Lumina::Particle& p_) -> void {
		static Lumina::F32 const delta{ 0.01f };
		static Lumina::F32 const cos_Delta{ std::cos(delta) };
		static Lumina::F32 const sin_Delta{ std::sin(delta) };

		if (p_.Scale.Z != 0.0f) {
			Lumina::F32x2 vel{ p_.Velocity.X, p_.Velocity.Y };
			if (p_.Scale.Z > 0.0f) {
				p_.Velocity.X = vel.X * cos_Delta + vel.Y * (-sin_Delta);
				p_.Velocity.Y = vel.X * sin_Delta + vel.Y * cos_Delta;

				p_.Rotate.Z += delta;
			}
			else {
				p_.Velocity.X = vel.X * cos_Delta + vel.Y * sin_Delta;
				p_.Velocity.Y = vel.X * (-sin_Delta) + vel.Y * cos_Delta;

				p_.Rotate.Z -= delta;
			}
		}

		p_.Translate.X += p_.Velocity.X;
		p_.Translate.Y += p_.Velocity.Y;

		p_.Scale.X *= 0.93f;
		p_.Scale.Y *= 0.93f;

		p_.RenderData.RGBA.W += 0.02f;

		p_.Life -= 1.0f;
	}
}