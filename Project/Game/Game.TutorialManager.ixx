export module Game.TutorialManager;

import <vector>;
import <string>;
import <unordered_map>;
import <unordered_set>;
import <cstdint>;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Primitive;
import Lumina.D3D12;

namespace Game {
	/// チュートリアルの1ステップ
	export struct TutorialStep {
		// ハイライト領域（スクリーン座標 NDC: -1~+1）
		// ハイライトなしの場合は Size = {0,0}
		Lumina::Math::F32x2 HighlightCenter{ 0.0f, 0.0f };
		Lumina::Math::F32x2 HighlightSize{ 0.0f, 0.0f };

		// テキスト画像テクスチャのインデックス（テクスチャ配列内）
		uint32_t TextureIndex{ 0U };

		// テキスト画像の表示位置とサイズ（スクリーン座標 0~1280, 0~720）
		Lumina::Math::F32x2 TextPosition{ 400.0f, 470.0f };
		Lumina::Math::F32x2 TextSize{ 480.0f, 120.0f };

		// 進行条件
		enum class Trigger : uint8_t {
			PadButton,     // 特定のパッドボタンを押す
			KeyPress,      // 特定のキーを押す
			AnyInput,      // 何か入力があれば進む
			Auto,          // 一定時間後に自動進行
			MoveDuration,  // 一定時間移動入力をし続けたら進行
			GuardDuration, // 傘開き（R2 / Iキー）を一定時間続けたら進行
			AimDuration,   // 照準（L2 / Kキー）を一定時間続けたら進行
			AreaExit,      // 特定のエリアから退出したら進行
			ShootDuration, // 射出（R2 / Lキー）を一定時間続けたら進行
		} trigger{ Trigger::AnyInput };

		uint16_t RequiredPadButton{ 0U };   // Trigger::PadButton の場合
		uint16_t RequiredKey{ 0U };         // 対応するキーボードのキー
		float AutoDuration{ 3.0f };         // Trigger::Auto / MoveDuration の場合（秒）
		int RequiredAreaIndex{ -1 };        // Trigger::AreaExit の場合

		// 許可する入力のビットフラグ
		enum AllowedInput : uint16_t {
			Input_None     = 0,
			Input_Move     = 1 << 0,  // 左スティック移動
			Input_Jump     = 1 << 1,  // ジャンプ
			Input_Attack   = 1 << 2,  // 攻撃
			Input_Sheathe  = 1 << 3,  // 納刀/抜刀
			Input_Guard    = 1 << 4,  // ガード
			Input_Reverse  = 1 << 5,  // リバース
			Input_Aim      = 1 << 6,  // 照準
			Input_Shoot    = 1 << 7,  // 射撃
			Input_Repair   = 1 << 8,  // 修復
			Input_Mana     = 1 << 9,  // マナ使用
			Input_All      = 0xFFFF,  // 全入力許可
		};
		uint16_t AllowedInputs{ Input_All };
	};

	/// 地点イベントトリガー情報
	export struct LocationTrigger {
		int AreaIndex{ 0 };
		Lumina::Math::F32x3 Position{ 0.0f, 0.0f, 0.0f };
		float Radius{ 50.0f };
		std::string EventName{};
		bool Triggered{ false };
	};

	/// チュートリアルマネージャー
	export class TutorialManager {
	public:
		void Initialize();

		/// JSONファイルからチュートリアルシーケンスを読み込む
		/// @param jsonPath JSONファイルのパス（例: "Assets/Data/Tutorial/tutorial_sequences.json"）
		void LoadFromJSON(const std::string& jsonPath);

		/// イベントを発火し、対応するシーケンスがあれば開始する
		/// @param eventName イベント名（例: "area_enter_0", "first_enemy_near"）
		/// @return 実際にチュートリアルが開始されたらtrue
		bool FireEvent(const std::string& eventName);

		/// 指定されたシーケンスIDのチュートリアルを開始（未完了の場合のみ）
		/// @return 実際に開始されたらtrue
		bool TryStartSequence(const std::string& sequenceId);

		/// 強制的に指定のシーケンスを開始する
		void StartSequence(const std::string& sequenceId);

		/// チュートリアルを強制終了
		void Skip();

		/// 毎フレーム更新
		void Update(float deltaTime, int currentAreaIndex = -1);

		/// 地点トリガーの進捗を含め、すべての進行状況をリセットする
		void ResetProgress();

		/// プレイヤーの位置情報に基づいて特定の地点に入った時にイベントを自動的に発火する
		/// @param currentAreaIndex 現在のエリアインデックス
		/// @param playerPosition プレイヤーの位置座標
		void UpdateLocationTriggers(int currentAreaIndex, const Lumina::Math::F32x3& playerPosition);

		/// PrimitiveManagerでオーバーレイ描画（NDC空間）
		void RenderOverlay(Lumina::PrimitiveManager& primMngr);

		/// アクティブかどうか
		bool IsActive() const noexcept { return Active_; }

		/// 手動で現在のステップを進める
		void AdvanceStep();

		/// 現在のステップを取得
		int GetCurrentStep() const noexcept { return CurrentStep_; }

		/// 現在アクティブなシーケンスIDを取得
		const std::string& GetActiveSequenceId() const noexcept { return ActiveSequenceId_; }

		/// 現在のステップの経過時間を取得
		float GetTimer() const noexcept { return Timer_; }

		/// 完了済みかどうか
		bool IsCompleted() const noexcept { return Completed_; }

		/// 現在のステップで許可されている入力フラグを取得
		uint16_t GetAllowedInputs() const noexcept;

		/// JSONから収集したテクスチャファイル名リスト（重複なし、順序保持）
		const std::vector<std::string>& GetTextureFiles() const noexcept { return TextureFiles_; }

		// セッション内で表示済みかのフラグ群
		std::unordered_set<std::string> CompletedSequences_;

	private:
		/// オーバーレイ用の暗い矩形を4つバッチ（ハイライト領域をくり抜き）
		void BatchDarkOverlay(Lumina::PrimitiveManager& primMngr, float alpha);

		/// ハイライト枠線をバッチ
		void BatchHighlightBorder(Lumina::PrimitiveManager& primMngr, float pulse);

		/// キー名文字列をKEY列挙値に変換
		static uint16_t KeyNameToCode(const std::string& keyName);

		/// trigger文字列をTrigger列挙値に変換
		static TutorialStep::Trigger TriggerFromString(const std::string& str);

		/// allowedInputs文字列配列をビットフラグに変換
		static uint16_t AllowedInputsFromStrings(const std::vector<std::string>& inputs);

	private:
		std::unordered_map<std::string, std::vector<TutorialStep>> Sequences_;
		std::unordered_map<std::string, std::string> EventToSequence_; // イベント名 → シーケンスID
		std::vector<LocationTrigger> LocationTriggers_;
		std::string ActiveSequenceId_{};

		std::vector<TutorialStep> Steps_;
		int CurrentStep_{ -1 };
		float Timer_{ 0.0f };
		float OverlayAlpha_{ 0.0f };     // フェードイン用 0→0.7
		float PulseTimer_{ 0.0f };        // ハイライト枠のパルス
		bool Active_{ false };
		bool Completed_{ false };

		// JSONから収集したテクスチャファイル名と名前→インデックスマップ
		std::vector<std::string> TextureFiles_;
		std::unordered_map<std::string, uint32_t> TextureNameToIndex_;

		// デフォルト表示位置・サイズ（JSONから読み込み）
		Lumina::Math::F32x2 DefaultTextPosition_{ 400.0f, 470.0f };
		Lumina::Math::F32x2 DefaultTextSize_{ 480.0f, 120.0f };

		// テクスチャ情報（InGameから設定される）
	public:
		uint32_t TutorialTextureStartIndex{ 0U };
		uint32_t TutorialTextureCount{ 0U };
		uint32_t WhiteTextureIndex{ 0U };
	};
}
