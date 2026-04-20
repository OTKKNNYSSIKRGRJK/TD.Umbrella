module Game.Scene.Title;

import : Impl;

import <cmath>;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.Scene;

namespace Game::Scene::Impl {
	void Title::Update() {
		AnimationTimer_ += 1.0f / 60.0f;
		BlinkTimer_ += 1.0f / 60.0f;

		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		auto const& pad{ inputMngr.Pad() };
		using Lumina::OS::Windows::KEY;

		// スタート入力の検出（Enter キー、スペースキー、パッドの A ボタン）
		if (!IsStartRequested_) {
			if (keyboard.IsJustPressed(KEY::ENTER) ||
				keyboard.IsJustPressed(KEY::SPACE) ||
				pad.IsHold(0x1000)) {
				IsStartRequested_ = true;
				FadeAlpha_ = 0.0f;
			}
		}

		// フェードアウト処理 → InGame シーンへ遷移
		if (IsStartRequested_) {
			FadeAlpha_ += 1.0f / 60.0f;
			if (FadeAlpha_ >= 1.0f) {
				FadeAlpha_ = 1.0f;
				// シーン遷移
				auto& sceneMngr{ Lumina::SceneManager::Instance() };
				sceneMngr.Activate("InGame");
				return;
			}
		}

#if defined(_DEBUG)
		ImGui::SetNextWindowPos(ImVec2(400, 200), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(480, 320), ImGuiCond_FirstUseEver);
		ImGui::Begin("Title Scene");

		ImGui::Text("=== TITLE SCREEN ===");
		ImGui::Separator();
		ImGui::Text("Game Title: TD.Umbrella");
		ImGui::Spacing();

		// 点滅表示 "Press Start"
		float blinkVal = std::sin(BlinkTimer_ * 3.0f);
		if (blinkVal > 0.0f) {
			ImGui::TextColored(ImVec4{ 1.0f, 1.0f, 0.3f, 1.0f }, ">> Press Enter / A Button to Start <<");
		}
		else {
			ImGui::TextColored(ImVec4{ 1.0f, 1.0f, 0.3f, 0.2f }, ">> Press Enter / A Button to Start <<");
		}

		ImGui::Spacing();
		ImGui::Separator();

		if (IsStartRequested_) {
			ImGui::ProgressBar(FadeAlpha_, ImVec2(-1, 0), "Transitioning...");
		}

		ImGui::Text("Animation Timer: %.2f", AnimationTimer_);

		ImGui::End();
#endif
	}
}

namespace Game::Scene {
	void Title::Update() {
		Impl_->Update();
	}
}
