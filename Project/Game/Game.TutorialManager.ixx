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
		Lumina::Math::F32x2 TextPosition{ 400.0f, 550.0f };
		Lumina::Math::F32x2 TextSize{ 480.0f, 120.0f };

		// 進行条件
		enum class Trigger : uint8_t {
			PadButton,     // 特定のパッドボタンを押す
			KeyPress,      // 特定のキーを押す
			AnyInput,      // 何か入力があれば進む
			Auto,          // 一定時間後に自動進行
			MoveDuration,  // 一定時間移動入力をし続けたら進行
		} trigger{ Trigger::AnyInput };

		uint16_t RequiredPadButton{ 0U };   // Trigger::PadButton の場合
		uint16_t RequiredKey{ 0U };         // 対応するキーボードのキー
		float AutoDuration{ 3.0f };         // Trigger::Auto / MoveDuration の場合（秒）

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

	/// チュートリアルマネージャー
	export class TutorialManager {
	public:
		void Initialize();

		/// チュートリアルのシーケンス（手順群）を登録
		void RegisterSequences();

		/// 指定されたシーケンスIDのチュートリアルを開始（未完了の場合のみ）
		/// @return 実際に開始されたらtrue
		bool TryStartSequence(const std::string& sequenceId);

		/// 強制的に指定のシーケンスを開始する
		void StartSequence(const std::string& sequenceId);

		/// チュートリアルを強制終了
		void Skip();

		/// 毎フレーム更新
		void Update(float deltaTime);

		/// PrimitiveManagerでオーバーレイ描画（NDC空間）
		void RenderOverlay(Lumina::PrimitiveManager& primMngr);

		/// アクティブかどうか
		bool IsActive() const noexcept { return Active_; }

		/// 完了済みかどうか
		bool IsCompleted() const noexcept { return Completed_; }

		/// 現在のステップで許可されている入力フラグを取得
		uint16_t GetAllowedInputs() const noexcept;

		// セッション内で表示済みかのフラグ群
		std::unordered_set<std::string> CompletedSequences_;

	private:
		/// オーバーレイ用の暗い矩形を4つバッチ（ハイライト領域をくり抜き）
		void BatchDarkOverlay(Lumina::PrimitiveManager& primMngr, float alpha);

		/// ハイライト枠線をバッチ
		void BatchHighlightBorder(Lumina::PrimitiveManager& primMngr, float pulse);

	private:
		std::unordered_map<std::string, std::vector<TutorialStep>> Sequences_;

		std::vector<TutorialStep> Steps_;
		int CurrentStep_{ -1 };
		float Timer_{ 0.0f };
		float OverlayAlpha_{ 0.0f };     // フェードイン用 0→0.7
		float PulseTimer_{ 0.0f };        // ハイライト枠のパルス
		bool Active_{ false };
		bool Completed_{ false };

		// テクスチャ情報（InGameから設定される）
	public:
		uint32_t TutorialTextureStartIndex{ 0U };
		uint32_t TutorialTextureCount{ 0U };
	};
}
