module Game.Scene.Result;

import : Impl;

import <cmath>;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.Scene;

namespace Game::Scene::Impl {
	void Result::Update() {
		AnimationTimer_ += 1.0f / 60.0f;
		DisplayTimer_ += 1.0f / 60.0f;

		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		auto const& pad{ inputMngr.Pad() };
		using Lumina::OS::Windows::KEY;

		// 一定時間経過後に入力受付（演出表示中の誤操作防止）
		if (!IsReturnRequested_ && DisplayTimer_ > 2.0f) {
			if (keyboard.IsJustPressed(KEY::ENTER) ||
				keyboard.IsJustPressed(KEY::SPACE) ||
				pad.IsHold(0x1000)) {
				IsReturnRequested_ = true;
				FadeAlpha_ = 0.0f;
			}
		}

		// フェードアウト処理 → Title シーンへ遷移
		if (IsReturnRequested_) {
			FadeAlpha_ += 1.0f / 60.0f;
			if (FadeAlpha_ >= 1.0f) {
				FadeAlpha_ = 1.0f;
				// タイトルシーンへ遷移
				auto& sceneMngr{ Lumina::SceneManager::Instance() };
				sceneMngr.Load<"Title">();
				sceneMngr.Activate("Title");
				return;
			}
		}

#if defined(_DEBUG)
		ImGui::SetNextWindowPos(ImVec2(300, 100), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(640, 500), ImGuiCond_FirstUseEver);
		ImGui::Begin("Result Scene");

		ImGui::Text("=== RESULT ===");
		ImGui::Separator();
		ImGui::Spacing();

		// リザルト情報の表示（段階的に出現）
		if (DisplayTimer_ > 0.3f) {
			ImGui::TextColored(ImVec4{ 1.0f, 0.85f, 0.2f, 1.0f }, "Stage Clear!");
			ImGui::Spacing();
		}

		if (DisplayTimer_ > 0.8f) {
			ImGui::Text("Clear Time:  --:--");
		}

		if (DisplayTimer_ > 1.2f) {
			ImGui::Text("Enemies Defeated:  ---");
		}

		if (DisplayTimer_ > 1.6f) {
			ImGui::Text("Damage Taken:  ---");
		}

		if (DisplayTimer_ > 2.0f) {
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// 点滅表示
			float blinkVal = std::sin(AnimationTimer_ * 3.0f);
			if (blinkVal > 0.0f) {
				ImGui::TextColored(ImVec4{ 0.5f, 1.0f, 0.5f, 1.0f }, ">> Press Enter / A Button to Return to Title <<");
			}
			else {
				ImGui::TextColored(ImVec4{ 0.5f, 1.0f, 0.5f, 0.2f }, ">> Press Enter / A Button to Return to Title <<");
			}
		}

		if (IsReturnRequested_) {
			ImGui::Spacing();
			ImGui::ProgressBar(FadeAlpha_, ImVec2(-1, 0), "Returning to Title...");
		}

		ImGui::End();
#endif
	}
}

namespace Game::Scene {
	void Result::Update() {
		Impl_->Update();
	}
}
