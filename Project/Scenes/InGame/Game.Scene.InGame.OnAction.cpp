module Game.Scene.InGame;

import : Impl;
import : Impl.Effect;

import Game.Events.InGame;
import Lumina.Utils.Color;
import Lumina.Core.Math;

namespace Game::Scene::Impl {
	// 中身の設定
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
		this->Update_<"SE.PlayerWarp">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerAttackCombo1">(
		[[maybe_unused]] Event::InGame::OnPlayerAttackCombo1& event_
	) {
		this->Update_<"SE.PlayerAttackCombo1">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerAttackCombo2">(
		[[maybe_unused]] Event::InGame::OnPlayerAttackCombo2& event_
	) {
		this->Update_<"SE.PlayerAttackCombo2">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerAttackCombo3">(
		[[maybe_unused]] Event::InGame::OnPlayerAttackCombo3& event_
	) {
		this->Update_<"SE.PlayerAttackCombo3">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerAttackRot">(
		[[maybe_unused]] Event::InGame::OnPlayerAttackRot& event_
	) {
		this->Update_<"SE.PlayerAttackRot">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerAttackJump">(
		[[maybe_unused]] Event::InGame::OnPlayerAttackJump& event_
	) {
		this->Update_<"SE.PlayerAttackJump">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerFlying">(
		[[maybe_unused]] Event::InGame::OnPlayerFlying& event_
	) {
		this->Update_<"SE.PlayerFlying">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerCharge">(
		[[maybe_unused]] Event::InGame::OnPlayerCharge& event_
	) {
		this->Update_<"SE.PlayerCharge">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerChargeAttack">(
		[[maybe_unused]] Event::InGame::OnPlayerChargeAttack& event_
	) {
		this->Update_<"SE.PlayerChargeAttack">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerThrowUmbrella">(
		[[maybe_unused]] Event::InGame::OnPlayerThrowUmbrella& event_
	) {
		this->Update_<"SE.PlayerThrowUmbrella">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerGainXp">(
		[[maybe_unused]] Event::InGame::OnPlayerGainXp& event_
	) {
		this->Update_<"SE.PlayerGainXp">(0.75f);
	}

	template<>
	void InGame::Update_<"OnPlayerLevelUp">(
		[[maybe_unused]] Event::InGame::OnPlayerLevelUp& event_
	) {
		this->Update_<"SE.PlayerLevelUp">(0.75f);
	}
}