module Game.Editor.EnemyEditor;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import <string>;
import <filesystem>;

namespace fs = std::filesystem;

namespace Game::Editor {
	void EnemyEditor::Update() {
#if defined(_DEBUG)
		DrawEditorUI();
#endif
	}

#if defined(_DEBUG)
	void EnemyEditor::DrawEditorUI() {
		// 左カラム: JSONファイル一覧
		ImGui::SetNextWindowPos(ImVec2(0, 18), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(250, 702), ImGuiCond_Always);
		ImGui::Begin("Asset Browser", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::TextDisabled("JSON FILES");
		ImGui::Separator();
		ImGui::BeginChild("FileList", ImVec2(0, 0), false);
		if (fs::exists("./")) {
			for (const auto& entry : fs::directory_iterator("./")) {
				if (entry.path().extension() == ".json") {
					std::string fName = entry.path().filename().string();
					if (fName.find("area") == 0) continue;
					bool isSelected = (editingEnemy_.name + ".json" == fName);
					if (ImGui::Selectable(fName.c_str(), isSelected)) {
						LoadEnemy(editingEnemy_, fName);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::End();

		// 右カラム: インスペクター
		ImGui::SetNextWindowPos(ImVec2(1280.0f - 350.0f, 18.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(350, 700), ImGuiCond_Always);
		ImGui::Begin("Enemy Inspector", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		if (ImGui::CollapsingHeader("Base Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
			char nameBuf[256];
			strncpy_s(nameBuf, editingEnemy_.name.c_str(), sizeof(nameBuf));
			if (ImGui::InputText("Enemy Name", nameBuf, sizeof(nameBuf))) {
				editingEnemy_.name = nameBuf;
			}
			ImGui::DragInt("Health Points", &editingEnemy_.hp, 1, 0, 9999);
			ImGui::DragFloat("Attack Power", &editingEnemy_.power, 0.05f, 0.0f, 10.0f, "%.2f");
		}

		if (ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen)) {
			char pathBuf[256];
			strncpy_s(pathBuf, editingEnemy_.gltfPath.c_str(), sizeof(pathBuf));
			if (ImGui::InputText("Model Path (GLTF)", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
				editingEnemy_.gltfPath = pathBuf;
			} else if (ImGui::IsItemDeactivatedAfterEdit()) {
				editingEnemy_.gltfPath = pathBuf;
			}
		}

		if (ImGui::CollapsingHeader("Animation Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
			std::string keyToDelete;
			std::string keyToRenameOld, keyToRenameNew;

			for (auto& [action, anim] : editingEnemy_.animationMap) {
				ImGui::PushID(action.c_str());
				char keyBuf[256];
				strncpy_s(keyBuf, action.c_str(), sizeof(keyBuf));
				ImGui::SetNextItemWidth(120.0f);
				ImGui::InputText("##action", keyBuf, sizeof(keyBuf));
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					if (action != keyBuf) {
						keyToRenameOld = action;
						keyToRenameNew = keyBuf;
					}
				}

				ImGui::SameLine();
				char animBuf[256];
				strncpy_s(animBuf, anim.c_str(), sizeof(animBuf));
				ImGui::SetNextItemWidth(150.0f);
				if (ImGui::InputText("##anim", animBuf, sizeof(animBuf))) {
					anim = animBuf;
				}

				ImGui::SameLine();
				if (ImGui::Button("X")) {
					keyToDelete = action;
				}
				ImGui::PopID();
			}

			if (!keyToDelete.empty()) {
				editingEnemy_.animationMap.erase(keyToDelete);
			}
			if (!keyToRenameOld.empty() && !keyToRenameNew.empty()) {
				std::string val = editingEnemy_.animationMap[keyToRenameOld];
				editingEnemy_.animationMap.erase(keyToRenameOld);
				editingEnemy_.animationMap[keyToRenameNew] = val;
			}

			if (ImGui::Button("Add Animation Map", ImVec2(-1, 30))) {
				std::string baseName = "New_Action";
				std::string newName = baseName;
				int count = 1;
				while (editingEnemy_.animationMap.find(newName) != editingEnemy_.animationMap.end()) {
					newName = baseName + std::to_string(count);
					count++;
				}
				editingEnemy_.animationMap[newName] = "";
			}
		}

		ImGui::Separator();
		ImGui::Spacing();
		if (ImGui::Button("SAVE ASSET", ImVec2(-1, 40))) {
			SaveEnemy(editingEnemy_);
		}
		ImGui::End();
	}
#endif
}
