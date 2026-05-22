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

		int PlayerIdleEffectEmitFrameCount{ 0 };
		int PlayerMoveEffectEmitFrameCount{ 0 };
		int PlayerJumpEffectEmitFrameCount{ 0 };
		int PlayerAttackEffectEmitFrameCount{ 0 };

		Lumina::F32 PlayerEffectTimeFactor{ 0.0f };

		Lumina::Math::F32x4x4<> World_Hands[2]{};
		Lumina::Math::F32x4x4<> World_Feet[2]{};

		Lumina::Math::F32x4x4<> World_Hip{};
		
		Lumina::Math::F32x3 WorldPos_UmbrellaRoot{};
		Lumina::Math::F32x3 WorldPos_UmbrellaTip{};

		using Effect::RGB_Gaming;
		using Effect::RNDEngine;
	}

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

		WorldPos_UmbrellaRoot = Player_->GetUmbrella().handle_->GetBaseJoint()->GetWorldPos();
		WorldPos_UmbrellaTip = Player_->GetUmbrella().handle_->GetTipJoint()->GetWorldPos();

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

				p.Scale.X = 1.0f;
				p.Scale.Y = 1.0f;

				p.Rotate.Z = RNDEngine() * Inv_0xFFFFFFFF * Pi * 2.0f;

				p.Life = 64.0f;

				p.RenderData.RGBA = {
					RGB_Gaming.R * 0.9f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
					RGB_Gaming.G * 0.9f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
					RGB_Gaming.B * 0.9f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
					0.7875f
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

				p_Move.Velocity.X = Effect::PlayerVelocity.X * (-0.015625f);
				p_Move.Velocity.Y = p_Move.Translate.X * (-0.5f);

				auto const& world_Foot{ World_Feet[i][3] };
				p_Move.Translate.X += world_Foot.X();
				p_Move.Translate.Y += world_Foot.Y();
				p_Move.Translate.Z += world_Foot.Z();

				p_Move.Scale.X = 0.625f;
				p_Move.Scale.Y = 0.625f;

				p_Move.Rotate.Z = RNDEngine() * Inv_0xFFFFFFFF * Pi * 2.0f;

				p_Move.Life = 32.0f;

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

				p_Move.Life = 24.0f;

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
	auto InGame::Update_<"PlayerEffectParticle">(Lumina::Particle& p_) -> void {
		p_.Translate.X += p_.Velocity.X;
		p_.Translate.Y += p_.Velocity.Y;
		p_.RenderData.RGBA.W *= 0.95f;
		p_.Scale.X *= 0.93f;
		p_.Scale.Y *= 0.93f;
		p_.Rotate.Z += p_.Velocity.Z * 0.01f;
		p_.Life -= 1.0f;
	}

	template<>
	auto InGame::Update_<"UmbrellaEffect.Perpetual">() -> void {
		auto tipEffect{
			[&, this](Lumina::I32 i_) {
				Lumina::Particle p{};
				{
					p.Translate = {
						std::cos(PlayerEffectTimeFactor * 0.14f + i_ * 2.4f) * 0.15f,
						std::sin(PlayerEffectTimeFactor * 0.13f - i_ * 3.6f) * 0.15f,
						std::sin(PlayerEffectTimeFactor * 0.12f - i_ * 1.2f) * 0.15f
					};

					p.Velocity.X = p.Translate.X * (-0.05f);
					p.Velocity.Y = p.Translate.Y * (-0.05f);
					p.Velocity.Z = p.Translate.Z * (-0.05f);

					p.Translate.X += WorldPos_UmbrellaTip.X;
					p.Translate.Y += WorldPos_UmbrellaTip.Y;
					p.Translate.Z += WorldPos_UmbrellaTip.Z;

					p.Scale.X = 0.75f;
					p.Scale.Y = 0.75f;

					p.Rotate.Z = RNDEngine() * Inv_0xFFFFFFFF * Pi * 2.0f;

					p.Life = 32.0f;

					p.RenderData.RGBA = {
						RGB_Gaming.R * 0.3f + 0.8f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.G * 0.2f + 0.3f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.B * 0.2f + 0.3f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						0.375f
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

		auto tipEffect{
			[&, this]() {
				Lumina::Particle p{};
				{
					p.Translate.X += WorldPos_UmbrellaRoot.X;
					p.Translate.Y += WorldPos_UmbrellaRoot.Y;
					p.Translate.Z += WorldPos_UmbrellaRoot.Z;

					p.Scale.X = 15.0f;
					p.Scale.Y = 15.0f;

					p.Life = 32.0f;

					p.RenderData.RGBA = {
						RGB_Gaming.R * 0.3f + 0.8f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.G * 0.2f + 0.3f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						RGB_Gaming.B * 0.2f + 0.3f + RNDEngine() * Inv_0xFFFFFFFF * 0.05f,
						0.75f
					};
					p.RenderData.DiffuseID = 1U;
					p.RenderData.DiffuseAtlasID = 0U;
					UmbrellaEffects_->Emit(std::move(p));
				}
			}
		};

		tipEffect();

		--PlayerAttackEffectEmitFrameCount;
	}

	template<>
	auto InGame::Update_<"UmbrellaEffectParticle">(Lumina::Particle& p_) -> void {
		p_.Translate.X += p_.Velocity.X;
		p_.Translate.Y += p_.Velocity.Y;
		p_.RenderData.RGBA.W *= 0.97f;
		p_.Scale.X *= 0.93f;
		p_.Scale.Y *= 0.93f;
		p_.Rotate.Z += p_.Velocity.Z * 0.02f;
		p_.Life -= 1.0f;
	}

	template<>
	void InGame::Update_<"OnPlayerMove">(Event::InGame::OnPlayerMove& event_) {
		PlayerMoveEffectEmitFrameCount = 6;
		Effect::PlayerVelocity = event_.Velocity;
	}

	template<>
	void InGame::Update_<"OnPlayerJump">(
		[[maybe_unused]] Event::InGame::OnPlayerJump& event_
	) {
		PlayerJumpEffectEmitFrameCount = 12;
	}

	template<>
	void InGame::Update_<"OnPlayerAttack">(
		[[maybe_unused]] Event::InGame::OnPlayerAttack& event_
	) {
		PlayerAttackEffectEmitFrameCount = 2;
	}
}