export module Game.Events;

import Lumina.Core.Math;

export namespace Game::Event {
	inline int CameraShakingTimer = 0;
	inline void OnAttack() {
		CameraShakingTimer = 15;
	}
	inline Lumina::Math::F32x3 RespawnPos = {};

	// ==============================
	//  ヒットストップ
	// ==============================
	inline float HitStopTimer = 0.0f;
	inline void AddHitStop(float duration) {
		if (duration > HitStopTimer) {
			HitStopTimer = duration;
		}
	}

	// ==============================
	//  ポーズ・リスタート
	// ==============================
	inline bool IsPaused = false;
	inline bool IsRestartRequested = false;

	// ==============================
	//  ゲームフェーズ管理
	// ==============================
	enum class GamePhase {
		Startup,    // 開始演出 (カウントダウンなど)
		InBattle,   // 戦闘中
		Win,        // 勝利
		Lose        // 敗北
	};
	inline GamePhase CurrentPhase = GamePhase::InBattle;
	inline float PhaseTimer = 0.0f;       // フェーズ内タイマー
	inline float StartupDuration = 3.0f;  // スタートアップカウントダウン秒数
	inline int EnemiesDefeated = 0;       // リザルト用: 撃破数
	inline float ElapsedBattleTime = 0.0f;// リザルト用: 戦闘経過時間

	// ==============================
	//  落下リスポーン
	// ==============================
	inline float FallDeathThresholdY = -30.0f;  // この Y 座標以下で落下死判定
	inline int FallDeathCount = 0;              // リザルト用: 落下回数

	// ==============================
	//  ポーズ用パッド前回状態
	// ==============================
	inline bool PrevPadStart = false;
}