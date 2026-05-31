module;

#include <Windows.h>

module Game.TutorialManager;

import <cmath>;
import <fstream>;
import <filesystem>;
import <algorithm>;

import nlohmann.json;

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
		LocationTriggers_.clear();
	}

	// ==============================
	//  キー名→KEY列挙値変換
	// ==============================
	uint16_t TutorialManager::KeyNameToCode(const std::string& keyName) {
		using KEY = Lumina::OS::Windows::KEY;
		// 一文字キー (A-Z, 0-9)
		if (keyName.size() == 1) {
			char c = keyName[0];
			if (c >= 'A' && c <= 'Z') return static_cast<uint16_t>(KEY::A) + (c - 'A');
			if (c >= 'a' && c <= 'z') return static_cast<uint16_t>(KEY::A) + (c - 'a');
			if (c >= '0' && c <= '9') return static_cast<uint16_t>(KEY::NUM_0) + (c - '0');
		}
		// 特殊キー
		if (keyName == "SPACE")     return static_cast<uint16_t>(KEY::SPACE);
		if (keyName == "ENTER")     return static_cast<uint16_t>(KEY::ENTER);
		if (keyName == "ESC")       return static_cast<uint16_t>(KEY::ESC);
		if (keyName == "SHIFT")     return static_cast<uint16_t>(KEY::SHIFT);
		if (keyName == "CTRL")      return static_cast<uint16_t>(KEY::CTRL);
		if (keyName == "ARROW_UP")  return static_cast<uint16_t>(KEY::ARROW_UP);
		if (keyName == "ARROW_DOWN") return static_cast<uint16_t>(KEY::ARROW_DOWN);
		if (keyName == "ARROW_LEFT") return static_cast<uint16_t>(KEY::ARROW_LEFT);
		if (keyName == "ARROW_RIGHT") return static_cast<uint16_t>(KEY::ARROW_RIGHT);
		return 0U;
	}

	// ==============================
	//  trigger文字列→Trigger列挙値変換
	// ==============================
	TutorialStep::Trigger TutorialManager::TriggerFromString(const std::string& str) {
		if (str == "PadButton")     return TutorialStep::Trigger::PadButton;
		if (str == "KeyPress")      return TutorialStep::Trigger::KeyPress;
		if (str == "AnyInput")      return TutorialStep::Trigger::AnyInput;
		if (str == "Auto")          return TutorialStep::Trigger::Auto;
		if (str == "MoveDuration")  return TutorialStep::Trigger::MoveDuration;
		if (str == "GuardDuration") return TutorialStep::Trigger::GuardDuration;
		if (str == "AimDuration")   return TutorialStep::Trigger::AimDuration;
		if (str == "AreaExit")      return TutorialStep::Trigger::AreaExit;
		if (str == "ShootDuration") return TutorialStep::Trigger::ShootDuration;
		return TutorialStep::Trigger::AnyInput; // デフォルト
	}

	// ==============================
	//  allowedInputs文字列→ビットフラグ変換
	// ==============================
	uint16_t TutorialManager::AllowedInputsFromStrings(const std::vector<std::string>& inputs) {
		using AI = TutorialStep::AllowedInput;
		uint16_t flags = 0;
		for (const auto& s : inputs) {
			if (s == "All")      return AI::Input_All;
			if (s == "Move")     flags |= AI::Input_Move;
			if (s == "Jump")     flags |= AI::Input_Jump;
			if (s == "Attack")   flags |= AI::Input_Attack;
			if (s == "Sheathe")  flags |= AI::Input_Sheathe;
			if (s == "Guard")    flags |= AI::Input_Guard;
			if (s == "Reverse")  flags |= AI::Input_Reverse;
			if (s == "Aim")      flags |= AI::Input_Aim;
			if (s == "Shoot")    flags |= AI::Input_Shoot;
			if (s == "Repair")   flags |= AI::Input_Repair;
			if (s == "Mana")     flags |= AI::Input_Mana;
		}
		return flags == 0 ? AI::Input_All : flags;
	}

	// ==============================
	//  JSONからチュートリアルデータを読み込み
	// ==============================
	void TutorialManager::LoadFromJSON(const std::string& jsonPath) {
		Sequences_.clear();
		EventToSequence_.clear();
		LocationTriggers_.clear();
		TextureFiles_.clear();
		TextureNameToIndex_.clear();

		if (!std::filesystem::exists(jsonPath)) {
			return; // ファイルなしの場合は空状態で安全に動作
		}

		std::ifstream file(jsonPath);
		if (!file.is_open()) return;

		nlohmann::json root = nlohmann::json::parse(file, nullptr, false);
		if (root.is_discarded()) return; // パース失敗時も安全に動作

		// デフォルト表示位置・サイズ
		if (root.contains("defaultTextPosition") && root["defaultTextPosition"].is_array()) {
			auto& pos = root["defaultTextPosition"];
			if (pos.size() >= 2) {
				DefaultTextPosition_ = { pos[0].get<float>(), pos[1].get<float>() };
			}
		}
		if (root.contains("defaultTextSize") && root["defaultTextSize"].is_array()) {
			auto& sz = root["defaultTextSize"];
			if (sz.size() >= 2) {
				DefaultTextSize_ = { sz[0].get<float>(), sz[1].get<float>() };
			}
		}

		if (!root.contains("sequences") || !root["sequences"].is_object()) return;

		auto& sequences = root["sequences"];
		for (auto it = sequences.begin(); it != sequences.end(); ++it) {
			std::string seqId = it.key();
			auto& seqData = it.value();

			// イベント→シーケンスマッピング
			if (seqData.contains("triggerEvent") && seqData["triggerEvent"].is_string()) {
				std::string eventName = seqData["triggerEvent"].get<std::string>();
				EventToSequence_[eventName] = seqId;
			}

			if (!seqData.contains("steps") || !seqData["steps"].is_array()) continue;

			std::vector<TutorialStep> steps;

			for (auto& stepData : seqData["steps"]) {
				TutorialStep step;

				// テクスチャ
				if (stepData.contains("texture") && stepData["texture"].is_string()) {
					std::string texName = stepData["texture"].get<std::string>();

					// テクスチャリストに未登録なら追加
					if (TextureNameToIndex_.find(texName) == TextureNameToIndex_.end()) {
						uint32_t idx = static_cast<uint32_t>(TextureFiles_.size());
						TextureFiles_.push_back(texName);
						TextureNameToIndex_[texName] = idx;
					}
					step.TextureIndex = TextureNameToIndex_[texName];
				}

				// 表示位置（省略時はデフォルト値）
				step.TextPosition = DefaultTextPosition_;
				step.TextSize = DefaultTextSize_;

				if (stepData.contains("textPosition") && stepData["textPosition"].is_array()) {
					auto& pos = stepData["textPosition"];
					if (pos.size() >= 2) {
						step.TextPosition = { pos[0].get<float>(), pos[1].get<float>() };
					}
				}
				if (stepData.contains("textSize") && stepData["textSize"].is_array()) {
					auto& sz = stepData["textSize"];
					if (sz.size() >= 2) {
						step.TextSize = { sz[0].get<float>(), sz[1].get<float>() };
					}
				}

				// トリガー
				if (stepData.contains("trigger") && stepData["trigger"].is_string()) {
					step.trigger = TriggerFromString(stepData["trigger"].get<std::string>());
				}

				// パッドボタン
				if (stepData.contains("padButton")) {
					step.RequiredPadButton = stepData["padButton"].get<uint16_t>();
				}

				// キーボードキー
				if (stepData.contains("key") && stepData["key"].is_string()) {
					step.RequiredKey = KeyNameToCode(stepData["key"].get<std::string>());
				}

				// 持続時間
				if (stepData.contains("duration")) {
					step.AutoDuration = stepData["duration"].get<float>();
				}

				// エリアインデックス（AreaExit用）
				if (stepData.contains("areaIndex")) {
					step.RequiredAreaIndex = stepData["areaIndex"].get<int>();
				}

				// 許可入力
				if (stepData.contains("allowedInputs") && stepData["allowedInputs"].is_array()) {
					std::vector<std::string> inputStrs;
					for (auto& v : stepData["allowedInputs"]) {
						if (v.is_string()) inputStrs.push_back(v.get<std::string>());
					}
					step.AllowedInputs = AllowedInputsFromStrings(inputStrs);
				}

				// ハイライトは固定（なし）
				step.HighlightCenter = { 0.0f, 0.0f };
				step.HighlightSize = { 0.0f, 0.0f };

				steps.push_back(step);
			}

			Sequences_[seqId] = std::move(steps);
		}

		// 地点イベントトリガーの読み込み
		if (root.contains("locationTriggers") && root["locationTriggers"].is_array()) {
			for (auto& triggerData : root["locationTriggers"]) {
				LocationTrigger trigger;
				if (triggerData.contains("area") && triggerData["area"].is_number()) {
					trigger.AreaIndex = triggerData["area"].get<int>();
				}
				if (triggerData.contains("position") && triggerData["position"].is_array()) {
					auto& pos = triggerData["position"];
					if (pos.size() >= 2) {
						trigger.Position.X = pos[0].get<float>();
						trigger.Position.Y = pos[1].get<float>();
						trigger.Position.Z = pos.size() >= 3 ? pos[2].get<float>() : 0.0f;
					}
				}
				if (triggerData.contains("radius") && triggerData["radius"].is_number()) {
					trigger.Radius = triggerData["radius"].get<float>();
				}
				if (triggerData.contains("event") && triggerData["event"].is_string()) {
					trigger.EventName = triggerData["event"].get<std::string>();
				}
				LocationTriggers_.push_back(trigger);
			}
		}
	}

	// ==============================
	//  イベント発火
	// ==============================
	bool TutorialManager::FireEvent(const std::string& eventName) {
		auto it = EventToSequence_.find(eventName);
		if (it == EventToSequence_.end()) return false;
		return TryStartSequence(it->second);
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

	void TutorialManager::AdvanceStep() {
		if (!Active_ || CurrentStep_ < 0 || CurrentStep_ >= static_cast<int>(Steps_.size())) {
			return;
		}

		CurrentStep_++;
		Timer_ = 0.0f;

		if (CurrentStep_ >= static_cast<int>(Steps_.size())) {
			// チュートリアル完了
			if (!ActiveSequenceId_.empty()) {
				CompletedSequences_.insert(ActiveSequenceId_);
			}
			Active_ = false;
			ActiveSequenceId_.clear();
		}
	}

	bool TutorialManager::TryStartSequence(const std::string& sequenceId) {
		if (CompletedSequences_.find(sequenceId) != CompletedSequences_.end()) {
			return false;
		}

		// 1. 新しいシーケンスの優先度（高か低か）を判定
		bool isNewHighPriority = false;
		std::string triggerEvent;
		for (auto const& [evt, seq] : EventToSequence_) {
			if (seq == sequenceId) {
				triggerEvent = evt;
				break;
			}
		}
		if (triggerEvent.rfind("area_enter_", 0) == 0 || triggerEvent == "umbrella_throw_ready") {
			isNewHighPriority = true;
		}

		// 2. 現在アクティブなシーケンスがある場合
		if (Active_) {
			// 現在表示中のシーケンスの優先度を判定
			bool isActiveHighPriority = false;
			std::string activeTriggerEvent;
			for (auto const& [evt, seq] : EventToSequence_) {
				if (seq == ActiveSequenceId_) {
					activeTriggerEvent = evt;
					break;
				}
			}
			if (activeTriggerEvent.rfind("area_enter_", 0) == 0 || activeTriggerEvent == "umbrella_throw_ready") {
				isActiveHighPriority = true;
			}

			// 新しいのが「高」で、今表示中なのが「低」なら、上書き（強制割り込み）を許可
			if (isNewHighPriority && !isActiveHighPriority) {
				StartSequence(sequenceId);
				return true;
			}

			// それ以外の組み合わせ（高対高、低対高、低対低）は割り込み却下
			return false;
		}

		// 再生中のものがなければそのまま開始！
		StartSequence(sequenceId);
		return true;
	}

	void TutorialManager::Update(float deltaTime, int currentAreaIndex) {
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

		case TutorialStep::Trigger::AimDuration:
			{
				using Lumina::OS::Windows::KEY;
				bool isAiming = (pad.GetLeftTrigger() > 10) || keyboard.IsPressed(KEY::K);
				if (isAiming) {
					Timer_ += deltaTime;
				}
				if (Timer_ >= step.AutoDuration) {
					shouldAdvance = true;
				}
			}
			break;

		case TutorialStep::Trigger::AreaExit:
			if (currentAreaIndex != -1 && currentAreaIndex != step.RequiredAreaIndex) {
				shouldAdvance = true;
			}
			break;

		case TutorialStep::Trigger::ShootDuration:
			{
				using Lumina::OS::Windows::KEY;
				// pad.GetRightTrigger() > 100 or L key
				bool isShooting = (pad.GetRightTrigger() > 100) || keyboard.IsPressed(KEY::L);
				if (isShooting) {
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
			// ハイライトなし: テキスト画像と同じ高さの水平ラインだけ黒く（テキスト領域のコントラスト用）
			float const sy = step.TextPosition.Y;
			float const sh = step.TextSize.Y;

			// スクリーン座標→NDCのY座標
			float const y0 = 1.0f - (sy / 360.0f);        // 帯の上端
			float const y1 = 1.0f - ((sy + sh) / 360.0f); // 帯の下端

			Lumina::F32x4 thinDark{ 0.0f, 0.0f, 0.0f, alpha * 0.95f };
			primMngr.BatchTriangle(
				{ { -1.0f,  y0, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, WhiteTextureIndex },
				{ {  1.0f,  y0, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, WhiteTextureIndex },
				{ { -1.0f,  y1, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, WhiteTextureIndex }
			);
			primMngr.BatchTriangle(
				{ {  1.0f,  y0, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, WhiteTextureIndex },
				{ {  1.0f,  y1, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, WhiteTextureIndex },
				{ { -1.0f,  y1, 0.0f, 1.0f }, thinDark, { 0.0f, 0.0f }, WhiteTextureIndex }
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
				{ { x0, y1, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, WhiteTextureIndex },
				{ { x1, y1, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, WhiteTextureIndex },
				{ { x0, y0, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, WhiteTextureIndex }
			);
			primMngr.BatchTriangle(
				{ { x1, y1, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, WhiteTextureIndex },
				{ { x1, y0, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, WhiteTextureIndex },
				{ { x0, y0, 0.0f, 1.0f }, darkColor, { 0.0f, 0.0f }, WhiteTextureIndex }
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
			{ { x0, y0, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, WhiteTextureIndex },
			{ { x1, y0, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, WhiteTextureIndex }
		);
		primMngr.BatchLine(
			{ { x1, y0, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, WhiteTextureIndex },
			{ { x1, y1, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, WhiteTextureIndex }
		);
		primMngr.BatchLine(
			{ { x1, y1, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, WhiteTextureIndex },
			{ { x0, y1, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, WhiteTextureIndex }
		);
		primMngr.BatchLine(
			{ { x0, y1, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, WhiteTextureIndex },
			{ { x0, y0, 0.0f, 1.0f }, borderColor, { 0.0f, 0.0f }, WhiteTextureIndex }
		);
	}

	void TutorialManager::ResetProgress() {
		CompletedSequences_.clear();
		for (auto& trigger : LocationTriggers_) {
			trigger.Triggered = false;
		}
		Active_ = false;
		Completed_ = false;
		CurrentStep_ = -1;
		Steps_.clear();
		ActiveSequenceId_.clear();
	}

	void TutorialManager::UpdateLocationTriggers(int currentAreaIndex, const Lumina::Math::F32x3& playerPosition) {
		for (auto& trigger : LocationTriggers_) {
			if (trigger.Triggered) continue;
			if (trigger.AreaIndex != currentAreaIndex) continue;

			// X, Y平面上での距離チェック
			float dx = trigger.Position.X - playerPosition.X;
			float dy = trigger.Position.Y - playerPosition.Y;
			float distanceSq = dx * dx + dy * dy;
			if (distanceSq <= trigger.Radius * trigger.Radius) {
				trigger.Triggered = true;
				FireEvent(trigger.EventName);
			}
		}
	}
}
