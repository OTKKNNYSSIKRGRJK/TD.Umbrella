module Game.Editor.EnemyEditor;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import <string>;
import <filesystem>;
import <array>;
import <vector>;
import <algorithm>;

namespace fs = std::filesystem;

#if defined(_DEBUG)
namespace {
	constexpr ImU32 MakeCol32(int r, int g, int b, int a) {
		return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r);
	}

	using Vec2 = std::pair<float, float>;

	float CrossProd(const Vec2& p0, const Vec2& p1, const Vec2& p2) {
		return (p1.first - p0.first) * (p2.second - p0.second)
		     - (p1.second - p0.second) * (p2.first - p0.first);
	}

	bool IsConvex(const std::vector<Game::Editor::CollisionVertex>& verts) {
		if (verts.size() < 3) return false;
		int n = static_cast<int>(verts.size());
		bool hasPos = false, hasNeg = false;
		for (int i = 0; i < n; ++i) {
			float c = CrossProd(
				{ verts[i].x, verts[i].y },
				{ verts[(i+1)%n].x, verts[(i+1)%n].y },
				{ verts[(i+2)%n].x, verts[(i+2)%n].y });
			if (c > 0) hasPos = true;
			if (c < 0) hasNeg = true;
			if (hasPos && hasNeg) return false;
		}
		return true;
	}

	bool PtInTri(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c) {
		float d1 = CrossProd(a, b, p);
		float d2 = CrossProd(b, c, p);
		float d3 = CrossProd(c, a, p);
		return !((d1 < 0 || d2 < 0 || d3 < 0) && (d1 > 0 || d2 > 0 || d3 > 0));
	}

	float SignedArea(const std::vector<Vec2>& poly) {
		float a = 0;
		int n = static_cast<int>(poly.size());
		for (int i = 0; i < n; ++i) {
			int j = (i + 1) % n;
			a += poly[i].first * poly[j].second - poly[j].first * poly[i].second;
		}
		return a * 0.5f;
	}

	std::vector<std::array<int, 3>> Triangulate(
		const std::vector<Game::Editor::CollisionVertex>& iv)
	{
		std::vector<std::array<int, 3>> tris;
		int n = static_cast<int>(iv.size());
		if (n < 3) return tris;

		std::vector<int> idx(n);
		std::vector<Vec2> poly(n);
		for (int i = 0; i < n; ++i) poly[i] = { iv[i].x, iv[i].y };

		if (SignedArea(poly) > 0) {
			for (int i = 0; i < n; ++i) idx[i] = i;
		} else {
			for (int i = 0; i < n; ++i) idx[i] = (n - 1) - i;
		}

		int rem = n, fail = 0;
		while (rem > 3) {
			bool found = false;
			for (int i = 0; i < rem; ++i) {
				int p = (i + rem - 1) % rem, nx = (i + 1) % rem;
				Vec2 a = poly[idx[p]], b = poly[idx[i]], c = poly[idx[nx]];
				if (CrossProd(a, b, c) <= 0) continue;
				bool ear = true;
				for (int j = 0; j < rem; ++j) {
					if (j == p || j == i || j == nx) continue;
					if (PtInTri(poly[idx[j]], a, b, c)) { ear = false; break; }
				}
				if (ear) {
					tris.push_back({ idx[p], idx[i], idx[nx] });
					idx.erase(idx.begin() + i);
					--rem; found = true; fail = 0; break;
				}
			}
			if (!found && ++fail > rem) break;
		}
		if (rem == 3) tris.push_back({ idx[0], idx[1], idx[2] });
		return tris;
	}
}
#endif

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

			ImGui::Spacing();
			ImGui::TextDisabled("Attack Type");
			const char* attackTypeNames[] = { "Melee", "Ranged" };
			int currentAttackType = (editingEnemy_.attackType == EnemyData::AttackType::Ranged) ? 1 : 0;
			if (ImGui::Combo("Attack Type", &currentAttackType, attackTypeNames, 2)) {
				editingEnemy_.attackType = (currentAttackType == 1)
					? EnemyData::AttackType::Ranged
					: EnemyData::AttackType::Melee;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Melee = rush attack, Ranged = fire projectile");
		}

		if (editingEnemy_.attackType == EnemyData::AttackType::Ranged) {
			if (ImGui::CollapsingHeader("Projectile Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::TextDisabled("Configure the projectile fired during ranged attacks");

				// Actor アセット選択コンボ
				ImGui::TextDisabled("Actor Asset (actor_*.json)");
				// actor_*.json ファイルを検索
				std::vector<std::string> actorNames;
				actorNames.push_back(""); // (none)
				if (fs::exists("./")) {
					for (const auto& entry : fs::directory_iterator("./")) {
						if (entry.is_regular_file() && entry.path().extension() == ".json") {
							std::string fName = entry.path().filename().string();
							if (fName.find("actor_") == 0 && fName.size() > 11) {
								// "actor_XXX.json" → "XXX"
								std::string name = fName.substr(6, fName.size() - 11);
								actorNames.push_back(name);
							}
						}
					}
				}
				int currentActorIdx = 0;
				for (int k = 0; k < static_cast<int>(actorNames.size()); ++k) {
					if (actorNames[k] == editingEnemy_.projectile.actorName) {
						currentActorIdx = k;
						break;
					}
				}
				std::string actorPreview = editingEnemy_.projectile.actorName.empty()
					? "(none)" : editingEnemy_.projectile.actorName;
				if (ImGui::BeginCombo("Actor##proj", actorPreview.c_str())) {
					for (int k = 0; k < static_cast<int>(actorNames.size()); ++k) {
						bool isSelected = (currentActorIdx == k);
						std::string label = actorNames[k].empty() ? "(none)" : actorNames[k];
						if (ImGui::Selectable(label.c_str(), isSelected)) {
							editingEnemy_.projectile.actorName = actorNames[k];
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select an Actor asset for projectile appearance and trajectory");

				ImGui::Spacing();

				// ホーミングオーバーライド
				ImGui::Checkbox("Homing Override##proj", &editingEnemy_.projectile.isHoming);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Override Actor movement with player-tracking homing behavior");

				if (editingEnemy_.projectile.isHoming) {
					ImGui::DragFloat("Homing Strength##proj", &editingEnemy_.projectile.homingStrength, 0.1f, 0.0f, 20.0f, "%.1f");
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("How aggressively the projectile tracks the player");
				}

				ImGui::Spacing();
				ImGui::TextDisabled("Combat Parameters");
				ImGui::TextDisabled("(Damage, Lifetime, and Collision Radius are now\ngoverned by the selected Actor asset.)");
			}
		}

		if (ImGui::CollapsingHeader("Size Tiers (S / M / L)", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextDisabled("Configure stats for each size variant");
			const char* tierNames[] = { "Small", "Medium", "Large" };
			const ImVec4 tierColors[] = {
				ImVec4(0.4f, 0.8f, 1.0f, 1.0f),  // Small: cyan
				ImVec4(1.0f, 0.9f, 0.3f, 1.0f),   // Medium: yellow
				ImVec4(1.0f, 0.4f, 0.3f, 1.0f),    // Large: red
			};

			for (int t = 0; t < 3; ++t) {
				ImGui::PushID(t);
				ImGui::TextColored(tierColors[t], "[%s]", tierNames[t]);
				ImGui::SameLine();
				ImGui::Text("  HP / Power / Scale");

				auto& tier = editingEnemy_.sizeTiers[t];
				ImGui::Indent(10.0f);
				ImGui::DragInt("HP##tier", &tier.hp, 1, 1, 9999);
				ImGui::DragFloat("Power##tier", &tier.power, 0.05f, 0.0f, 50.0f, "%.2f");
				ImGui::DragFloat("Scale##tier", &tier.scale, 0.01f, 0.05f, 5.0f, "%.2f");
				ImGui::Unindent(10.0f);

				if (t < 2) ImGui::Separator();
				ImGui::PopID();
			}
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

		if (ImGui::CollapsingHeader("Collision Shape", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextDisabled("Click canvas to add vertex. Drag to move. Right-click to delete.");
			ImGui::Text("Vertices: %d", static_cast<int>(editingEnemy_.collisionVertices.size()));
			ImGui::SameLine();
			if (ImGui::Button("Clear All##collision")) {
				editingEnemy_.collisionVertices.clear();
			}

			// 凸包性表示
			if (editingEnemy_.collisionVertices.size() >= 3) {
				ImGui::SameLine();
				if (IsConvex(editingEnemy_.collisionVertices)) {
					ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "[Convex]");
				} else {
					ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.1f, 1.0f), "[Concave -> %d tris]",
						static_cast<int>(Triangulate(editingEnemy_.collisionVertices).size()));
				}
			}

			ImGui::SliderFloat("Zoom", &collisionZoom_, 1.0f, 10.0f, "%.1fx");

			// 3Dプレビュー設定
			ImGui::Checkbox("Show Solid Mesh", &showMeshWireframe_);
			if (showMeshWireframe_) {
				ImGui::SameLine();
				const char* viewNames[] = { "Front (XY)", "Side (ZY)", "Top (XZ)" };
				ImGui::SetNextItemWidth(120.0f);
				ImGui::Combo("View", &meshViewMode_, viewNames, 3);
			}
		}

		ImGui::Separator();
		ImGui::Spacing();
		if (ImGui::Button("SAVE ASSET", ImVec2(-1, 40))) {
			SaveEnemy(editingEnemy_);
		}
		ImGui::End();

		// 中央キャンバス: 当たり判定エディタ
		DrawCollisionEditor();
	}

	void EnemyEditor::DrawCollisionEditor() {
		// キャンバスウィンドウ（左パネルと右パネルの間）
		const float canvasX = 255.0f;
		const float canvasW = 1280.0f - 350.0f - canvasX - 5.0f;
		const float canvasY = 18.0f;
		const float canvasH = 700.0f;

		ImGui::SetNextWindowPos(ImVec2(canvasX, canvasY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(canvasW, canvasH), ImGuiCond_Always);
		ImGui::Begin("Collision Editor", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
		ImVec2 canvasSz = ImGui::GetContentRegionAvail();
		if (canvasSz.x < 50.0f) canvasSz.x = 50.0f;
		if (canvasSz.y < 50.0f) canvasSz.y = 50.0f;
		ImVec2 canvasP1 = ImVec2(canvasP0.x + canvasSz.x, canvasP0.y + canvasSz.y);

		// キャンバス背景
		drawList->AddRectFilled(canvasP0, canvasP1, MakeCol32(30, 30, 35, 255));
		drawList->AddRect(canvasP0, canvasP1, MakeCol32(80, 80, 90, 255));

		ImGui::InvisibleButton("collision_canvas", canvasSz,
			ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		bool isHovered = ImGui::IsItemHovered();

		// キャンバス中心 = 敵の原点
		ImVec2 center(canvasP0.x + canvasSz.x * 0.5f + canvasOffsetX_, canvasP0.y + canvasSz.y * 0.5f + canvasOffsetY_);
		float scale = collisionZoom_ * 30.0f; // 1単位 = scale pixels

		// --- グリッド描画 ---
		float gridStep = scale; // 1単位ごとにグリッド
		if (gridStep < 15.0f) gridStep *= 2.0f;
		if (gridStep < 15.0f) gridStep *= 2.0f;

		for (float gx = center.x; gx < canvasP1.x; gx += gridStep) {
			drawList->AddLine(ImVec2(gx, canvasP0.y), ImVec2(gx, canvasP1.y), MakeCol32(50, 50, 55, 255));
		}
		for (float gx = center.x - gridStep; gx > canvasP0.x; gx -= gridStep) {
			drawList->AddLine(ImVec2(gx, canvasP0.y), ImVec2(gx, canvasP1.y), MakeCol32(50, 50, 55, 255));
		}
		for (float gy = center.y; gy < canvasP1.y; gy += gridStep) {
			drawList->AddLine(ImVec2(canvasP0.x, gy), ImVec2(canvasP1.x, gy), MakeCol32(50, 50, 55, 255));
		}
		for (float gy = center.y - gridStep; gy > canvasP0.y; gy -= gridStep) {
			drawList->AddLine(ImVec2(canvasP0.x, gy), ImVec2(canvasP1.x, gy), MakeCol32(50, 50, 55, 255));
		}

		// --- 十字ガイド（原点） ---
		drawList->AddLine(ImVec2(canvasP0.x, center.y), ImVec2(canvasP1.x, center.y), MakeCol32(100, 100, 110, 200), 1.0f);
		drawList->AddLine(ImVec2(center.x, canvasP0.y), ImVec2(center.x, canvasP1.y), MakeCol32(100, 100, 110, 200), 1.0f);

		// 原点マーカー
		drawList->AddCircleFilled(center, 4.0f, MakeCol32(255, 200, 50, 200));
		drawList->AddText(ImVec2(center.x + 6, center.y - 14), MakeCol32(200, 200, 200, 200), "Origin");

		// --- メッシュワイヤーフレームキャッシュ更新 ---
		if (cachedMeshGltfPath_ != editingEnemy_.gltfPath) {
			ExtractMeshWireframe(editingEnemy_.gltfPath);
		}

		// --- メッシュ描画（ソリッドポリゴン） ---
		if (showMeshWireframe_ && !cachedMeshFaces_.empty()) {
			// 3D→ 2D投影 (ビューモードに応じた座標選択)
			auto project3D = [&](const std::array<float, 3>& pos) -> ImVec2 {
				float px, py;
				switch (meshViewMode_) {
				case 0: // Front (XY)
					px = pos[0]; py = pos[1]; break;
				case 1: // Side (ZY)
					px = pos[2]; py = pos[1]; break;
				case 2: // Top (XZ)
					px = pos[0]; py = pos[2]; break;
				default:
					px = pos[0]; py = pos[1]; break;
				}
				return ImVec2(center.x + px * scale, center.y - py * scale);
			};

			// 深度取得（奥にあるものから手前に描画するためのZソート用）
			auto getDepth = [&](const std::array<float, 3>& pos) -> float {
				switch (meshViewMode_) {
				case 0: return -pos[2]; // Front: 奥方向は -Z
				case 1: return -pos[0]; // Side: 奥方向は -X
				case 2: return -pos[1]; // Top: 奥方向は -Y
				default: return -pos[2];
				}
			};

			struct SolidFace {
				ImVec2 p0, p1, p2;
				float depth;
				ImU32 col;
			};
			std::vector<SolidFace> renderFaces;

			for (const auto& face : cachedMeshFaces_) {
				if (face[0] < 0 || face[0] >= cachedMeshPositions_.size()) continue;
				if (face[1] < 0 || face[1] >= cachedMeshPositions_.size()) continue;
				if (face[2] < 0 || face[2] >= cachedMeshPositions_.size()) continue;

				const auto& v0 = cachedMeshPositions_[face[0]];
				const auto& v1 = cachedMeshPositions_[face[1]];
				const auto& v2 = cachedMeshPositions_[face[2]];

				float d = (getDepth(v0) + getDepth(v1) + getDepth(v2)) / 3.0f;

				ImVec2 p0 = project3D(v0);
				ImVec2 p1 = project3D(v1);
				ImVec2 p2 = project3D(v2);

				// キャンバス内か簡易チェック（カリング）
				if (p0.x < canvasP0.x - 50 && p1.x < canvasP0.x - 50 && p2.x < canvasP0.x - 50) continue;
				if (p0.x > canvasP1.x + 50 && p1.x > canvasP1.x + 50 && p2.x > canvasP1.x + 50) continue;
				if (p0.y < canvasP0.y - 50 && p1.y < canvasP0.y - 50 && p2.y < canvasP0.y - 50) continue;
				if (p0.y > canvasP1.y + 50 && p1.y > canvasP1.y + 50 && p2.y > canvasP1.y + 50) continue;

				// 法線計算と簡易Lighting（フラットシェーディング）
				float dx1 = v1[0] - v0[0]; float dy1 = v1[1] - v0[1]; float dz1 = v1[2] - v0[2];
				float dx2 = v2[0] - v0[0]; float dy2 = v2[1] - v0[1]; float dz2 = v2[2] - v0[2];
				float nx = dy1*dz2 - dz1*dy2;
				float ny = dz1*dx2 - dx1*dz2;
				float nz = dx1*dy2 - dy1*dx2;
				float len = std::sqrt(nx*nx + ny*ny + nz*nz);
				if (len > 0.0001f) { nx /= len; ny /= len; nz /= len; }

				float dot = nx * 0.4f + ny * 0.8f + nz * 0.4f;
				float intensity = 0.35f + 0.65f * std::max(0.0f, dot);
				
				ImU32 faceColor = MakeCol32(
					static_cast<int>(120 * intensity),
					static_cast<int>(150 * intensity),
					static_cast<int>(200 * intensity),
					255
				);

				renderFaces.push_back({ p0, p1, p2, d, faceColor });
			}

			// Zソート（奥から手前へPainter's Algorithm）
			std::sort(renderFaces.begin(), renderFaces.end(), [](const SolidFace& a, const SolidFace& b) {
				return a.depth > b.depth;
			});

			for (const auto& f : renderFaces) {
				drawList->AddTriangleFilled(f.p0, f.p1, f.p2, f.col);
				drawList->AddTriangle(f.p0, f.p1, f.p2, MakeCol32(50, 70, 90, 80), 1.0f); // 輪郭を薄く表示して立体感を強調
			}

			// メッシュ情報表示
			char meshInfo[128];
			snprintf(meshInfo, sizeof(meshInfo), "Solid Mesh: %d verts, %d faces",
				static_cast<int>(cachedMeshPositions_.size()),
				static_cast<int>(cachedMeshFaces_.size()));
			drawList->AddText(ImVec2(canvasP0.x + 5, canvasP0.y + 5),
				MakeCol32(120, 200, 255, 200), meshInfo);
		}

		// --- ローカル→スクリーン変換 ---
		auto localToScreen = [&](float lx, float ly) -> ImVec2 {
			return ImVec2(center.x + lx * scale, center.y - ly * scale); // Y反転
		};
		auto screenToLocal = [&](ImVec2 screen) -> std::pair<float, float> {
			return { (screen.x - center.x) / scale, -(screen.y - center.y) / scale };
		};

		auto& verts = editingEnemy_.collisionVertices;
		const float VERTEX_RADIUS = 7.0f;
		const float VERTEX_HIT_RADIUS = 12.0f;

		// --- ポリゴン塗りつぶし描画（三角形分割ベース） ---
		if (verts.size() >= 3) {
			bool convex = IsConvex(verts);

			if (convex) {
				// 凸ならそのまま塗りつぶし
				std::vector<ImVec2> polyPoints;
				for (const auto& v : verts) {
					polyPoints.push_back(localToScreen(v.x, v.y));
				}
				drawList->AddConvexPolyFilled(polyPoints.data(), static_cast<int>(polyPoints.size()),
					MakeCol32(0, 180, 255, 40));
			} else {
				// 凹なら三角形に分割して塗りつぶし + 分割線表示
				auto tris = Triangulate(verts);

				// 各三角形を交互色で塗りつぶし
				constexpr ImU32 triColors[] = {
					MakeCol32(0, 180, 255, 35),
					MakeCol32(255, 140, 0, 35),
					MakeCol32(0, 255, 140, 35),
					MakeCol32(200, 80, 255, 35),
				};

				for (size_t ti = 0; ti < tris.size(); ++ti) {
					ImVec2 triPts[3] = {
						localToScreen(verts[tris[ti][0]].x, verts[tris[ti][0]].y),
						localToScreen(verts[tris[ti][1]].x, verts[tris[ti][1]].y),
						localToScreen(verts[tris[ti][2]].x, verts[tris[ti][2]].y),
					};
					drawList->AddConvexPolyFilled(triPts, 3, triColors[ti % 4]);
				}

				// 分割線（元の辺ではない内部辺）を点線で描画
				for (const auto& tri : tris) {
					for (int e = 0; e < 3; ++e) {
						int a = tri[e], b = tri[(e + 1) % 3];
						// 元のポリゴンの辺であるかチェック
						bool isOrigEdge = false;
						int vn = static_cast<int>(verts.size());
						for (int i = 0; i < vn; ++i) {
							int ni = (i + 1) % vn;
							if ((i == a && ni == b) || (i == b && ni == a)) {
								isOrigEdge = true;
								break;
							}
						}
						if (!isOrigEdge) {
							ImVec2 pa = localToScreen(verts[a].x, verts[a].y);
							ImVec2 pb = localToScreen(verts[b].x, verts[b].y);
							drawList->AddLine(pa, pb, MakeCol32(255, 200, 50, 160), 1.0f);
						}
					}
				}
			}
		}

		// --- 辺描画 ---
		if (verts.size() >= 2) {
			for (size_t i = 0; i < verts.size(); ++i) {
				size_t next = (i + 1) % verts.size();
				ImVec2 p0 = localToScreen(verts[i].x, verts[i].y);
				ImVec2 p1 = localToScreen(verts[next].x, verts[next].y);
				drawList->AddLine(p0, p1, MakeCol32(0, 200, 255, 220), 2.0f);
			}
		}

		// --- 頂点描画 ---
		for (size_t i = 0; i < verts.size(); ++i) {
			ImVec2 sp = localToScreen(verts[i].x, verts[i].y);
			bool isDragged = (draggedVertexIndex_ == static_cast<int>(i));
			ImU32 col = isDragged ? MakeCol32(255, 100, 50, 255) : MakeCol32(0, 220, 255, 255);
			drawList->AddCircleFilled(sp, VERTEX_RADIUS, col);
			drawList->AddCircle(sp, VERTEX_RADIUS, MakeCol32(255, 255, 255, 180), 0, 1.5f);

			// 頂点番号
			char idxBuf[8];
			snprintf(idxBuf, sizeof(idxBuf), "%d", static_cast<int>(i));
			drawList->AddText(ImVec2(sp.x + 9, sp.y - 12), MakeCol32(255, 255, 255, 200), idxBuf);
		}

		// --- マウス操作 ---
		ImVec2 mousePos = ImGui::GetIO().MousePos;

		// 左クリック: 頂点追加 or ドラッグ開始
		if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			// 既存頂点のクリック判定
			int hitIdx = -1;
			for (size_t i = 0; i < verts.size(); ++i) {
				ImVec2 sp = localToScreen(verts[i].x, verts[i].y);
				float dx = mousePos.x - sp.x;
				float dy = mousePos.y - sp.y;
				if (dx * dx + dy * dy < VERTEX_HIT_RADIUS * VERTEX_HIT_RADIUS) {
					hitIdx = static_cast<int>(i);
					break;
				}
			}

			if (hitIdx >= 0) {
				// 既存頂点をドラッグ開始
				draggedVertexIndex_ = hitIdx;
			} else {
				// 新規頂点追加
				auto [lx, ly] = screenToLocal(mousePos);
				CollisionVertex newVert;
				newVert.x = lx;
				newVert.y = ly;
				verts.push_back(newVert);
			}
		}

		// ドラッグ中: 頂点移動
		if (draggedVertexIndex_ >= 0 && draggedVertexIndex_ < static_cast<int>(verts.size())) {
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				auto [lx, ly] = screenToLocal(mousePos);
				verts[draggedVertexIndex_].x = lx;
				verts[draggedVertexIndex_].y = ly;
			}
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				draggedVertexIndex_ = -1;
			}
		}

		// 右クリック: 頂点削除
		if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			for (size_t i = 0; i < verts.size(); ++i) {
				ImVec2 sp = localToScreen(verts[i].x, verts[i].y);
				float dx = mousePos.x - sp.x;
				float dy = mousePos.y - sp.y;
				if (dx * dx + dy * dy < VERTEX_HIT_RADIUS * VERTEX_HIT_RADIUS) {
					verts.erase(verts.begin() + i);
					if (draggedVertexIndex_ == static_cast<int>(i)) draggedVertexIndex_ = -1;
					break;
				}
			}
		}

		// 右クリックドラッグ: キャンバスの移動
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			canvasOffsetX_ += delta.x;
			canvasOffsetY_ += delta.y;
		}

		// --- 座標表示 ---
		if (isHovered) {
			auto [lx, ly] = screenToLocal(mousePos);
			char coordBuf[64];
			snprintf(coordBuf, sizeof(coordBuf), "(%.2f, %.2f)", lx, ly);
			drawList->AddText(ImVec2(mousePos.x + 15, mousePos.y - 5), MakeCol32(200, 200, 200, 220), coordBuf);
		}

		ImGui::End();
	}

	void EnemyActionEditor::Update() {
#if defined(_DEBUG)
		DrawEditorUI();
#endif
	}

	void EnemyActionEditor::DrawEditorUI() {
		ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300.0f, 400.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Enemy Action Editor", nullptr, ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Save")) {
					SaveEnemy(editingEnemy_);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Text("Editing JSON:");
		static char filenameBuf[64] = "enemy_data";
		ImGui::InputText(".json##action", filenameBuf, sizeof(filenameBuf));

		if (ImGui::Button("Load Enemy##action")) {
			std::string fname = std::string(filenameBuf) + ".json";
			LoadEnemy(editingEnemy_, fname);
			if (!editingEnemy_.gltfPath.empty()) {
				cachedAnimationNames_ = ExtractAnimationNames(editingEnemy_.gltfPath);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Save Enemy##action")) {
			editingEnemy_.name = filenameBuf;
			SaveEnemy(editingEnemy_);
		}

		ImGui::End();

		DrawNodeEditor();
	}

	void EnemyActionEditor::DrawNodeEditor() {
		ImGui::SetNextWindowPos(ImVec2(255.0f, 720.0f), ImGuiCond_FirstUseEver); // Positioning below the existing canvas
		ImGui::SetNextWindowSize(ImVec2(1280.0f - 350.0f - 255.0f, 250.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("State Machine Node Editor", nullptr, ImGuiWindowFlags_NoCollapse);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 mousePos = ImGui::GetIO().MousePos;

		if (ImGui::Button("Add Node")) {
			int newId = 1;
			for (const auto& n : editingEnemy_.nodes) {
				if (n.id >= newId) newId = n.id + 1;
			}
			Node node;
			node.id = newId;
			node.name = "State" + std::to_string(newId);
			node.state = "Idle";
			node.x = 40.0f + static_cast<float>((editingEnemy_.nodes.size() % 6) * 190);
			node.y = 40.0f + static_cast<float>((editingEnemy_.nodes.size() / 6) * 110);
			editingEnemy_.nodes.push_back(node);
		}

		ImGui::SameLine();
		ImGui::TextDisabled("Connect: drag from Out port to a node. Right click node/link to delete.");

		ImGui::Separator();

		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();
		if (canvasSize.x < 100) canvasSize.x = 100;
		if (canvasSize.y < 100) canvasSize.y = 100;
		nodeCanvasWidth_ = canvasSize.x;
		nodeCanvasHeight_ = canvasSize.y;

		ImGui::InvisibleButton("node_canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		ImVec2 origin = canvasPos;

		drawList->AddRectFilled(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), MakeCol32(40, 40, 45, 255));
		drawList->AddRect(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), MakeCol32(80, 80, 90, 255));

		if (nodeDragActive_) {
			auto selectedIt = std::find_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& node) {
				return node.id == nodeEditor_selectedNodeId_;
			});
			if (selectedIt != editingEnemy_.nodes.end() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !nodeLinkDragActive_) {
				selectedIt->x = mousePos.x - origin.x - nodeDragOffsetX_;
				selectedIt->y = mousePos.y - origin.y - nodeDragOffsetY_;
				if (selectedIt->x < 0.0f) selectedIt->x = 0.0f;
				if (selectedIt->y < 0.0f) selectedIt->y = 0.0f;
			} else {
				nodeDragActive_ = false;
			}
		}

		// Draw Nodes
		for (auto& n : editingEnemy_.nodes) {
			ImVec2 a = ImVec2(origin.x + n.x, origin.y + n.y);
			ImVec2 b = ImVec2(a.x + 180.0f, a.y + 110.0f);

			ImU32 col = MakeCol32(60, 60, 70, 220);
			if (nodeEditor_selectedNodeId_ == n.id) {
				col = MakeCol32(100, 80, 80, 255);
			}

			drawList->AddRectFilled(a, b, col, 6.0f);
			drawList->AddRect(a, b, MakeCol32(200, 200, 200, 220), 6.0f, 0, 2.0f);

			// inline UI
			ImVec2 prevScreenPos = ImGui::GetCursorScreenPos();
			ImGui::SetCursorScreenPos(ImVec2(a.x + 6.0f, a.y + 6.0f));
			ImGui::PushID(n.id);
			
			char nameBufFB[128]; strncpy_s(nameBufFB, n.name.c_str(), sizeof(nameBufFB));
			ImGui::SetNextItemWidth(150.0f);
			if (ImGui::InputText("##node_name_fb", nameBufFB, sizeof(nameBufFB))) n.name = nameBufFB;
			if (ImGui::IsItemActive()) nodeEditor_selectedNodeId_ = n.id;

			ImGui::SetCursorScreenPos(ImVec2(a.x + 6.0f, a.y + 36.0f));
			char stateBufFB[128]; strncpy_s(stateBufFB, n.state.c_str(), sizeof(stateBufFB));
			ImGui::SetNextItemWidth(150.0f);
			if (ImGui::InputText("##node_state_fb", stateBufFB, sizeof(stateBufFB))) n.state = stateBufFB;

			ImGui::SetCursorScreenPos(ImVec2(a.x + 6.0f, a.y + 66.0f));
			ImGui::SetNextItemWidth(150.0f);
			if (ImGui::BeginCombo("##node_anim_inline", n.animationName.empty() ? "(None)" : n.animationName.c_str())) {
				if (ImGui::Selectable("(None)", n.animationName.empty())) n.animationName.clear();
				for (const auto& avail : cachedAnimationNames_) {
					bool isSel = (n.animationName == avail);
					if (ImGui::Selectable(avail.c_str(), isSel)) n.animationName = avail;
					if (isSel) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::PopID();
			ImGui::SetCursorScreenPos(prevScreenPos);

			bool hovered = (mousePos.x >= a.x && mousePos.x <= b.x && mousePos.y >= a.y && mousePos.y <= b.y);
			bool overInlineControls = (mousePos.x >= a.x + 6.0f && mousePos.x <= a.x + 166.0f && mousePos.y >= a.y + 6.0f && mousePos.y <= a.y + 90.0f);
			
			if (!nodeDragActive_ && hovered && !overInlineControls && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				nodeDragActive_ = true;
				nodeEditor_selectedNodeId_ = n.id;
				nodeDragOffsetX_ = mousePos.x - a.x;
				nodeDragOffsetY_ = mousePos.y - a.y;
			}

			if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				nodeEditor_contextNodeId_ = n.id;
				ImGui::OpenPopup("NodeContextMenu");
			}

			// Ports
			ImVec2 inputPortPos = ImVec2(a.x + 8.0f, a.y + 96.0f);
			ImVec2 portPos = ImVec2(b.x - 8.0f, a.y + 96.0f);
			
			drawList->AddCircleFilled(inputPortPos, 8.0f, MakeCol32(120, 220, 140, 220));
			drawList->AddCircleFilled(portPos, 8.0f, MakeCol32(120, 160, 255, 220));
			drawList->AddText(ImVec2(inputPortPos.x - 5.0f, inputPortPos.y - 22.0f), MakeCol32(180, 220, 180, 255), "In");
			drawList->AddText(ImVec2(portPos.x - 8.0f, portPos.y - 22.0f), MakeCol32(180, 200, 255, 255), "Out");

			bool mouseOverPort = (mousePos.x >= portPos.x - 10.0f && mousePos.x <= portPos.x + 10.0f && mousePos.y >= portPos.y - 10.0f && mousePos.y <= portPos.y + 10.0f);
			if (!nodeLinkDragActive_ && mouseOverPort && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				nodeLinkDragActive_ = true;
				nodeEditor_linkStartId_ = n.id;
				pendingNewNodeScreenX_ = portPos.x;
				pendingNewNodeScreenY_ = portPos.y;
			}
		}

		if (nodeDragActive_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			nodeDragActive_ = false;
		}

		// Draw Links
		for (size_t i = 0; i < editingEnemy_.links.size(); ++i) {
			const auto& l = editingEnemy_.links[i];
			const Node* from = nullptr;
			const Node* to = nullptr;
			for (const auto& n : editingEnemy_.nodes) {
				if (n.id == l.from) from = &n;
				if (n.id == l.to) to = &n;
			}
			if (from && to) {
				ImVec2 pa = ImVec2(origin.x + from->x + 180.0f - 8.0f, origin.y + from->y + 96.0f);
				ImVec2 pb = ImVec2(origin.x + to->x + 8.0f, origin.y + to->y + 96.0f);
				drawList->AddBezierCubic(pa, ImVec2(pa.x + 40, pa.y), ImVec2(pb.x - 40, pb.y), pb, MakeCol32(200, 200, 100, 220), 3.0f);
				
				// Click to context-menu / link logic
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					float d1 = (mousePos.x - pa.x) * (mousePos.x - pa.x) + (mousePos.y - pa.y) * (mousePos.y - pa.y);
					float d2 = (mousePos.x - pb.x) * (mousePos.x - pb.x) + (mousePos.y - pb.y) * (mousePos.y - pb.y);
					if (d1 < 100.0f || d2 < 100.0f) {
						linkEditor_contextLinkIndex_ = static_cast<int>(i);
						ImGui::OpenPopup("LinkContextMenu");
					}
				}
			}
		}

		if (nodeLinkDragActive_) {
			ImVec2 start = ImVec2(pendingNewNodeScreenX_, pendingNewNodeScreenY_);
			drawList->AddLine(start, mousePos, MakeCol32(255, 255, 150, 220), 3.0f);
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				int targetId = -1;
				for (const auto& n : editingEnemy_.nodes) {
					ImVec2 na = ImVec2(origin.x + n.x, origin.y + n.y);
					ImVec2 nb = ImVec2(na.x + 180.0f, na.y + 110.0f);
					if (mousePos.x >= na.x && mousePos.x <= nb.x && mousePos.y >= na.y && mousePos.y <= nb.y) {
						targetId = n.id; break;
					}
				}
				if (targetId != -1 && targetId != nodeEditor_linkStartId_) {
					bool exists = false;
					for (const auto& l : editingEnemy_.links) {
						if (l.from == nodeEditor_linkStartId_ && l.to == targetId) exists = true;
					}
					if (!exists) {
						Link link; link.from = nodeEditor_linkStartId_; link.to = targetId; link.condition = "Always";
						editingEnemy_.links.push_back(link);
					}
				}
				nodeLinkDragActive_ = false;
				nodeEditor_linkStartId_ = -1;
			}
		}

		if (ImGui::BeginPopup("NodeContextMenu")) {
			if (nodeEditor_contextNodeId_ != -1) {
				if (ImGui::MenuItem("Delete Node")) {
					int id = nodeEditor_contextNodeId_;
					editingEnemy_.links.erase(std::remove_if(editingEnemy_.links.begin(), editingEnemy_.links.end(), [&](const Link& lk) {
						return lk.from == id || lk.to == id;
					}), editingEnemy_.links.end());
					editingEnemy_.nodes.erase(std::remove_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& nd) {
						return nd.id == id;
					}), editingEnemy_.nodes.end());
					if (nodeEditor_selectedNodeId_ == id) nodeEditor_selectedNodeId_ = -1;
				}
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("LinkContextMenu")) {
			if (linkEditor_contextLinkIndex_ != -1) {
			    auto& linkRef = editingEnemy_.links[linkEditor_contextLinkIndex_];
			    ImGui::Text("Link %d -> %d", linkRef.from, linkRef.to);
			    ImGui::Separator();
			    
			    char condBuf[256];
			    strncpy_s(condBuf, linkRef.condition.c_str(), sizeof(condBuf));
			    if (ImGui::InputText("Condition", condBuf, sizeof(condBuf))) {
			        linkRef.condition = condBuf;
			    }

				if (ImGui::MenuItem("Delete Link")) {
					editingEnemy_.links.erase(editingEnemy_.links.begin() + linkEditor_contextLinkIndex_);
					linkEditor_contextLinkIndex_ = -1;
				}
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}
#endif
}
