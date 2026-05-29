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
}