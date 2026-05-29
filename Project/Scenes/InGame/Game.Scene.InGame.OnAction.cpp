module Game.Scene.InGame;

import : Impl;
import : Impl.Effect;

import Game.Events.InGame;
import Lumina.Utils.Color;
import Lumina.Core.Math;

namespace Game::Scene::Impl {
	template<>
	void InGame::Update_<"OnPlayerMove">(Event::InGame::OnPlayerMove& event_) {
		Effect::PlayerMoveEffectEmitFrameCount = 6;
		Effect::PlayerVelocity = event_.Velocity;
	}

	template<>
	void InGame::Update_<"OnPlayerJump">(
		[[maybe_unused]] Event::InGame::OnPlayerJump& event_
	) {
		Effect::PlayerJumpEffectEmitFrameCount = 12;

		//this->Update_<"SE.PlayerJump">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerAttack">(
		[[maybe_unused]] Event::InGame::OnPlayerAttack& event_
	) {
		Effect::PlayerAttackEffectEmitFrameCount = 2;

		//this->Update_<"SE.PlayerAttack">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerWarp">(
		[[maybe_unused]] Event::InGame::OnPlayerWarp& event_
	) {
		Effect::PlayerWarpEffectEmitFrameCount = 3;
	}
}