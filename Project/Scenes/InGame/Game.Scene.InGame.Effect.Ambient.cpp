module Game.Scene.InGame;

import : Impl;
import : Impl.Effect;

import <cmath>;

import Game.Events.InGame;
import Lumina.Utils.Color;
import Lumina.Core.Math;

namespace Game::Scene::Impl {
	namespace {
		using Effect::Inv_0xFFFFFFFF;
		using Lumina::Math::Constant::Pi;
		using Lumina::Math::Constant::Inv_Pi;

		using Effect::RGB_Gaming;
		using Effect::RNDEngine;

		Lumina::F32 RaindropsAngle{};
	}

	template<>
	auto InGame::Update_<"Effect.Ambient.Raindrops">() -> void {
		auto emitRaindrops{
			[&, this] (Lumina::F32 hueFactor_) -> void {
				static float effectTimeFactor{ 0.0f };
				effectTimeFactor += 0.5f;

				for (Lumina::I32 i = 0; i < 32; ++i) {
					// * パーティクル初期化
					Lumina::Particle p{};
					{
						Lumina::F32 const x{
							(RNDEngine() * Inv_0xFFFFFFFF * 40.0f - 20.0f) +
							Player_->GetPosition().X
						};
						Lumina::F32 const y{
							(RNDEngine() * Inv_0xFFFFFFFF * 10.0f - 5.0f) +
							Effect::ViewToWorld[3].Y() + (5.0f)
						};
						Lumina::F32 const z{
							(RNDEngine() * Inv_0xFFFFFFFF * 20.0f - 10.0f) +
							Player_->GetPosition().Z
						};
						p.Translate = { x, y, z };

						p.Velocity.X = (RNDEngine() * Inv_0xFFFFFFFF - 0.5f) * 0.01f;
						p.Velocity.Y = RNDEngine() * Inv_0xFFFFFFFF + (-2.0f);
						p.Velocity.Z = (RNDEngine() * Inv_0xFFFFFFFF - 0.5f) * 0.01f;

						p.Scale.X = 0.125f;
						p.Scale.Y = 2.5f;

						p.Life = 18.0f;

						auto const rgb_Base{
							Lumina::Utils::Color::Convert(
								Lumina::Utils::Color::HSV{
									RNDEngine() * Inv_0xFFFFFFFF * 45.0f + hueFactor_,
									RNDEngine() * Inv_0xFFFFFFFF * 0.3f + 0.2f,
									0.8f
								}
							)
						};

						p.RenderData.RGBA = {
							rgb_Base.R,
							rgb_Base.G,
							rgb_Base.B,
							0.25f
						};
						p.RenderData.DiffuseID = 1U;
						p.RenderData.DiffuseAtlasID = 0U;

						Raindrops_->Emit(std::move(p));
					}
				}
			}
		};

		emitRaindrops(180.0f);
	}

	template<>
	auto InGame::Update_<"Effect.Ambient.Sparkle">() -> void {
		static float sparkleTimeFactor{ 0.0f };
		sparkleTimeFactor += 0.75f;

		Lumina::F32 const x{
			(RNDEngine() * Inv_0xFFFFFFFF * 40.0f - 20.0f) +
			Player_->GetPosition().X
		};
		Lumina::F32 const y{
			(RNDEngine() * Inv_0xFFFFFFFF * 10.0f - 5.0f) +
			Effect::ViewToWorld[3].Y() + (-2.5f)
		};
		Lumina::F32 const z{
			(RNDEngine() * Inv_0xFFFFFFFF * 20.0f - 10.0f) +
			Player_->GetPosition().Z
		};

		static int sparkeTimeFactor2{ 0 };
		if ((sparkeTimeFactor2 & 0x1) == 0) {
			Lumina::Particle sparkle{};
			sparkle.Translate = {
				x,
				y,
				z
			};

			sparkle.Scale.X = 0.25f;
			sparkle.Scale.Y = 0.25f;
			sparkle.Life = 60.0f;

			sparkle.Rotate.Z = RNDEngine() * Inv_0xFFFFFFFF * Pi;

			auto rgb = Lumina::Utils::Color::Convert(
				Lumina::Utils::Color::HSV{
					RNDEngine() * Inv_0xFFFFFFFF * 45.0f,
					RNDEngine() * Inv_0xFFFFFFFF * 0.3f + 0.2f,
					0.5f
				}
			);

			sparkle.RenderData.RGBA = {
				rgb.R,
				rgb.G,
				rgb.B,
				0.0f
			};
			sparkle.RenderData.DiffuseID = 1U;
			sparkle.RenderData.DiffuseAtlasID = 5U;

			AmbientSparkles_->Emit(std::move(sparkle));
		}
		++sparkeTimeFactor2;
	}

	template<>
	auto InGame::Update_<"Effect.Ambient.Portals">() -> void {
		auto emitParticles{
			[this] (Lumina::F32x2&& pos_, int type_) -> void {
				Lumina::F32 const rnd{ RNDEngine() * Inv_0xFFFFFFFF };
				Lumina::F32 const theta{ rnd * 2.0f * Pi };
				Lumina::F32 const rho{ RNDEngine() * Inv_0xFFFFFFFF * 0.5f * Pi };
				Lumina::F32 const cos_Theta{ std::cos(theta) };
				Lumina::F32 const sin_Theta{ std::sin(theta) };
				Lumina::F32 const cos_Rho{ std::cos(rho) };
				Lumina::F32 const sin_Rho{ std::sin(rho) };

				Lumina::Particle p{};
				{
					p.Velocity.X = cos_Theta * cos_Rho * (-0.05f);
					p.Velocity.Y = sin_Theta * cos_Rho * (-0.05f);
					p.Velocity.Z = sin_Rho * (-0.05f);

					p.Translate.X = pos_.X + cos_Theta * cos_Rho * 2.5f;
					p.Translate.Y = pos_.Y + sin_Theta * cos_Rho * 2.5f;
					p.Translate.Z = sin_Rho * 2.5f;

					if (type_ == 0) {
						p.Scale.X = 5.0f;
						p.Scale.Y = 0.5f;

						p.Scale.Z = 0.0f;

						p.Rotate.Y = rho * (-1.0f);
						p.Rotate.Z = theta;
					}
					else {
						p.Scale.X = 0.75f;
						p.Scale.Y = 0.75f;

						p.Scale.Z = (type_ > 0) ? (0.001f) : (-0.001f);

						p.Rotate.Z = theta;
					}

					p.Life = 36.0f;

					Lumina::F32 const saturation{
						(type_ == 0) ?
						(RNDEngine() * Inv_0xFFFFFFFF * 0.2f + 0.1f) :
						(RNDEngine() * Inv_0xFFFFFFFF * 0.5f + 0.4f)
					};
					auto const rgb = Lumina::Utils::Color::Convert(
						Lumina::Utils::Color::HSV{
							RNDEngine() * Inv_0xFFFFFFFF * 25.0f + rnd * 360.0f,
							saturation,
							0.75f
						}
					);
					p.RenderData.RGBA = {
						rgb.R,
						rgb.G,
						rgb.B,
						0.0f
					};
					p.RenderData.DiffuseID = 1U;
					p.RenderData.DiffuseAtlasID = 5U;
					PortalSparkles_->Emit(std::move(p));
				}
			}
		};

		for (const auto& conn : playState_.CurrentArea.connections) {
			emitParticles({ conn.position.x, conn.position.y }, 0);
			emitParticles({ conn.position.x, conn.position.y }, 1);
			emitParticles({ conn.position.x, conn.position.y }, -1);
		}
	}

	template<>
	auto InGame::Update_<"RaindropParticle">(Lumina::Particle& p_) -> void {
		p_.Translate.X += p_.Velocity.X;
		p_.Translate.Y += p_.Velocity.Y;
		p_.Translate.Z += p_.Velocity.Z;

		p_.Rotate.Z =
			Effect::PlayerVelocity.X * (-0.015f) +
			(RNDEngine() * Inv_0xFFFFFFFF - 0.5f) * 0.05f;

		p_.Life -= 1.0f;
	}

	template<>
	auto InGame::Update_<"AmbientSparkleParticle">(Lumina::Particle& p_) -> void {
		p_.Translate.X += (RNDEngine() * Inv_0xFFFFFFFF - 0.5f) * 0.01f;
		p_.Translate.Y += RNDEngine() * Inv_0xFFFFFFFF * 0.02f;
		p_.Translate.Z += (RNDEngine() * Inv_0xFFFFFFFF - 0.5f) * 0.01f;

		Lumina::F32 const factor{
			1.0f - (p_.Life - 30.0f) * (p_.Life - 30.0f) * Effect::Inv_900
		};

		p_.RenderData.RGBA.W =
			factor * 0.25f +
			std::sin(p_.Life * 0.375f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f) * 0.15f;

		p_.Scale.X = factor * 0.5f;
		p_.Scale.Y = factor * 0.5f;

		Lumina::Math::F32x2 const d{
			Player_->GetPosition().X - p_.Translate.X,
			Player_->GetPosition().Y - p_.Translate.Y,
		};
		if (d.Dot(d) > 400.0f) { p_.Life -= 1.25f; }
		else { p_.Life -= 0.125f; }
	}

	template<>
	auto InGame::Update_<"PortalSparkleParticle">(Lumina::Particle& p_) -> void {
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