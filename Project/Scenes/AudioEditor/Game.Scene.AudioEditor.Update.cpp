module Game.Editor.AudioEditor;

import <string>;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
import Lumina.Main;
import Lumina.ResourceManager;
#endif

namespace Game::Editor {

	void AudioEditor::Update() {
#if defined(_DEBUG)
		DrawEditorUI();
#endif
	}

#if defined(_DEBUG)
	void AudioEditor::DrawEditorUI() {
		ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Scene BGM Editor")) {
			char nameBuf[256];
			strncpy_s(nameBuf, editingAudio_.name.c_str(), sizeof(nameBuf));
			if (ImGui::InputText("Config Name", nameBuf, sizeof(nameBuf))) {
				editingAudio_.name = nameBuf;
			}

			// スペースキーでプレビューの再生/停止トグル
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Space) && !previewSceneName_.empty()) {
				if (isPreviewPlaying_) {
					Lumina::Context::Instance().ResourceContext().Audio().Stop(previewPlayerHandle_);
					isPreviewPlaying_ = false;
				} else {
					auto& config = editingAudio_.bgmMap[previewSceneName_];
					if (!config.filePath.empty()) {
						auto& audioContext = Lumina::Context::Instance().ResourceContext().Audio();
						auto stream = audioContext.LoadFromFile(config.filePath);
						previewPlayerHandle_ = audioContext.Play(stream, config.isLoop, config.volume);
						isPreviewPlaying_ = true;
					}
				}
			}

			ImGui::Separator();
			ImGui::TextDisabled("Scene BGMs");

			std::string keyToDelete;

			for (auto& [sceneName, bgmData] : editingAudio_.bgmMap) {
				ImGui::PushID(sceneName.c_str());
				
				bool isSelected = (previewSceneName_ == sceneName);
				if (ImGui::Selectable(sceneName.c_str(), isSelected, 0, ImVec2(150, 0))) {
					previewSceneName_ = sceneName;
					// 別のシーンを選択した場合はプレビューを一旦止める
					if (isPreviewPlaying_) {
						Lumina::Context::Instance().ResourceContext().Audio().Stop(previewPlayerHandle_);
						isPreviewPlaying_ = false;
					}
				}
				
				ImGui::SameLine(ImGui::GetWindowWidth() - 50.0f);
				if (ImGui::Button("X")) {
					keyToDelete = sceneName;
				}

				ImGui::Indent();
				
				char pathBuf[256];
				strncpy_s(pathBuf, bgmData.filePath.c_str(), sizeof(pathBuf));
				ImGui::PushItemWidth(250.0f);
				if (ImGui::InputText("BGM Path", pathBuf, sizeof(pathBuf))) {
					bgmData.filePath = pathBuf;
				}
				ImGui::PopItemWidth();

				if (ImGui::SliderFloat("Volume", &bgmData.volume, 0.0f, 2.0f)) {
					// 音量変更中で、現在このシーンがプレビュー中なら再生ましにして音量変更を即時反映(簡易的)
					if (isSelected && isPreviewPlaying_) {
						Lumina::Context::Instance().ResourceContext().Audio().Stop(previewPlayerHandle_);
						if (!bgmData.filePath.empty()) {
							auto& audioContext = Lumina::Context::Instance().ResourceContext().Audio();
							auto stream = audioContext.LoadFromFile(bgmData.filePath);
							previewPlayerHandle_ = audioContext.Play(stream, bgmData.isLoop, bgmData.volume);
						}
					}
				}

				ImGui::Checkbox("Loop Playback", &bgmData.isLoop);
				
				if (isSelected) {
					ImGui::SameLine();
					if (isPreviewPlaying_) {
						ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "(Playing... Press Space to Stop)");
					} else {
						ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "(Press Space to Play)");
					}
				}

				ImGui::Unindent();
				ImGui::Spacing();
				ImGui::PopID();
			}

			if (!keyToDelete.empty()) {
				editingAudio_.bgmMap.erase(keyToDelete);
				if (previewSceneName_ == keyToDelete) {
					previewSceneName_ = "";
					if (isPreviewPlaying_) {
						Lumina::Context::Instance().ResourceContext().Audio().Stop(previewPlayerHandle_);
					}
					isPreviewPlaying_ = false;
				}
			}

			ImGui::Separator();

			static char newSceneBuf[256] = "";
			ImGui::InputText("New Scene Name", newSceneBuf, sizeof(newSceneBuf));
			ImGui::SameLine();
			if (ImGui::Button("Add Scene")) {
				std::string newScene = newSceneBuf;
				if (!newScene.empty() && editingAudio_.bgmMap.find(newScene) == editingAudio_.bgmMap.end()) {
					editingAudio_.bgmMap[newScene] = SceneBGMData{};
					newSceneBuf[0] = '\0';
				}
			}

			ImGui::Spacing();
			ImGui::Separator();

			if (ImGui::Button("Save Asset", ImVec2(120, 30))) {
				SaveAudio(editingAudio_);
			}
			
			ImGui::SameLine();
			
			if (ImGui::Button("Load Asset", ImVec2(120, 30))) {
				LoadAudio(editingAudio_, editingAudio_.name + ".json");
			}

		}
		ImGui::End();
	}
#else
	void AudioEditor::DrawEditorUI() {}
#endif
}
