module Game.Scene.InGame;

import : Impl;
import : Impl.Effect;

import Game.Events.InGame;
import Lumina.Utils.Color;
import Lumina.Core.Math;

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

		using Effect::RGB_Gaming;
		using Effect::RNDEngine;
	}
}

namespace Game::Scene::Impl {
	template<>
	auto InGame::Update_<"PlayerEffect.Common">() -> void {
		static auto const& animatedModel{ Player_->GetAnimatedModel() };
		static auto const& skeleton{ animatedModel.second.Skeleton_ };

		// * World Positions of Hands

		static Lumina::U32 idx_LeftHand{ skeleton.IDX_Joint.find("Bone.012")->second };
		static Lumina::U32 idx_RightHand{ skeleton.IDX_Joint.find("Bone.024")->second };
		static auto const& leftHand{ skeleton.ARR_Joint[idx_LeftHand] };
		static auto const& rightHand{ skeleton.ARR_Joint[idx_RightHand] };

		World_Hands[0] = leftHand.SkeletonSpace * Player_->WorldMatrix();
		World_Hands[1] = rightHand.SkeletonSpace * Player_->WorldMatrix();

		// * World Positions of Feet

		static Lumina::U32 idx_LeftFoot{ skeleton.IDX_Joint.find("Bone.018")->second };
		static Lumina::U32 idx_RightFoot{ skeleton.IDX_Joint.find("Bone.030")->second };
		static auto const& leftFoot{ skeleton.ARR_Joint[idx_LeftFoot] };
		static auto const& rightFoot{ skeleton.ARR_Joint[idx_RightFoot] };

		World_Feet[0] = leftFoot.SkeletonSpace * Player_->WorldMatrix();
		World_Feet[1] = rightFoot.SkeletonSpace * Player_->WorldMatrix();

		// * World Position of Hip

		static Lumina::U32 idx_Hip{ skeleton.IDX_Joint.find("Bone")->second };
		static auto const& hip{ skeleton.ARR_Joint[idx_Hip] };

		World_Hip = hip.SkeletonSpace * Player_->WorldMatrix();

		auto const& umbrella{ Player_->GetUmbrella() };
		WorldPos_UmbrellaRoot = umbrella.handle_->GetBaseJoint()->GetWorldPos();
		WorldPos_UmbrellaTip = umbrella.handle_->GetTipJoint()->GetWorldPos();

		PlayerEffectTimeFactor += 0.5f;
	}

	template<>
	auto InGame::Update_<"PlayerEffect.Perpetual">() -> void {
		//auto const& playerPos{ Player_->GetPosition() };

		// * Hands

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

				auto const& worldPos_Hand{ World_Hands[i][3] };
				p.Translate.X += worldPos_Hand.X();
				p.Translate.Y += worldPos_Hand.Y();
				p.Translate.Z += worldPos_Hand.Z();

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
	auto InGame::Update_<"PlayerEffect.Move">() -> void {
		if (PlayerMoveEffectEmitFrameCount <= 0) { return; }

		// * Feet

		for (int i{ 0 }; i < 2; ++i) {
			Lumina::Particle p_Move{};
			{
				p_Move.Translate = {
					std::cos(PlayerEffectTimeFactor * 4.8f + Pi * i) * 0.0625f,
					std::sin(PlayerEffectTimeFactor * 4.8f + Pi * i) * 0.0625f,
					0.0f,
				};
				p_Move.Translate.Z = p_Move.Translate.X;

				p_Move.Velocity.X = Effect::PlayerVelocity.X * (-0.015625f) * 0.75f;
				p_Move.Velocity.Y = p_Move.Translate.X * (-0.5f) * 0.75f;

				auto const& world_Foot{ World_Feet[i][3] };
				p_Move.Translate.X += world_Foot.X();
				p_Move.Translate.Y += world_Foot.Y();
				p_Move.Translate.Z += world_Foot.Z();

				p_Move.Scale.X = 0.625f;
				p_Move.Scale.Y = 0.625f;

				p_Move.Rotate.Z = RNDEngine() * Inv_0xFFFFFFFF * Pi * 2.0f;

				p_Move.Life = 48.0f;

				p_Move.RenderData.RGBA = {
					0.2f + RGB_Gaming.R * 0.5f,
					0.6f + RGB_Gaming.G * 0.3f,
					0.4f + RGB_Gaming.B * 0.6f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
					0.75f
				};
				p_Move.RenderData.DiffuseID = 1U;
				p_Move.RenderData.DiffuseAtlasID = (RNDEngine() & 3) ? (0U) : (3U);
				PlayerEffects_->Emit(std::move(p_Move));
			}
		}

		--PlayerMoveEffectEmitFrameCount;
	}

	template<>
	auto InGame::Update_<"PlayerEffect.Jump">() -> void {
		if (PlayerJumpEffectEmitFrameCount <= 0) { return; }

		auto& rndEngine{ Lumina::Math::Random::Generator() };

		// * Feet

		for (int i{ 0 }; i < 2; ++i) {
			Lumina::Particle p_Move{};
			{
				p_Move.Translate = {
					std::cos(PlayerEffectTimeFactor * 4.8f + Pi * i) * 0.0625f,
					std::sin(PlayerEffectTimeFactor * 4.8f + Pi * i) * 0.0625f,
					0.0f,
				};
				p_Move.Translate.Z = p_Move.Translate.X;

				p_Move.Velocity.X = p_Move.Translate.X * (-0.1f) + Effect::PlayerVelocity.X * (-0.015625f);
				p_Move.Velocity.Y = -0.125f;

				auto const& world_Foot{ World_Feet[i][3] };
				p_Move.Translate.X += world_Foot.X();
				p_Move.Translate.Y += world_Foot.Y();
				p_Move.Translate.Z += world_Foot.Z();

				p_Move.Scale.X = 0.75f;
				p_Move.Scale.Y = 0.75f;

				p_Move.Rotate.Z = rndEngine() * Inv_0xFFFFFFFF * Pi * 2.0f;

				p_Move.Life = 36.0f;

				p_Move.RenderData.RGBA = {
					0.5f + RGB_Gaming.R * 0.1f + rndEngine() * Inv_0xFFFFFFFF * 0.3f,
					0.6f + RGB_Gaming.G * 0.1f + rndEngine() * Inv_0xFFFFFFFF * 0.3f,
					0.3f + RGB_Gaming.B * 0.1f + rndEngine() * Inv_0xFFFFFFFF * 0.05f,
					0.75f
				};
				p_Move.RenderData.DiffuseID = 1U;
				p_Move.RenderData.DiffuseAtlasID = (rndEngine() & 3) ? (0U) : (3U);
				PlayerEffects_->Emit(std::move(p_Move));
			}
		}

		--PlayerJumpEffectEmitFrameCount;
	}

	template<>
	auto InGame::Update_<"PlayerEffect.Warp.0">() -> void {
		if (PlayerWarpEffectEmitFrameCount <= 0) { return; }

		static float effectTimeFactor{ 0.0f };
		effectTimeFactor += 0.5f;

		constexpr Lumina::F32 inv_32{ 1.0f / 32.0f };

		for (int i = 0; i < 32; ++i) {
			Lumina::Particle p{};
			{
				Lumina::F32 const theta{ i * inv_32 * 2.0f * Pi };
				p.Translate = {
					std::cos(theta) * 0.75f * PlayerWarpEffectEmitFrameCount,
					std::sin(theta) * 0.75f * PlayerWarpEffectEmitFrameCount,
					0.0f
				};

				p.Velocity.X = p.Translate.X * 0.1f;
				p.Velocity.Y = p.Translate.Y * 0.1f;

				p.Translate.X += World_Hip[3].X();
				p.Translate.Y += World_Hip[3].Y();
				p.Translate.Z += World_Hip[3].Z();

				p.Scale.X = 2.0f + RNDEngine() * Inv_0xFFFFFFFF;
				p.Scale.Y = 2.0f + RNDEngine() * Inv_0xFFFFFFFF;

				p.Rotate.Z = RNDEngine() * Inv_0xFFFFFFFF * Pi * 2.0f;

				p.Life = 36.0f;

				auto const rgb_Gaming = Lumina::Utils::Color::Convert(
					Lumina::Utils::Color::HSV{
						RNDEngine() * Inv_0xFFFFFFFF * 45.0f + theta * 90.0f,
						RNDEngine() * Inv_0xFFFFFFFF * 0.5f + 0.5f,
						0.75f
					}
				);
				p.RenderData.RGBA = {
					rgb_Gaming.R * 0.75f + 0.25f,
					rgb_Gaming.G * 0.9f + 0.1f,
					rgb_Gaming.B,
					0.25f
				};
				p.RenderData.DiffuseID = 1U;
				p.RenderData.DiffuseAtlasID = 5U;
				PlayerEffects_->Emit(std::move(p));
			}
		}
	}
	template<>
	auto InGame::Update_<"PlayerEffect.Warp.1">() -> void {
		if (PlayerWarpEffectEmitFrameCount <= 0) { return; }

		static float effectTimeFactor{ 0.0f };
		effectTimeFactor += 0.5f;

		constexpr Lumina::F32 inv_32{ 1.0f / 32.0f };

		Lumina::F32 const factor2{ RNDEngine() * Inv_0xFFFFFFFF };

		for (int i = 0; i < 32; ++i) {
			Lumina::Particle p{};
			{
				Lumina::F32 const theta{ i * inv_32 * 2.0f * Pi };

				Lumina::F32 const factor{ std::abs(std::cos(theta * 4.0f + factor2)) + 0.01f };

				p.Velocity.X = std::cos(theta) * 0.1f * PlayerWarpEffectEmitFrameCount * factor;
				p.Velocity.Y = std::sin(theta) * 0.1f * PlayerWarpEffectEmitFrameCount * factor;

				p.Translate.X += World_Hip[3].X();
				p.Translate.Y += World_Hip[3].Y();
				p.Translate.Z += World_Hip[3].Z();

				p.Scale.X = 2.5f;
				p.Scale.Y = 0.75f;

				p.Rotate.Z = theta;

				p.Life = 18.0f;

				auto const rgb_Base = Lumina::Utils::Color::Convert(
					Lumina::Utils::Color::HSV{
						RNDEngine() * Inv_0xFFFFFFFF * 45.0f,
						RNDEngine() * Inv_0xFFFFFFFF * 0.3f + 0.2f,
						0.75f
					}
				);
				p.RenderData.RGBA = {
					rgb_Base.R,
					rgb_Base.G,
					rgb_Base.B,
					0.9f
				};
				p.RenderData.DiffuseID = 1U;
				p.RenderData.DiffuseAtlasID = 0U;
				PlayerEffects_->Emit(std::move(p));
			}
		}
	}
	template<>
	auto InGame::Update_<"PlayerEffect.Warp.2">() -> void {
		if (PlayerWarpEffectEmitFrameCount <= 0) { return; }

		static float effectTimeFactor{ 0.0f };
		effectTimeFactor += 0.5f;

		for (int i = 0; i < 4; ++i) {
			Lumina::Particle p{};
			{
				Lumina::F32 const theta{ RNDEngine() * Inv_0xFFFFFFFF * 2.0f * Pi };

				p.Velocity.X = std::cos(theta) * 0.05f * PlayerWarpEffectEmitFrameCount;
				p.Velocity.Y = std::sin(theta) * 0.05f * PlayerWarpEffectEmitFrameCount;

				p.Translate.X += World_Hip[3].X();
				p.Translate.Y += World_Hip[3].Y();
				p.Translate.Z += World_Hip[3].Z();

				p.Scale.X = 5.0f;
				p.Scale.Y = 5.0f;

				p.Rotate.Z = RNDEngine() * Inv_0xFFFFFFFF * 2.0f * Pi;

				p.Life = 24.0f;

				auto const rgb_Base = Lumina::Utils::Color::Convert(
					Lumina::Utils::Color::HSV{
						RNDEngine() * Inv_0xFFFFFFFF * 45.0f,
						RNDEngine() * Inv_0xFFFFFFFF * 0.1f + 0.1f,
						0.75f
					}
				);
				p.RenderData.RGBA = {
					rgb_Base.R,
					rgb_Base.G,
					rgb_Base.B,
					0.15f
				};
				p.RenderData.DiffuseID = 1U;
				p.RenderData.DiffuseAtlasID = 6U;
				PlayerEffects_->Emit(std::move(p));
			}
		}
	}

	template<>
	auto InGame::Update_<"PlayerEffect.Warp">() -> void {
		Update_<"PlayerEffect.Warp.0">();
		Update_<"PlayerEffect.Warp.1">();
		Update_<"PlayerEffect.Warp.2">();

		--PlayerWarpEffectEmitFrameCount;
	}

	template<>
	auto InGame::Update_<"PlayerEffect.Charge.Cylinder">() -> void {
		
	}
	template<>
	auto InGame::Update_<"PlayerEffect.Charge.Spring">() -> void {
		
	}
}

namespace Game::Scene::Impl {
	template<>
	auto InGame::Update_<"PlayerEffectParticle">(Lumina::Particle& p_) -> void {
		p_.Translate.X += p_.Velocity.X;
		p_.Translate.Y += p_.Velocity.Y;
		p_.RenderData.RGBA.W *= 0.95f;
		p_.Scale.X *= 0.93f;
		p_.Scale.Y *= 0.93f;
		p_.Rotate.Z += p_.Velocity.Z * 0.01f;
		p_.Life -= 1.0f;
	}
}

namespace Game::Scene::Impl {
	template<>
	auto InGame::Update_<"UmbrellaEffect.Perpetual">() -> void {
		auto tipEffect{
			[&, this](Lumina::I32 i_) {
				Lumina::Particle2 p{};
				{
					std::memcpy(
						&p.World,
						Lumina::Math::F32x4x4<>::Identity,
						sizeof(Lumina::Math::F32x4x4<>)
					);

					p.World[3][0] = std::cos(PlayerEffectTimeFactor * 0.14f + i_ * 2.4f) * 0.15f;
					p.World[3][1] = std::sin(PlayerEffectTimeFactor * 0.13f - i_ * 3.6f) * 0.15f;
					p.World[3][2] = std::sin(PlayerEffectTimeFactor * 0.12f - i_ * 1.2f) * 0.15f;

					p.Velocity.X = p.World[3][0] * (-0.05f);
					p.Velocity.Y = p.World[3][1] * (-0.05f);
					p.Velocity.Z = p.World[3][2] * (-0.05f);

					p.World[3][0] += WorldPos_UmbrellaTip.X;
					p.World[3][1] += WorldPos_UmbrellaTip.Y;
					p.World[3][2] += WorldPos_UmbrellaTip.Z;

					Lumina::F32 const rotZ{ RNDEngine() * Inv_0xFFFFFFFF * Pi * 2.0f };
					Lumina::F32 const cos_RotZ{ std::cos(rotZ) };
					Lumina::F32 const sin_RotZ{ std::sin(rotZ) };

					p.World[0][0] = cos_RotZ * 0.75f;
					p.World[0][1] = sin_RotZ * 0.75f;
					p.World[1][0] = -sin_RotZ * 0.75f;
					p.World[1][1] = cos_RotZ * 0.75f;

					p.Life = 48.0f;

					p.RenderData.RGBA = {
						RGB_Gaming.R * 0.3f + 0.8f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.G * 0.2f + 0.3f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.B * 0.2f + 0.3f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						0.125f
					};
					p.RenderData.DiffuseID = 1U;
					p.RenderData.DiffuseAtlasID = (RNDEngine() & 3) ? (5U) : (4U);
					UmbrellaEffects_->Emit(std::move(p));
				}
			}
		};

		for (int i{ 0 }; i < 2; ++i) {
			tipEffect(i);
		}
	}

	template<>
	auto InGame::Update_<"UmbrellaEffect.Attack">() -> void {
		if (PlayerAttackEffectEmitFrameCount <= 0) { return; }

		auto const worldPos{ (WorldPos_UmbrellaRoot + WorldPos_UmbrellaTip) * 0.5f };
		auto const dPos{ (WorldPos_UmbrellaTip - WorldPos_UmbrellaRoot) * 0.5f };

		auto emitEffect{
			[&, this]() {
				Lumina::Particle2 p{};
				{
					auto const& umbrella{ Player_->GetUmbrella() };
					auto const& world_Umbrella{ umbrella.handle_->GetBaseJoint()->GetMatrix() };

					std::memcpy(
					   &p.World,
					   world_Umbrella,
					   sizeof(Lumina::Math::F32x4x4<>)
					);

					p.World[3][0] = worldPos.X;
					p.World[3][1] = worldPos.Y;
					p.World[3][2] = worldPos.Z;

					p.World[0][0] *= 3.0f;
					p.World[0][1] *= 3.0f;
					p.World[0][2] *= 3.0f;
					p.World[1][0] *= 3.0f;
					p.World[1][1] *= 3.0f;
					p.World[1][2] *= 3.0f;
					p.World[2][0] *= 3.0f;
					p.World[2][1] *= 3.0f;
					p.World[2][2] *= 3.0f;

					//p.Velocity = { dPos.X * 0.0625f, dPos.Y * 0.0625f, dPos.Z * 0.0625f };

					p.Life = 48.0f;

					p.RenderData.RGBA = {
						RGB_Gaming.R * 0.2f + 0.8f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.G * 0.8f + 0.2f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.B * 0.8f + 0.2f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						0.5f
					};
					p.RenderData.DiffuseID = 1U;
					p.RenderData.DiffuseAtlasID = 5U;
				}

				Lumina::Particle2 p2{};
				{
					std::memcpy(&p2, &p, sizeof(Lumina::Particle2));

					p2.Velocity.X *= -1.0f;
					p2.Velocity.Y *= -1.0f;
					p2.Velocity.Z *= -1.0f;

					p2.RenderData.RGBA = {
						RGB_Gaming.R * 0.2f + 0.8f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.G * 0.8f + 0.2f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.B * 0.6f + 0.4f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						0.5f
					};
				}

				UmbrellaEffects_->Emit(std::move(p));
				UmbrellaEffects_->Emit(std::move(p2));
			}
		};

		emitEffect();

		--PlayerAttackEffectEmitFrameCount;
	}
}

namespace Game::Scene::Impl {
	template<>
	auto InGame::Update_<"UmbrellaEffectParticle">(Lumina::Particle2& p_) -> void {
		p_.World[3][0] += p_.Velocity.X;
		p_.World[3][1] += p_.Velocity.Y;
		p_.World[3][2] += p_.Velocity.Z;

		p_.World[0][0] *= 0.99f;
		p_.World[0][1] *= 0.99f;
		p_.World[0][2] *= 0.99f;
		p_.World[1][0] *= 0.99f;
		p_.World[1][1] *= 0.99f;
		p_.World[1][0] *= 0.99f;
		p_.World[1][2] *= 0.99f;
		p_.World[2][1] *= 0.99f;
		p_.World[2][0] *= 0.99f;
		p_.World[2][2] *= 0.99f;

		p_.RenderData.RGBA.W *= 0.97f;
		
		p_.Life -= 1.0f;
	}
}