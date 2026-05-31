module Game.Scene.InGame;

import : Impl;
import : Impl.Effect;

import Lumina.Main;

namespace Game::Scene::Impl {
	// * BGM一時停止
	template<>
	auto InGame::Update_<"BGM.Pause">() -> void {
		ResourceManager_->Audio().Pause(BGMPlayerHandle_);
	}
	// * BGM再開
	template<>
	auto InGame::Update_<"BGM.Resume">() -> void {
		ResourceManager_->Audio().Resume(BGMPlayerHandle_);
	}
	// * BGM音量調整
	template<>
	auto InGame::Update_<"BGM.AdjustVolume">(float&& volume_) -> void {
		ResourceManager_->Audio().SetVolume(BGMPlayerHandle_, volume_);

		// * 音量の値の取得は
		// float volume = ResourceManager_->Audio().GetVolume(BGMPlayerHandle_);
	}

	template<>
	auto InGame::Update_<"PlaySE">(
		AUDIO_STREAM_ID&& audioStreamID_,
		float&& volume_
	) -> void {
		auto const& audoStreamHandle{ AudioStreamHandles_[static_cast<Lumina::U32>(audioStreamID_)] };
		ResourceManager_->Audio().Play(audoStreamHandle, false, volume_);
	}

	// InGame.OnActionへ移動
	template<>
	auto InGame::Update_<"SE.PlayerAttack">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_ATTACK, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerJump">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_JUMP, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.AttackedByEnemy">() -> void {
	}

	template<>
	auto InGame::Update_<"SE.EnemyShotBullet">() -> void {
	}

	template<>
	auto InGame::Update_<"SE.PlayerAttackCombo1">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_ATTACKCOMBO1, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerAttackCombo2">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_ATTACKCOMBO2, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerAttackCombo3">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_ATTACKCOMBO3, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerAttackRot">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_ATTACKROT, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerAttackJump">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_ATTACKJUMP, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerFlying">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_FLYING, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerCharge">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_CHARGE, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerChargeAttack">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_CHARGEATTACK, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerWarp">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_WARP, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerThrowUmbrella">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_THROWUMBRELLA, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerGainXp">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_GAINXP, std::move(volume_));
	}

	template<>
	auto InGame::Update_<"SE.PlayerLevelUp">(float&& volume_) -> void {
		Update_<"PlaySE">(AUDIO_STREAM_ID::PLAYER_LEVELUP, std::move(volume_));
	}
}