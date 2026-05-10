module;

#include <Windows.h>

module Game.TutorialManager;

import <cmath>;
import <fstream>;
import <filesystem>;

import Lumina.Main;
import Lumina.OS.Windows.RawInput;

namespace Game {
	void TutorialManager::Initialize() {
		Steps_.clear();
		CurrentStep_ = -1;
		Timer_ = 0.0f;
		OverlayAlpha_ = 0.0f;
		PulseTimer_ = 0.0f;
		Active_ = false;
		Completed_ = false;
	}

	void TutorialManager::RegisterSequences() {
		Sequences_.clear();

		using AI = TutorialStep::AllowedInput;
		std::vector<TutorialStep> basicControls;

		// ステップ1: 移動（左スティック）— 移動のみ許可
		{
			TutorialStep step;
			step.TextureIndex = 0U;
			step.TextPosition = { 400.0f, 530.0f };
			step.TextSize = { 480.0f, 120.0f };
			step.trigger = TutorialStep::Trigger::MoveDuration;
			step.AutoDuration = 2.0f; // 2秒間移動入力したら完了
			step.HighlightCenter = { 0.0f, 0.0f };
			step.HighlightSize = { 0.0f, 0.0f };
			step.AllowedInputs = AI::Input_Move;
			basicControls.push_back(step);
		}

		// ステップ2: ジャンプ（Aボタン / SPACE）— 移動＋ジャンプのみ許可
		{
			TutorialStep step;
			step.TextureIndex = 1U;
			step.TextPosition = { 400.0f, 530.0f };
			step.TextSize = { 480.0f, 120.0f };
			step.trigger = TutorialStep::Trigger::PadButton;
			step.RequiredPadButton = 0x1000; // Aボタン (XINPUT_GAMEPAD_A)
			step.RequiredKey = static_cast<uint16_t>(Lumina::OS::Windows::KEY::SPACE); // スペースキー
			step.HighlightCenter = { 0.0f, 0.0f };
			step.HighlightSize = { 0.0f, 0.0f };
			step.AllowedInputs = AI::Input_Move | AI::Input_Jump;
			basicControls.push_back(step);
		}

		// ステップ3: 抜刀・攻撃（Yボタン / J）— 攻撃のみ許可（移動やジャンプ不可）
		{
			TutorialStep step;
			step.TextureIndex = 2U;
			step.TextPosition = { 400.0f, 530.0f };
			step.TextSize = { 480.0f, 120.0f };
			step.trigger = TutorialStep::Trigger::PadButton;
			step.RequiredPadButton = 0x8000; // Yボタン
			step.RequiredKey = static_cast<uint16_t>(Lumina::OS::Windows::KEY::J); // Jキー
			step.HighlightCenter = { 0.0f, 0.0f };
			step.HighlightSize = { 0.0f, 0.0f };
			step.AllowedInputs = AI::Input_Attack;
			basicControls.push_back(step);
		}
		
		Sequences_["BasicControls"] = basicControls;

		std::vector<TutorialStep> parachute;
		{
			TutorialStep step;
			step.TextureIndex = 3U; // rakkasan.png
			step.TextPosition = { 400.0f, 530.0f };
			step.TextSize = { 480.0f, 120.0f };
			step.trigger = TutorialStep::Trigger::GuardDuration;
			step.AutoDuration = 2.0f; // 2秒間ガードボタン（傘開く）
			step.HighlightCenter = { 0.0f, 0.0f };
			step.HighlightSize = { 0.0f, 0.0f };
			step.AllowedInputs = AI::Input_All; // 制限なし
			parachute.push_back(step);
		}
		Sequences_["Parachute"] = parachute;
	}

	void TutorialManager::StartSequence(const std::string& sequenceId) {
		auto it = Sequences_.find(sequenceId);
		if (it == Sequences_.end() || it->second.empty()) return;
		
		ActiveSequenceId_ = sequenceId;
		Steps_ = it->second;
		CurrentStep_ = 0;
		Timer_ = 0.0f;
		OverlayAlpha_ = 0.0f;
		PulseTimer_ = 0.0f;
		Active_ = true;
		Completed_ = false;
	}

	void TutorialManager::Skip() {
		Active_ = false;
		Completed_ = true;
		CurrentStep_ = -1;
		OverlayAlpha_ = 0.0f;
		ActiveSequenceId_.clear();
	}

	bool TutorialManager::TryStartSequence(const std::string& sequenceId) {
		if (CompletedSequences_.find(sequenceId) == CompletedSequences_.end()) {
			StartSequence(sequenceId);
			return true;
		}
		return false;
	}

	void TutorialManager::Update(float deltaTime) {
		if (!Active_ || CurrentStep_ < 0 || CurrentStep_ >= static_cast<int>(Steps_.size())) {
			return;
		}

		// フェードイン
		if (OverlayAlpha_ < 0.65f) {
			OverlayAlpha_ += deltaTime * 2.0f;
			if (OverlayAlpha_ > 0.65f) OverlayAlpha_ = 0.65f;
		}

		// パルスタイマー更新
		PulseTimer_ += deltaTime * 3.0f;

		auto const& step = Steps_[CurrentStep_];

		// 進行条件チェック
		bool shouldAdvance = false;

		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		auto const& pad{ inputMngr.Pad() };

		switch (step.trigger) {
		case TutorialStep::Trigger::PadButton:
			Timer_ += deltaTime;
			if (pad.IsHold(step.RequiredPadButton) || (step.RequiredKey != 0U && keyboard.IsPressed(static_cast<Lumina::OS::Windows::KEY>(step.RequiredKey)))) {
				shouldAdvance = true;
			}
			break;

		case TutorialStep::Trigger::KeyPress:
			// 汎用キー判定（将来用）
			break;

		case TutorialStep::Trigger::AnyInput:
			{
				using Lumina::OS::Windows::KEY;
				if (pad.IsHold(0x1000) || pad.IsHold(0x2000) ||
					pad.IsHold(0x4000) || pad.IsHold(0x8000) ||
					keyboard.IsJustPressed(KEY::SPACE) ||
					keyboard.IsJustPressed(KEY::ENTER)) {
					shouldAdvance = true;
				}
			}
			break;

		case TutorialStep::Trigger::Auto:
			Timer_ += deltaTime;
			if (Timer_ >= step.AutoDuration) {
				shouldAdvance = true;
			}
			break;

		case TutorialStep::Trigger::MoveDuration:
			{
				using Lumina::OS::Windows::KEY;
				bool isMoving = (std::abs(pad.GetLeftStickX()) > 0.15f) ||
								(std::abs(pad.GetLeftStickY()) > 0.15f) ||
								keyboard.IsPressed(KEY::W) ||
								keyboard.IsPressed(KEY::A) ||
								keyboard.IsPressed(KEY::S) ||
								keyboard.IsPressed(KEY::D);
				if (isMoving) {
					Timer_ += deltaTime;
				}
				if (Timer_ >= step.AutoDuration) {
					shouldAdvance = true;
				}
			}
			break;

		case TutorialStep::Trigger::GuardDuration:
			{
				using Lumina::OS::Windows::KEY;
				bool isGuarding = (pad.GetRightTrigger() > 10) || keyboard.IsPressed(KEY::I);
				if (isGuarding) {
					Timer_ += deltaTime;
				}
				if (Timer_ >= step.AutoDuration) {
					shouldAdvance = true;
				}
			}
			break;
		}

		if (shouldAdvance) {
			CurrentStep_++;
			Timer_ = 0.0f;

			if (CurrentStep_ >= static_cast<int>(Steps_.size())) {
				// チュートリアル完了
				if (!ActiveSequenceId_.empty()) {
					CompletedSequences_.insert(ActiveSequenceId_);
					ActiveSequenceId_.clear();
				}
				Active_ = false;
				Completed_ = true;
				OverlayAlpha_ = 0.0f;
				return;
			}
		}
	}

	uint16_t TutorialManager::GetAllowedInputs() const noexcept {
		if (!Active_ || CurrentStep_ < 0 || CurrentStep_ >= static_cast<int>(Steps_.size())) {
			return TutorialStep::Input_All;
		}
		return Steps_[CurrentStep_].AllowedInputs;
	}

	void TutorialManager::RenderOverlay(Lumina::PrimitiveManager& primMngr) {
		if (!Active_ || CurrentStep_ < 0 || CurrentStep_ >= static_cast<int>(Steps_.size())) {
			return;
		}

		float const alpha = OverlayAlpha_;
		if (alpha <= 0.001f) return;

		float const pulse = 0.5f + 0.5f * std::sin(PulseTimer_);

		BatchDarkOverlay(primMngr, alpha);

		auto const& step = Steps_[CurrentStep_];
		if (step.HighlightSize.X > 0.001f && step.HighlightSize.Y > 0.001f) {
			BatchHighlightBorder(primMngr, pulse);
		}

		// テキスト画像を矩形として描画
		// NDC空間に変換: screenX → NDC.X = (x / 640) - 1, screenY → NDC.Y = 1 - (y / 360)
		{
			float const sx = step.TextPosition.X;
			float const sy = step.TextPosition.Y;
			float const sw = step.TextSize.X;
			float const sh = step.TextSize.Y;

			// スクリーン座標→NDC
			float const x0 = (sx / 640.0f) - 1.0f;
			float const y0 = 1.0f - (sy / 360.0f);
			float const x1 = ((sx + sw) / 640.0f) - 1.0f;
			float const y1 = 1.0f - ((sy + sh) / 360.0f);

			// テキスト背景（暗い半透明パネル）
			float const panelAlpha = alpha * 0.85f;
			Lumina::F32x4 panelColor{ 0.02f, 0.02f, 0.05f, panelAlpha };

			primMngr.BatchTriangle(
				{ { x0, y0, 0.0f, 1.0f }, panelColor, { 0.0f, 0.0f }, 0U },
				{ { x1, y0, 0.0f, 1.0f }, panelColor, { 1.0f, 0.0f }, 0U },
				{ { x0, y1, 0.0f, 1.0f }, panelColor, { 0.0f, 1.0f }, 0U }
			);
			primMngr.BatchTriangle(
				{ { x1, y0, 0.0f, 1.0f }, panelColor, { 1.0f, 0.0f }, 0U },
				{ { x1, y1, 0.0f, 1.0f }, panelColor, { 1.0f, 1.0f }, 0U },
				{ { x0, y1, 0.0f, 1.0f }, panelColor, { 0.0f, 1.0f }, 0U }
			);

			// テキスト画像（テクスチャ付き）
			uint32_t texID = TutorialTextureStartIndex + step.TextureIndex;
			Lumina::F32x4 texColor{ 1.0f, 1.0f, 1.0f, alpha * 1.2f };

			primMngr.BatchTriangle(
				{ { x0, y0, 0.0f, 1.0f }, texColor, { 0.0f, 0.0f }, texID },
				{ { x1, y0, 0.0f, 1.0f }, texColor, { 1.0f, 0.0f }, texID },
				{ { x0, y1, 0.0f, 1.0f }, texColor, { 0.0f, 1.0f }, texID }
			);
			primMngr.BatchTriangle(
				{ { x1, y0, 0.0f, 1.0f }, texColor, { 1.0f, 0.0f }, texID },
				{ { x1, y1, 0.0f, 1.0f }, texColor, { 1.0f, 1.0f }, texID },
				{ { x0, y1, 0.0f, 1.0f }, texColor, { 0.0f, 1.0f }, texID }
			);
		}
	}

	void TutorialManager::BatchDarkOverlay(Lumina::PrimitiveManager& primMngr, float alpha) {
		auto const& step = Steps_[CurrentStep_];
		Lumina::F32x4 darkColor{ 0.0f, 0.0f, 0.0f, alpha };

		bool hasHighlight = (step.HighlightSize.X > 0.001f && step.HighlightSize.Y > 0.001f);

		if (!hasHighlight) {
			// ハイライトなし: 画面上部だけ軽く暗く（テキスト領域のコントラスト用）
			// 上部のみ薄いオーバーレイ
			Lumina::F32x4 thinDark{ 0.0f, 0.0f, 0.0f, alpha * 0.3f };
			primMngr.BatchTriangle(
				{ { -1.0f,  1.0f, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, 0U },
				{ {  1.0f,  1.0f, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, 0U },
				{ { -1.0f, -0.4f, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, 0U }
			);
			primMngr.BatchTriangle(
				{ {  1.0f,  1.0f, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, 0U },
				{ {  1.0f, -0.4f, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, 0U },
				{ { -1.0f, -0.4f, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, 0U }
			);
			return;
		}

		// ハイライト領域をくり抜いた4矩形
		float const cx = step.HighlightCenter.X;
		float const cy = step.HighlightCenter.Y;
		float const hw = step.HighlightSize.X * 0.5f;
		float const hh = step.HighlightSize.Y * 0.5f;

		float const hx0 = cx - hw;
		float const hx1 = cx + hw;
		float const hy0 = cy - hh;
		float const hy1 = cy + hh;

		auto batchQuad = [&](float x0, float y0, float x1, float y1) {
			primMngr.BatchTriangle(
				{ { x0, y1, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, 0U },
				{ { x1, y1, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, 0U },
				{ { x0, y0, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, 0U }
			);
			primMngr.BatchTriangle(
				{ { x1, y1, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, 0U },
				{ { x1, y0, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, 0U },
				{ { x0, y0, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, 0U }
			);
		};

		// 上
		batchQuad(-1.0f, hy1, 1.0f, 1.0f);
		// 下
		batchQuad(-1.0f, -1.0f, 1.0f, hy0);
		// 左
		batchQuad(-1.0f, hy0, hx0, hy1);
		// 右
		batchQuad(hx1, hy0, 1.0f, hy1);
	}

	void TutorialManager::BatchHighlightBorder(Lumina::PrimitiveManager& primMngr, float pulse) {
		auto const& step = Steps_[CurrentStep_];
		float const cx = step.HighlightCenter.X;
		float const cy = step.HighlightCenter.Y;
		float const hw = step.HighlightSize.X * 0.5f;
		float const hh = step.HighlightSize.Y * 0.5f;

		float const x0 = cx - hw;
		float const x1 = cx + hw;
		float const y0 = cy - hh;
		float const y1 = cy + hh;

		// パルスする明るい枠線の色
		float const brightness = 0.5f + 0.5f * pulse;
		Lumina::F32x4 borderColor{ 0.3f * brightness, 0.7f * brightness, 1.0f * brightness, 0.9f };

		// 4辺のライン
		primMngr.BatchLine(
			{ { x0, y0, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, 0U },
			{ { x1, y0, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, 0U }
		);
		primMngr.BatchLine(
			{ { x1, y0, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, 0U },
			{ { x1, y1, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, 0U }
		);
		primMngr.BatchLine(
			{ { x1, y1, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, 0U },
			{ { x0, y1, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, 0U }
		);
		primMngr.BatchLine(
			{ { x0, y1, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, 0U },
			{ { x0, y0, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, 0U }
		);
	}
}
