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

		if (ImGui::CollapsingHeader("AI Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextDisabled("Detection & Combat");
			ImGui::DragFloat("Aggro Radius", &editingEnemy_.aggroRadius, 0.5f, 0.0f, 100.0f, "%.1f");
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enemy detection range");
			ImGui::DragFloat("Attack Range", &editingEnemy_.attackRange, 0.1f, 0.0f, 50.0f, "%.1f");
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance at which enemy can attack");
			ImGui::DragFloat("Attack Cooldown", &editingEnemy_.attackCooldown, 0.05f, 0.0f, 10.0f, "%.2f s");
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Time between attacks (seconds)");

			ImGui::Spacing();
			ImGui::TextDisabled("Movement");
			ImGui::DragFloat("Move Speed", &editingEnemy_.moveSpeed, 0.1f, 0.0f, 30.0f, "%.1f");
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Movement speed");
			ImGui::DragFloat("Patrol Radius", &editingEnemy_.patrolRadius, 0.5f, 0.0f, 100.0f, "%.1f");
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Radius of idle patrol area");

			ImGui::Spacing();
			ImGui::TextDisabled("Behavior");
			ImGui::SliderFloat("Retreat Threshold", &editingEnemy_.retreatThreshold, 0.0f, 1.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("HP ratio at which enemy will retreat (0 = never)");
			ImGui::SliderFloat("Aggressiveness", &editingEnemy_.aggressiveness, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("0 = passive, 1 = always attacks on sight");
		}

		if (ImGui::CollapsingHeader("Animation Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
			// gltfPathが変わったらアニメーション名を再抽出
			if (cachedGltfPath_ != editingEnemy_.gltfPath) {
				cachedGltfPath_ = editingEnemy_.gltfPath;
				cachedAnimationNames_ = ExtractAnimationNames(editingEnemy_.gltfPath);
			}

			if (!cachedAnimationNames_.empty()) {
				ImGui::TextDisabled("Animations from: %s (%d found)",
					editingEnemy_.gltfPath.c_str(),
					static_cast<int>(cachedAnimationNames_.size()));
			} else {
				ImGui::TextDisabled("No animations found (check Model Path)");
			}
			ImGui::Spacing();

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

				// アニメーション名をコンボボックスで選択
				if (!cachedAnimationNames_.empty()) {
					int currentIdx = -1;
					for (int k = 0; k < static_cast<int>(cachedAnimationNames_.size()); ++k) {
						if (cachedAnimationNames_[k] == anim) {
							currentIdx = k;
							break;
						}
					}
					std::string preview = anim.empty() ? "(none)" : anim;
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo("##anim", preview.c_str())) {
						// 「なし」の選択肢
						if (ImGui::Selectable("(none)", anim.empty())) {
							anim = "";
						}
						for (int k = 0; k < static_cast<int>(cachedAnimationNames_.size()); ++k) {
							bool isSelected = (currentIdx == k);
							if (ImGui::Selectable(cachedAnimationNames_[k].c_str(), isSelected)) {
								anim = cachedAnimationNames_[k];
							}
							if (isSelected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				} else {
					// フォールバック: 手入力
					char animBuf[256];
					strncpy_s(animBuf, anim.c_str(), sizeof(animBuf));
					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::InputText("##anim", animBuf, sizeof(animBuf))) {
						anim = animBuf;
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("X")) {
					keyToDelete = action;
				}
				ImGui::PopID();
			}

			if (!keyToDelete.empty()) {
				editingEnemy_.animationMap.erase(keyToDelete);
				editingEnemy_.motionMap.erase(keyToDelete);
			}
			if (!keyToRenameOld.empty() && !keyToRenameNew.empty()) {
				std::string val = editingEnemy_.animationMap[keyToRenameOld];
				editingEnemy_.animationMap.erase(keyToRenameOld);
				editingEnemy_.animationMap[keyToRenameNew] = val;

				// motionMapも同期してリネーム
				std::string motionVal = editingEnemy_.motionMap[keyToRenameOld];
				editingEnemy_.motionMap.erase(keyToRenameOld);
				editingEnemy_.motionMap[keyToRenameNew] = motionVal;
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
				editingEnemy_.motionMap[newName] = "";
			}
		}

		if (ImGui::CollapsingHeader("Motion Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextDisabled("Action -> Motion (coordinate movement)");
			ImGui::Spacing();

			// Assets/Data/Motion/ 内のモーションファイルをスキャン
			std::vector<std::string> motionFiles;
			motionFiles.push_back("");  // 「なし」の選択肢
			const std::string motionDir = "Assets/Data/Motion/";
			if (fs::exists(motionDir)) {
				for (const auto& entry : fs::directory_iterator(motionDir)) {
					if (entry.is_regular_file() && entry.path().extension() == ".json") {
						motionFiles.push_back(entry.path().stem().string());
					}
				}
			}

			// animationMapの各アクションに対してモーション選択コンボを表示
			for (auto& [action, anim] : editingEnemy_.animationMap) {
				// motionMapにキーがなければ空文字で初期化
				if (editingEnemy_.motionMap.find(action) == editingEnemy_.motionMap.end()) {
					editingEnemy_.motionMap[action] = "";
				}
				std::string& currentMotion = editingEnemy_.motionMap[action];

				ImGui::PushID(("motion_" + action).c_str());

				// 現在の選択を探す
				int currentIdx = 0;
				for (int k = 0; k < static_cast<int>(motionFiles.size()); ++k) {
					if (motionFiles[k] == currentMotion) {
						currentIdx = k;
						break;
					}
				}

				std::string label = action;
				std::string preview = currentMotion.empty() ? "(none)" : currentMotion;
				if (ImGui::BeginCombo(label.c_str(), preview.c_str())) {
					for (int k = 0; k < static_cast<int>(motionFiles.size()); ++k) {
						bool isSelected = (currentIdx == k);
						std::string itemLabel = motionFiles[k].empty() ? "(none)" : motionFiles[k];
						if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
							currentMotion = motionFiles[k];
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::PopID();
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
