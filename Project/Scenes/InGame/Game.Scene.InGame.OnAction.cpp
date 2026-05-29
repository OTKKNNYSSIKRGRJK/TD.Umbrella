module Game.Scene.InGame;

import : Impl;
import : Impl.Effect;

import Game.Events.InGame;
import Lumina.Utils.Color;
import Lumina.Core.Math;

namespace Game::Scene::Impl {
	template<>
	void InGame::Update_<"プレイヤー移動">(Event::InGame::OnPlayerMove& event_) {
		Effect::PlayerMoveEffectEmitFrameCount = 6;
		Effect::PlayerVelocity = event_.Velocity;
	}

	template<>
	void InGame::Update_<"OnPlayerJump">(
		[[maybe_unused]] Event::InGame::OnPlayerJump& event_
	) {
		Effect::PlayerJumpEffectEmitFrameCount = 12;

		//float a = 0.75f;
		//this->Update_<"SE.PlayerJump">(float{ a });
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