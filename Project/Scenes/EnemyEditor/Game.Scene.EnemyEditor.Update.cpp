module Game.Editor.EnemyEditor;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import <string>;
import <filesystem>;
import Game.MotionManager;
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
#if defined(_DEBUG)
	static float s_actionSaveNotificationTimer = 0.0f;
#endif

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
		if (fs::exists("Assets/Data/Enemy/")) {
			for (const auto& entry : fs::directory_iterator("Assets/Data/Enemy/")) {
				if (entry.path().extension() == ".json") {
					std::string fName = entry.path().filename().string();
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
				if (fs::exists("Assets/Data/Actor/")) {
					for (const auto& entry : fs::directory_iterator("Assets/Data/Actor/")) {
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

				std::string ext = "";
				if (!editingEnemy_.gltfPath.empty()) {
					ext = fs::path(editingEnemy_.gltfPath).extension().string();
					for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				}

				if (ext == ".obj") {
					// OBJの場合はアニメーション名自体が存在しないので表示しない
				} else {
					// GLTF / GLB の場合は必ずコンボボックス（選択式）にする
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

	float DistPtSegSq(float px, float py, float ax, float ay, float bx, float by) {
		float abx = bx - ax, aby = by - ay;
		float apx = px - ax, apy = py - ay;
		float len2 = abx * abx + aby * aby;
		if (len2 <= 0.0001f) return apx * apx + apy * apy;
		float t = (apx * abx + apy * aby) / len2;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;
		float cx = ax + abx * t, cy = ay + aby * t;
		float dx = px - cx, dy = py - cy;
		return dx * dx + dy * dy;
	}

	void EnemyActionEditor::Update() {
#if defined(_DEBUG)
		float dt = ImGui::GetIO().DeltaTime;
		currentStateElapsedTime_ += dt;
		if (transitionFlashTimer_ > 0.0f) transitionFlashTimer_ -= dt;
		EvaluateStateMachine();
		DrawEditorUI();
		DrawEditorLogUI();
#endif
	}

	void EnemyActionEditor::DrawEditorUI() {
		ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300.0f, 400.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Enemy Action Editor", nullptr, ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Save")) { SaveEnemy(editingEnemy_); }
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

	ImGui::SetNextWindowPos(ImVec2(320.0f, 10.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(340.0f, 300.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Action File Browser", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
	ImGui::TextDisabled("Click a JSON to load into the Action Editor");
	ImGui::Separator();
	static char fileFilter[128] = "";
	ImGui::InputText("Filter", fileFilter, sizeof(fileFilter));
	ImGui::Separator();
	ImGui::BeginChild("ActionFileList", ImVec2(0, 0), false);
	if (fs::exists("Assets/Data/Enemy/")) {
		for (const auto& entry : fs::directory_iterator("Assets/Data/Enemy/")) {
			if (entry.path().extension() != ".json") continue;
			std::string fName = entry.path().filename().string();
			if (fileFilter[0] != '\0') {
				if (fName.find(fileFilter) == std::string::npos) continue;
			}
			bool isSel = (!activeFileName_.empty() && activeFileName_ == fName);
			if (ImGui::Selectable(fName.c_str(), isSel)) {
				LoadEnemy(editingEnemy_, fName);
				if (!editingEnemy_.gltfPath.empty()) cachedAnimationNames_ = ExtractAnimationNames(editingEnemy_.gltfPath);
				if (!editingEnemy_.nodes.empty()) {
					if (requireManualStart_) { currentStateId_ = -1; firstNodeStarted_ = false; }
					else { currentStateId_ = editingEnemy_.nodes.front().id; }
				}
				currentStateElapsedTime_ = 0.0f;
				previousStateId_ = -1;
				AddLog(std::string("[EnemyEditor] Loaded: ") + fName);
			}
		}
	}
	ImGui::EndChild();
	ImGui::End();

	ImGui::Text("Editing JSON:");
	static char filenameBuf[64] = "enemy_data";
	static std::string lastActiveFile;
	if (!activeFileName_.empty()) {
		ImGui::TextDisabled("Active: %s", activeFileName_.c_str());
		if (activeFileName_ != lastActiveFile) {
			std::string base = activeFileName_;
			if (base.size() > 5 && base.substr(base.size() - 5) == ".json") base = base.substr(0, base.size() - 5);
			strncpy_s(filenameBuf, base.c_str(), sizeof(filenameBuf));
			lastActiveFile = activeFileName_;
		}
	} else {
		lastActiveFile.clear();
	}
	ImGui::InputText(".json##action", filenameBuf, sizeof(filenameBuf));

	ImGui::Spacing();
	ImGui::TextDisabled("JSON Files");
	ImGui::BeginChild("FileListAction", ImVec2(0, 150), true);
	if (fs::exists("Assets/Data/Enemy/")) {
		for (const auto& entry : fs::directory_iterator("Assets/Data/Enemy/")) {
			if (entry.path().extension() == ".json") {
				std::string fName = entry.path().filename().string();
				bool isSelected = (!activeFileName_.empty() && activeFileName_ == fName);
				if (ImGui::Selectable(fName.c_str(), isSelected)) {
					LoadEnemy(editingEnemy_, fName);
					if (!editingEnemy_.gltfPath.empty()) cachedAnimationNames_ = ExtractAnimationNames(editingEnemy_.gltfPath);
					if (!editingEnemy_.nodes.empty()) {
						if (requireManualStart_) { currentStateId_ = -1; firstNodeStarted_ = false; }
						else { currentStateId_ = editingEnemy_.nodes.front().id; }
					}
					currentStateElapsedTime_ = 0.0f;
					previousStateId_ = -1;
					AddLog(std::string("[EnemyEditor] Loaded: ") + fName);
				}
			}
		}
	}
	ImGui::EndChild();

		if (ImGui::Button("Load Enemy##action")) {
			std::string fname = std::string(filenameBuf) + ".json";
			LoadEnemy(editingEnemy_, fname);
			if (!editingEnemy_.gltfPath.empty()) {
				cachedAnimationNames_ = ExtractAnimationNames(editingEnemy_.gltfPath);
			}
			if (!editingEnemy_.nodes.empty()) {
				if (requireManualStart_) { currentStateId_ = -1; firstNodeStarted_ = false; }
				else { currentStateId_ = editingEnemy_.nodes.front().id; }
			}
			currentStateElapsedTime_ = 0.0f;
			previousStateId_ = -1;
			AddLog("[EnemyEditor] Loaded: " + fname);
		}
		ImGui::SameLine();
		if (ImGui::Button("Save Enemy##action")) {
			editingEnemy_.name = filenameBuf;
			SaveEnemy(editingEnemy_);
			AddLog("[EnemyEditor] Saved: " + editingEnemy_.name);
			s_actionSaveNotificationTimer = 2.0f;
		}

		if (s_actionSaveNotificationTimer > 0.0f) {
			s_actionSaveNotificationTimer -= ImGui::GetIO().DeltaTime;
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "  Saved Successfully!");
		}

		ImGui::End();
		DrawNodeEditor();
	}

	void EnemyActionEditor::DrawNodeEditor() {
		ImGui::SetNextWindowPos(ImVec2(10.0f, 420.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(1260.0f, 300.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("State Machine", nullptr, ImGuiWindowFlags_NoCollapse);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 mousePos = ImGui::GetIO().MousePos;

		if (ImGui::Button("Add Node")) {
			PushUndoState();
			int newId = 1;
			for (const auto& n : editingEnemy_.nodes) { if (n.id >= newId) newId = n.id + 1; }
			Node node;
			node.id = newId;
			node.name = "State" + std::to_string(newId);
			node.state = "Idle";
			node.x = 40.0f + static_cast<float>((editingEnemy_.nodes.size() % 6) * 190);
			node.y = 40.0f + static_cast<float>((editingEnemy_.nodes.size() / 6) * 120);
			editingEnemy_.nodes.push_back(node);
			char dbg[256]; snprintf(dbg, sizeof(dbg), "[EnemyEditor] Added node %d", newId); AddLog(dbg);
		}

		ImGui::SameLine();
        if (ImGui::Button("Save Nodes##toolbar")) {
			SaveEnemy(editingEnemy_);
			AddLog("[EnemyEditor] Saved nodes");
			s_actionSaveNotificationTimer = 2.0f;
		}

		ImGui::SameLine();
		if (ImGui::Button("Start First Node")) {
			if (!editingEnemy_.nodes.empty()) {
				previousStateId_ = currentStateId_;
				currentStateId_ = editingEnemy_.nodes.front().id;
				currentStateElapsedTime_ = 0.0f;
				transitionFlashTimer_ = 1.0f;
				userRequestedStart_ = true;
				firstNodeStarted_ = true;
				lockStateMachineAfterStartFirstNode_ = false;
				if (!editingEnemy_.nodes.front().boundBool.empty()) {
					runtimeBoolFlags_[editingEnemy_.nodes.front().boundBool] = true;
				}
				AddLog("[EnemyEditor] Started first node");
			}
		}

		if (s_actionSaveNotificationTimer > 0.0f) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Saved Successfully!");
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset State Timer")) {
			currentStateElapsedTime_ = 0.0f;
			AddLog("[EnemyEditor] Timer reset");
		}

		ImGui::SameLine();
		if (ImGui::Button("Restart State")) {
			if (currentStateId_ != -1) {
				currentStateElapsedTime_ = 0.0f;
				transitionFlashTimer_ = 0.5f;
				AddLog("[EnemyEditor] State restarted");
			}
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(80.0f);
		static int jumpNodeIdx = 0;
		if (!editingEnemy_.nodes.empty()) {
			if (jumpNodeIdx >= static_cast<int>(editingEnemy_.nodes.size())) jumpNodeIdx = 0;
			std::string jumpPreview = editingEnemy_.nodes[jumpNodeIdx].name;
            if (ImGui::BeginCombo("##jump_state", jumpPreview.c_str())) {
                for (int i = 0; i < static_cast<int>(editingEnemy_.nodes.size()); ++i) {
                    bool isSel = (i == jumpNodeIdx);
                    if (ImGui::Selectable(editingEnemy_.nodes[i].name.c_str(), isSel)) {
                        jumpNodeIdx = i;
                        previousStateId_ = currentStateId_;
                        currentStateId_ = editingEnemy_.nodes[i].id;
                        currentStateElapsedTime_ = 0.0f;
                        transitionFlashTimer_ = 1.0f;
                        firstNodeStarted_ = true;
                        if (!editingEnemy_.nodes[i].boundBool.empty()) {
                            runtimeBoolFlags_[editingEnemy_.nodes[i].boundBool] = true;
                        }
                        char dbg[256]; snprintf(dbg, sizeof(dbg), "[EnemyEditor] Jumped to node %d", currentStateId_); AddLog(dbg);
                    }
                    if (isSel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
		}
		ImGui::SameLine();
		if (ImGui::Button("Jump To Node")) {
			if (!editingEnemy_.nodes.empty() && jumpNodeIdx < static_cast<int>(editingEnemy_.nodes.size())) {
				previousStateId_ = currentStateId_;
				currentStateId_ = editingEnemy_.nodes[jumpNodeIdx].id;
				currentStateElapsedTime_ = 0.0f;
				transitionFlashTimer_ = 1.0f;
				firstNodeStarted_ = true;
				if (!editingEnemy_.nodes[jumpNodeIdx].boundBool.empty()) {
					runtimeBoolFlags_[editingEnemy_.nodes[jumpNodeIdx].boundBool] = true;
				}
				char dbg[256]; snprintf(dbg, sizeof(dbg), "[EnemyEditor] Jumped to node %d", currentStateId_); AddLog(dbg);
			}
		}

		ImGui::SameLine();
		ImGui::Checkbox("Require Manual Start (first node)", &requireManualStart_);
		if (requireManualStart_ != requireManualStart_) {
			if (!requireManualStart_) {
				firstNodeStarted_ = true;
				lockStateMachineAfterStartFirstNode_ = false;
			} else {
				firstNodeStarted_ = false;
			}
		}

		if (currentStateId_ != -1) {
			auto cit = std::find_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& n) { return n.id == currentStateId_; });
			if (cit != editingEnemy_.nodes.end()) {
				ImGui::Text("Active: %s (id=%d)  Timer: %.2f s", cit->name.c_str(), cit->id, currentStateElapsedTime_);
			}
		} else {
			ImGui::TextDisabled("No active state");
		}

		if (transitionFlashTimer_ > 0.0f && previousStateId_ != -1) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, transitionFlashTimer_), "TRANSITION: %d -> %d", previousStateId_, currentStateId_);
		}

		if (currentStateId_ != -1) {
			bool foundOut = false, anyReady = false;
			float soonestTime = 1e9f; int soonestTo = -1; std::string soonestCond;
			for (const auto& link : editingEnemy_.links) {
				if (link.from != currentStateId_ || link.to == currentStateId_) continue;
				foundOut = true;
				std::string cond = link.condition;
				while (!cond.empty() && cond.front() == ' ') cond.erase(cond.begin());
				while (!cond.empty() && cond.back() == ' ') cond.pop_back();
				bool isTimeD = (cond.rfind("Time>=", 0) == 0) || (cond.rfind("Time>", 0) == 0);
				bool condMet = CheckLinkCondition(link);
				if (condMet) { anyReady = true; soonestTo = link.to; soonestCond = cond.empty() ? "Always" : cond; break; }
				if (isTimeD) {
					float target = 0.0f;
					try { if (cond.rfind("Time>=", 0) == 0) target = std::stof(cond.substr(6)); else if (cond.rfind("Time>", 0) == 0) target = std::stof(cond.substr(5)); } catch (...) {}
					float remain = target - currentStateElapsedTime_;
					if (remain < 0.0f) remain = 0.0f;
					if (remain < soonestTime) { soonestTime = remain; soonestTo = link.to; soonestCond = cond; }
				}
			}
			if (!foundOut) { ImGui::TextDisabled("No outgoing transitions."); }
			else if (anyReady) { ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Transition ready -> id=%d (%s)", soonestTo, soonestCond.c_str()); }
			else if (soonestTime < 1e8f) { ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Next in %.2fs -> id=%d (%s)", soonestTime, soonestTo, soonestCond.c_str()); }
			else { ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "No transitions satisfied."); }
		}

		{
			{
				std::vector<std::string> allFlags;
				for (const auto& nd : editingEnemy_.nodes) if (!nd.boundBool.empty()) allFlags.push_back(nd.boundBool);
				for (const auto& lk : editingEnemy_.links) {
					if (lk.condition.rfind("BOOL:", 0) == 0 && lk.condition.size() > 5) {
						std::string f = lk.condition.substr(5);
						allFlags.push_back(f);
					}
				}
				for (const auto& kv : editingEnemy_.animationMap) {
					if (!kv.first.empty()) allFlags.push_back(kv.first);
				}
				allFlags.push_back("walk");
				allFlags.push_back("boundBool");
				allFlags.push_back("followAbove");
				std::sort(allFlags.begin(), allFlags.end());
				allFlags.erase(std::unique(allFlags.begin(), allFlags.end()), allFlags.end());
				for (const auto& f : allFlags) {
					if (runtimeBoolFlags_.find(f) == runtimeBoolFlags_.end()) runtimeBoolFlags_[f] = false;
				}
			}
			if (!runtimeBoolFlags_.empty()) {
				ImGui::Separator();
				ImGui::TextDisabled("Bool Flags (toggle to test BOOL: transitions):");
				for (auto& [fname, fval] : runtimeBoolFlags_) {
					ImGui::SameLine();
					ImGui::Checkbox(fname.c_str(), &fval);
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Clear All")) {
					for (auto& [fname, fval] : runtimeBoolFlags_) fval = false;
				}
			}
		}

		ImGui::TextDisabled("Connect: drag from blue circle to another node");

		DrawLinkConditionList();

		if (Node* selectedNode = FindNodeById(nodeEditor_selectedNodeId_)) {
			ImGui::SeparatorText("Selected Node Binding");
			ImGui::Text("Selected: %s (id=%d)", selectedNode->name.c_str(), selectedNode->id);

            std::vector<std::string> boolOptions;
            boolOptions.push_back("(none)");
            bool hasWalk = false;
            for (const auto& nd : editingEnemy_.nodes) {
                if (!nd.boundBool.empty()) {
                    if (nd.boundBool == "walk") hasWalk = true;
                    bool dup = false;
                    for (const auto& b : boolOptions) { if (b == nd.boundBool) { dup = true; break; } }
                    if (!dup) boolOptions.push_back(nd.boundBool);
                }
            }
            if (!hasWalk) boolOptions.push_back("walk");
            bool hasBoundBool = false;
            bool hasFollowAbove = false;
            for (const auto& b : boolOptions) {
                if (b == "boundBool") hasBoundBool = true;
                if (b == "followAbove") hasFollowAbove = true;
            }
            if (!hasBoundBool) boolOptions.push_back("boundBool");
            if (!hasFollowAbove) boolOptions.push_back("followAbove");

            int boolIdx = 0;
            for (int bi = 0; bi < static_cast<int>(boolOptions.size()); ++bi) {
                if (selectedNode->boundBool == boolOptions[bi]) { boolIdx = bi; break; }
            }

            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::BeginCombo("Bool", boolOptions[boolIdx].c_str())) {
                for (int bi = 0; bi < static_cast<int>(boolOptions.size()); ++bi) {
                    bool isSel = (bi == boolIdx);
                    if (ImGui::Selectable(boolOptions[bi].c_str(), isSel)) {
                        selectedNode->boundBool = (bi == 0) ? std::string() : boolOptions[bi];
                        boolIdx = bi;
                    }
                    if (isSel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

			std::vector<std::string> motionFiles;
			motionFiles.push_back("");
			const std::string motionDir = "Assets/Data/Motion/";
			if (fs::exists(motionDir)) {
				for (const auto& entry : fs::directory_iterator(motionDir)) {
					if (entry.is_regular_file() && entry.path().extension() == ".json") {
						motionFiles.push_back(entry.path().stem().string());
					}
				}
			}

			int currentMotionIdx = 0;
			for (int i = 0; i < static_cast<int>(motionFiles.size()); ++i) {
				if (motionFiles[i] == selectedNode->boundMotion) {
					currentMotionIdx = i;
					break;
				}
			}

			std::string motionPreview = selectedNode->boundMotion.empty() ? "(none)" : selectedNode->boundMotion;
			ImGui::SetNextItemWidth(220.0f);
			if (ImGui::BeginCombo("Motion", motionPreview.c_str())) {
				for (int i = 0; i < static_cast<int>(motionFiles.size()); ++i) {
					std::string label = motionFiles[i].empty() ? "(none)" : motionFiles[i];
					bool isSelected = (i == currentMotionIdx);
					if (ImGui::Selectable(label.c_str(), isSelected)) {
						selectedNode->boundMotion = motionFiles[i];
						selectedNode->boundMotionNodeIndex = -1;
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			std::string stateMappedMotion;
			if (auto it = editingEnemy_.motionMap.find(selectedNode->state); it != editingEnemy_.motionMap.end()) {
				stateMappedMotion = it->second;
			}
			if (!stateMappedMotion.empty()) {
				ImGui::SameLine();
				if (ImGui::Button("Use State Motion")) {
					selectedNode->boundMotion = stateMappedMotion;
					selectedNode->boundMotionNodeIndex = -1;
				}
			}

			int motionNodeCount = 0;
			if (!selectedNode->boundMotion.empty()) {
				motionNodeCount = static_cast<int>(MotionManager::GetInstance()->GetMotion(selectedNode->boundMotion).size());
			}

			ImGui::BeginDisabled(motionNodeCount <= 0);
			std::string nodePreview = (selectedNode->boundMotionNodeIndex >= 0)
				? ("Node " + std::to_string(selectedNode->boundMotionNodeIndex))
				: std::string("(select node)");
			ImGui::SetNextItemWidth(220.0f);
			if (ImGui::BeginCombo("Motion Node", nodePreview.c_str())) {
				for (int i = 0; i < motionNodeCount; ++i) {
					std::string label = "Node " + std::to_string(i);
					bool isSelected = (selectedNode->boundMotionNodeIndex == i);
					if (ImGui::Selectable(label.c_str(), isSelected)) {
						selectedNode->boundMotionNodeIndex = i;
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::EndDisabled();

			if (ImGui::Button("Clear Binding")) {
				selectedNode->boundBool.clear();
				selectedNode->boundMotion.clear();
				selectedNode->boundMotionNodeIndex = -1;
			}
		}

		ImGui::Separator();

		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();
		if (canvasSize.x < 100) canvasSize.x = 100;
		if (canvasSize.y < 100) canvasSize.y = 100;
		nodeCanvasWidth_ = canvasSize.x;
		nodeCanvasHeight_ = canvasSize.y;

		ImGui::InvisibleButton("node_canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		ImGui::SetItemAllowOverlap();  // ノード内のUI部品（コンボ等）がクリックを受け取れるようにする
		ImVec2 origin = canvasPos;

		drawList->AddRectFilled(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), MakeCol32(40, 40, 45, 255));
		drawList->AddRect(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), MakeCol32(80, 80, 90, 255));

		{
			char buf[64]; snprintf(buf, sizeof(buf), "Nodes: %zu", editingEnemy_.nodes.size());
			drawList->AddText(ImVec2(origin.x + 6.0f, origin.y + 4.0f), MakeCol32(200, 200, 200, 180), buf);
		}

		if (nodeDragActive_) {
			auto selectedIt = std::find_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& node) { return node.id == nodeEditor_selectedNodeId_; });
			if (selectedIt != editingEnemy_.nodes.end() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !nodeLinkDragActive_) {
				selectedIt->x = mousePos.x - origin.x - nodeDragOffsetX_;
				selectedIt->y = mousePos.y - origin.y - nodeDragOffsetY_;
				if (selectedIt->x < 0.0f) selectedIt->x = 0.0f;
				if (selectedIt->y < 0.0f) selectedIt->y = 0.0f;
				if (selectedIt->x > canvasSize.x - 180.0f) selectedIt->x = canvasSize.x - 180.0f;
				if (selectedIt->y > canvasSize.y - 110.0f) selectedIt->y = canvasSize.y - 110.0f;
			} else { nodeDragActive_ = false; }
		}

		for (auto& n : editingEnemy_.nodes) {
			ImVec2 a = ImVec2(origin.x + n.x, origin.y + n.y);
			ImVec2 b = ImVec2(a.x + 180.0f, a.y + 160.0f);

			ImU32 col = MakeCol32(60, 60, 70, 220);
			if (currentStateId_ == n.id) {
				float pulse = 0.5f + 0.5f * std::sin(currentStateElapsedTime_ * 8.0f);
				col = MakeCol32(60, static_cast<int>(180 + pulse * 50.0f), 90, 255);
			} else if (nodeEditor_selectedNodeId_ == n.id) {
				col = MakeCol32(100, 80, 80, 255);
			}

			drawList->AddRectFilled(a, b, col, 6.0f);
			drawList->AddRect(a, b, MakeCol32(200, 200, 200, 220), 6.0f, 0, 2.0f);

			if (currentStateId_ == n.id) {
				drawList->AddText(ImVec2(a.x + 140.0f, a.y + 2.0f), MakeCol32(100, 255, 130, 255), "ACTIVE");
			}

			ImVec2 prevScreenPos = ImGui::GetCursorScreenPos();
			ImGui::SetCursorScreenPos(ImVec2(a.x + 6.0f, a.y + 6.0f));
			ImGui::PushID(n.id);

			char nameBufFB[128]; strncpy_s(nameBufFB, sizeof(nameBufFB), n.name.c_str(), _TRUNCATE);
			ImGui::SetNextItemWidth(150.0f);
			if (ImGui::InputText("##node_name_fb", nameBufFB, sizeof(nameBufFB))) n.name = nameBufFB;
			if (ImGui::IsItemActive()) nodeEditor_selectedNodeId_ = n.id;

			ImGui::SetCursorScreenPos(ImVec2(a.x + 6.0f, a.y + 36.0f));
			char stateBufFB[128]; strncpy_s(stateBufFB, sizeof(stateBufFB), n.state.c_str(), _TRUNCATE);
			ImGui::SetNextItemWidth(150.0f);
			if (ImGui::InputText("##node_state_fb", stateBufFB, sizeof(stateBufFB))) n.state = stateBufFB;

			// animationName inline combo box
			ImGui::SetCursorScreenPos(ImVec2(a.x + 6.0f, a.y + 66.0f));
			ImGui::SetNextItemWidth(150.0f);
			if (ImGui::BeginCombo("##node_anim_inline", n.animationName.empty() ? "(none)" : n.animationName.c_str())) {
				if (ImGui::Selectable("(none)", n.animationName.empty())) n.animationName.clear();
				for (const auto& avail : cachedAnimationNames_) {
					bool isSel = (n.animationName == avail);
					if (ImGui::Selectable(avail.c_str(), isSel)) n.animationName = avail;
					if (isSel) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// boundBool inline combo box
			ImGui::SetCursorScreenPos(ImVec2(a.x + 6.0f, a.y + 96.0f));
			ImGui::SetNextItemWidth(150.0f);
			std::vector<std::string> boolOptions;
			boolOptions.push_back("(none)");
			bool hasWalk = false;
			for (const auto& nd : editingEnemy_.nodes) {
				if (!nd.boundBool.empty()) {
					if (nd.boundBool == "walk") hasWalk = true;
					bool dup = false;
					for (const auto& opt : boolOptions) { if (opt == nd.boundBool) { dup = true; break; } }
					if (!dup) boolOptions.push_back(nd.boundBool);
				}
			}
			for (const auto& l : editingEnemy_.links) {
				if (l.condition.size() > 5 && l.condition.rfind("BOOL:", 0) == 0) {
					std::string flag = l.condition.substr(5);
					if (flag == "walk") hasWalk = true;
					bool dup = false;
					for (const auto& opt : boolOptions) { if (opt == flag) { dup = true; break; } }
					if (!dup) boolOptions.push_back(flag);
				}
			}
			for (const auto& kv : editingEnemy_.animationMap) {
				if (kv.first.empty()) continue;
				if (kv.first == "walk" || kv.first == "Walk") hasWalk = true;
				bool dup = false;
				for (const auto& opt : boolOptions) { if (opt == kv.first) { dup = true; break; } }
				if (!dup) boolOptions.push_back(kv.first);
			}
			if (!hasWalk) boolOptions.push_back("walk");
			{
				bool hasBoundBool = false;
				bool hasFollowAbove = false;
				for (const auto& opt : boolOptions) {
					if (opt == "boundBool") hasBoundBool = true;
					if (opt == "followAbove") hasFollowAbove = true;
				}
				if (!hasBoundBool) boolOptions.push_back("boundBool");
				if (!hasFollowAbove) boolOptions.push_back("followAbove");
			}
			int boolIdx = 0;
			for (int bi = 0; bi < static_cast<int>(boolOptions.size()); ++bi) {
				if (n.boundBool == boolOptions[bi]) { boolIdx = bi; break; }
			}
			
			std::string displayStr = n.boundBool.empty() ? "" : ("BOOL:" + n.boundBool);
			char boolBuf[64];
			strncpy_s(boolBuf, sizeof(boolBuf), displayStr.c_str(), _TRUNCATE);
			ImGui::SetNextItemWidth(126.0f);
			if (ImGui::InputText("##node_bool_input", boolBuf, sizeof(boolBuf))) {
				std::string newVal = boolBuf;
				if (newVal.rfind("BOOL:", 0) == 0) newVal = newVal.substr(5);
				n.boundBool = newVal;
			}
			ImGui::SameLine(0, 4.0f);
			ImGui::Button("v##node_bool_btn", ImVec2(20.0f, 0));
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				ImGui::OpenPopup("node_bool_popup");
			}

			// Loop checkbox: when enabled, the node will re-trigger (reset timer) in the
			// editor runtime when no outgoing transition is satisfied, effectively
			// looping the node until some external condition becomes true.
			ImGui::SetCursorScreenPos(ImVec2(a.x + 6.0f, a.y + 116.0f));
			ImGui::SetNextItemWidth(80.0f);
			if (ImGui::Checkbox("Loop", &n.loop)) {
				// immediate visual feedback logged
				char dbg[128]; snprintf(dbg, sizeof(dbg), "[EnemyEditor] Node %d Loop=%s", n.id, n.loop ? "ON" : "OFF"); AddLog(dbg);
			}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(90.0f);
		if (ImGui::DragFloat("Cooldown##node_loop_cd", &n.loopCooldown, 0.1f, 0.0f, 10.0f, "%.1fs")) {
			char dbg2[128]; snprintf(dbg2, sizeof(dbg2), "[EnemyEditor] Node %d LoopCooldown=%.2f", n.id, n.loopCooldown); AddLog(dbg2);
		}
		ImGui::SameLine();
		if (ImGui::Checkbox("Grounded##rg", &n.requireGrounded)) {
			char dbg3[128]; snprintf(dbg3, sizeof(dbg3), "[EnemyEditor] Node %d RequireGrounded=%s", n.id, n.requireGrounded ? "ON" : "OFF"); AddLog(dbg3);
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("ON: this state can only be entered\nwhen the enemy is on the ground");

		ImGui::SetCursorScreenPos(ImVec2(a.x + 6.0f, a.y + 136.0f));
		if (ImGui::Checkbox("Pitch by Y-Vel##pp", &n.proceduralPitch)) {
			char dbg4[128]; snprintf(dbg4, sizeof(dbg4), "[EnemyEditor] Node %d ProceduralPitch=%s", n.id, n.proceduralPitch ? "ON" : "OFF"); AddLog(dbg4);
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("ON: Leans the character forward/backward based on vertical velocity");
			if (ImGui::BeginPopup("node_bool_popup")) {
				for (int bi = 0; bi < static_cast<int>(boolOptions.size()); ++bi) {
					bool isSel = (bi == boolIdx);
					std::string label = (bi == 0) ? "(none)" : ("BOOL:" + boolOptions[bi]);
					if (ImGui::Selectable(label.c_str(), isSel)) {
						n.boundBool = (bi == 0) ? std::string() : boolOptions[bi];
					}
					if (isSel) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndPopup();
			}

			ImGui::PopID();
			ImGui::SetCursorScreenPos(prevScreenPos);

			bool hovered = (mousePos.x >= a.x && mousePos.x <= b.x && mousePos.y >= a.y && mousePos.y <= b.y);
			bool overInline = (mousePos.x >= a.x + 6.0f && mousePos.x <= a.x + 166.0f && mousePos.y >= a.y + 6.0f && mousePos.y <= a.y + 126.0f);

			if (!nodeDragActive_ && hovered && !overInline && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				nodeDragActive_ = true;
				nodeEditor_selectedNodeId_ = n.id;
				nodeDragOffsetX_ = mousePos.x - a.x;
				nodeDragOffsetY_ = mousePos.y - a.y;
			}

			if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				nodeEditor_contextNodeId_ = n.id;
				ImGui::OpenPopup("NodeContextMenu");
			}

			ImVec2 inputPortPos = ImVec2(a.x + 8.0f, a.y + 146.0f);
			ImVec2 portPos = ImVec2(b.x - 8.0f, a.y + 146.0f);
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

		for (size_t i = 0; i < editingEnemy_.links.size(); ++i) {
			const auto& l = editingEnemy_.links[i];
			const Node* from = nullptr; const Node* to = nullptr;
			for (const auto& n : editingEnemy_.nodes) {
				if (n.id == l.from) from = &n;
				if (n.id == l.to) to = &n;
			}
			if (from && to) {
				ImVec2 pa = ImVec2(origin.x + from->x + 180.0f - 8.0f, origin.y + from->y + 146.0f);
				ImVec2 pb = ImVec2(origin.x + to->x + 8.0f, origin.y + to->y + 146.0f);
				drawList->AddBezierCubic(pa, ImVec2(pa.x + 40, pa.y), ImVec2(pb.x - 40, pb.y), pb, MakeCol32(200, 200, 100, 220), 3.0f);

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					if (DistPtSegSq(mousePos.x, mousePos.y, pa.x, pa.y, pb.x, pb.y) <= 144.0f) {
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
					ImVec2 nb = ImVec2(na.x + 180.0f, na.y + 160.0f);
					if (mousePos.x >= na.x && mousePos.x <= nb.x && mousePos.y >= na.y && mousePos.y <= nb.y) { targetId = n.id; break; }
				}
				if (targetId != -1 && targetId != nodeEditor_linkStartId_) {
					bool exists = false;
					for (const auto& l : editingEnemy_.links) if (l.from == nodeEditor_linkStartId_ && l.to == targetId) exists = true;
					if (!exists) {
						PushUndoState();
						Link link; link.from = nodeEditor_linkStartId_; link.to = targetId; link.condition = "Always";
						editingEnemy_.links.push_back(link);
						char dbg[256]; snprintf(dbg, sizeof(dbg), "[EnemyEditor] Link %d -> %d", link.from, link.to); AddLog(dbg);
					}
				}
				nodeLinkDragActive_ = false;
				nodeEditor_linkStartId_ = -1;
			}
		}

		{
			ImGui::SetCursorScreenPos(ImVec2(origin.x + 6.0f, origin.y + canvasSize.y + 8.0f));
			if (ImGui::Button("Undo", ImVec2(100, 0))) { if (CanUndo()) Undo(); }
			ImGui::SameLine();
			if (ImGui::Button("Save Nodes", ImVec2(120, 0))) { SaveEnemy(editingEnemy_); AddLog("[EnemyEditor] Saved nodes"); }
			ImGui::Dummy(ImVec2(0.0f, 8.0f));
		}

		bool requestDeleteNode = false;
		if (ImGui::BeginPopup("NodeContextMenu")) {
			if (nodeEditor_contextNodeId_ != -1) {
				auto nit = std::find_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& nd) { return nd.id == nodeEditor_contextNodeId_; });
				std::string nlabel = "Node";
				if (nit != editingEnemy_.nodes.end()) nlabel = nit->name + " (id=" + std::to_string(nit->id) + ")";
				ImGui::TextDisabled("%s", nlabel.c_str());
				ImGui::Separator();
				if (ImGui::MenuItem("Delete Node...")) {
					nodeEditor_pendingDeleteNodeId_ = nodeEditor_contextNodeId_;
					requestDeleteNode = true;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
		if (requestDeleteNode) ImGui::OpenPopup("ConfirmDeleteNode");

		if (ImGui::BeginPopupModal("ConfirmDeleteNode", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			int delId = nodeEditor_pendingDeleteNodeId_;
			if (delId != -1) {
				auto dit = std::find_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& nd) { return nd.id == delId; });
				std::string dname = (dit != editingEnemy_.nodes.end()) ? dit->name : "<unknown>";
				ImGui::Text("Delete node '%s' (id=%d)?", dname.c_str(), delId);
				ImGui::Separator();
				if (ImGui::Button("Delete", ImVec2(120, 0))) {
					PushUndoState();
					editingEnemy_.links.erase(std::remove_if(editingEnemy_.links.begin(), editingEnemy_.links.end(), [&](const Link& lk) { return lk.from == delId || lk.to == delId; }), editingEnemy_.links.end());
					editingEnemy_.nodes.erase(std::remove_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& nd) { return nd.id == delId; }), editingEnemy_.nodes.end());
					if (nodeEditor_selectedNodeId_ == delId) nodeEditor_selectedNodeId_ = -1;
					if (currentStateId_ == delId) currentStateId_ = editingEnemy_.nodes.empty() ? -1 : editingEnemy_.nodes.front().id;
					char dbg[256]; snprintf(dbg, sizeof(dbg), "[EnemyEditor] Deleted node %d", delId); AddLog(dbg);
					SaveEnemy(editingEnemy_);
					s_actionSaveNotificationTimer = 2.0f;
					nodeEditor_pendingDeleteNodeId_ = -1;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) { nodeEditor_pendingDeleteNodeId_ = -1; ImGui::CloseCurrentPopup(); }
			} else {
				ImGui::Text("No node selected.");
				if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		bool requestDeleteLink = false;
		if (ImGui::BeginPopup("LinkContextMenu")) {
			if (linkEditor_contextLinkIndex_ >= 0 && linkEditor_contextLinkIndex_ < static_cast<int>(editingEnemy_.links.size())) {
				auto& linkRef = editingEnemy_.links[linkEditor_contextLinkIndex_];
				ImGui::TextDisabled("Link %d -> %d", linkRef.from, linkRef.to);
				ImGui::Separator();
				char condBuf[256]; strncpy_s(condBuf, sizeof(condBuf), linkRef.condition.c_str(), _TRUNCATE);
				if (ImGui::InputText("Condition", condBuf, sizeof(condBuf))) linkRef.condition = condBuf;
				if (ImGui::MenuItem("Delete Link...")) {
					linkEditor_pendingDeleteLinkIndex_ = linkEditor_contextLinkIndex_;
					requestDeleteLink = true;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
		if (requestDeleteLink) ImGui::OpenPopup("ConfirmDeleteLink");

		if (ImGui::BeginPopupModal("ConfirmDeleteLink", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			int idx = linkEditor_pendingDeleteLinkIndex_;
			if (idx >= 0 && idx < static_cast<int>(editingEnemy_.links.size())) {
				auto lnk = editingEnemy_.links[idx];
				ImGui::Text("Delete link '%d -> %d'?", lnk.from, lnk.to);
				ImGui::Separator();
				if (ImGui::Button("Delete", ImVec2(120, 0))) {
					PushUndoState();
					editingEnemy_.links.erase(std::remove_if(editingEnemy_.links.begin(), editingEnemy_.links.end(), [&](const Link& l) { return l.from == lnk.from && l.to == lnk.to && l.condition == lnk.condition; }), editingEnemy_.links.end());
					char dbg[256]; snprintf(dbg, sizeof(dbg), "[EnemyEditor] Deleted link %d -> %d", lnk.from, lnk.to); AddLog(dbg);
					SaveEnemy(editingEnemy_);
					s_actionSaveNotificationTimer = 2.0f;
					linkEditor_pendingDeleteLinkIndex_ = -1; linkEditor_contextLinkIndex_ = -1;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) { linkEditor_pendingDeleteLinkIndex_ = -1; ImGui::CloseCurrentPopup(); }
			} else {
				ImGui::Text("No link selected.");
				if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void EnemyActionEditor::EvaluateStateMachine() {
		if (currentStateId_ == -1) return;
		if (lockStateMachineAfterStartFirstNode_ && !firstNodeStarted_) return;
		const int firstId = editingEnemy_.nodes.empty() ? -1 : editingEnemy_.nodes.front().id;
		if (requireManualStart_ && currentStateId_ == firstId && !firstNodeStarted_) return;

        bool transitioned = false;
		for (const auto& link : editingEnemy_.links) {
			if (link.from != currentStateId_) continue;
			if (CheckLinkCondition(link)) {
				transitioned = true;
				if (link.to != currentStateId_) {
					int old = currentStateId_;
					previousStateId_ = old;
					currentStateId_ = link.to;
					currentStateElapsedTime_ = 0.0f;
					transitionFlashTimer_ = 1.0f;
					char buf[256]; snprintf(buf, sizeof(buf), "[StateMachine] %d -> %d (%s)", old, currentStateId_, link.condition.c_str()); AddLog(buf);
				{
						auto newNodeIt = std::find_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& nd) { return nd.id == currentStateId_; });
						if (newNodeIt != editingEnemy_.nodes.end() && !newNodeIt->boundBool.empty()) {
							runtimeBoolFlags_[newNodeIt->boundBool] = true;
						}
					}
					if (requireManualStart_ && old == firstId) firstNodeStarted_ = false;
				} else {
					// Self-loop: re-trigger the current node (loop) by resetting its timer and
					// optionally firing its boundBool so editor/runtime can observe repeated entry.
					currentStateElapsedTime_ = 0.0f;
					transitionFlashTimer_ = 0.5f;
					char buf[256]; snprintf(buf, sizeof(buf), "[StateMachine] Self-loop id=%d (%s)", currentStateId_, link.condition.c_str()); AddLog(buf);
					auto curNodeIt = std::find_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& nd) { return nd.id == currentStateId_; });
					if (curNodeIt != editingEnemy_.nodes.end() && !curNodeIt->boundBool.empty()) {
						runtimeBoolFlags_[curNodeIt->boundBool] = true;
					}
				}
				break;
			}
		}

		// If no transition occurred and the current node is marked `loop`, then
		// periodically re-trigger the node (reset its timer and fire boundBool)
		// so it repeats until some outgoing condition becomes true. Use the
		// node's `splineDuration` as the loop interval when available.
		if (!transitioned) {
			auto curNodeIt = std::find_if(editingEnemy_.nodes.begin(), editingEnemy_.nodes.end(), [&](const Node& nd) { return nd.id == currentStateId_; });
                if (curNodeIt != editingEnemy_.nodes.end() && curNodeIt->loop) {
					float base = (curNodeIt->splineDuration > 0.0f) ? curNodeIt->splineDuration : 1.0f;
					float loopInterval = base + curNodeIt->loopCooldown;
					if (currentStateElapsedTime_ >= loopInterval) {
					currentStateElapsedTime_ = 0.0f;
					transitionFlashTimer_ = 0.5f;
					char dbg[256]; snprintf(dbg, sizeof(dbg), "[StateMachine] Looping node id=%d", currentStateId_); AddLog(dbg);
					if (!curNodeIt->boundBool.empty()) runtimeBoolFlags_[curNodeIt->boundBool] = true;
				}
			}
		}
	}

	bool EnemyActionEditor::CheckLinkCondition(const Link& link) {
		if (link.condition.empty()) return true;
		std::string c = link.condition;
		while (!c.empty() && c.front() == ' ') c.erase(c.begin());
		while (!c.empty() && c.back() == ' ') c.pop_back();
		if (c == "Always") return true;

		if (c.rfind("BOOL:", 0) == 0) {
			std::string flag = c.substr(5);
			while (!flag.empty() && flag.front() == ' ') flag.erase(flag.begin());
			while (!flag.empty() && flag.back() == ' ') flag.pop_back();
			auto it = runtimeBoolFlags_.find(flag);
			if (it != runtimeBoolFlags_.end()) return it->second;
			return false;
		}

		if (c.rfind("Time>=", 0) == 0) { try { return currentStateElapsedTime_ >= std::stof(c.substr(6)); } catch (...) { return false; } }
		if (c.rfind("Time>", 0) == 0) { try { return currentStateElapsedTime_ > std::stof(c.substr(5)); } catch (...) { return false; } }

		float hpRatio = (editingEnemy_.hp > 0) ? static_cast<float>(editingEnemy_.hp) / 100.0f : 0.0f;
		if (c.rfind("HP<=", 0) == 0) { try { return hpRatio <= std::stof(c.substr(4)); } catch (...) { return false; } }
		if (c.rfind("HP<", 0) == 0) { try { return hpRatio < std::stof(c.substr(3)); } catch (...) { return false; } }
		if (c.rfind("HP>=", 0) == 0) { try { return hpRatio >= std::stof(c.substr(4)); } catch (...) { return false; } }
		if (c.rfind("HP>", 0) == 0) { try { return hpRatio > std::stof(c.substr(3)); } catch (...) { return false; } }
		if (c.rfind("HP==", 0) == 0) { try { return std::abs(hpRatio - std::stof(c.substr(4))) < 0.001f; } catch (...) { return false; } }

		return false;
	}

	bool EnemyActionEditor::HasOutgoingTransition(int nodeId) const {
		for (const auto& l : editingEnemy_.links) {
			if (l.from == nodeId && l.to != nodeId) return true;
		}
		return false;
	}

	bool EnemyActionEditor::HasTimeDrivenTransition(int nodeId) const {
		for (const auto& l : editingEnemy_.links) {
			if (l.from != nodeId) continue;
			std::string c = l.condition;
			if (c.rfind("Time>=", 0) == 0 || c.rfind("Time>", 0) == 0) return true;
		}
		return false;
	}

	void EnemyActionEditor::DrawStateMachineControlUI() {
	}

	void EnemyActionEditor::DrawLinkConditionList() {
		if (editingEnemy_.links.empty()) return;
		if (ImGui::TreeNode("Link Conditions")) {
			for (size_t i = 0; i < editingEnemy_.links.size(); ++i) {
				auto& l = editingEnemy_.links[i];
				ImGui::PushID(static_cast<int>(i));
			std::string fromName = "?", toName = "?";
				for (const auto& n : editingEnemy_.nodes) {
					if (n.id == l.from) fromName = n.name;
					if (n.id == l.to) toName = n.name;
				}
				ImGui::Text("%s -> %s", fromName.c_str(), toName.c_str());
				ImGui::SameLine();

				const char* condTypes[] = { "Always", "Time>=", "Time>", "HP<=", "HP<", "HP>=", "HP>", "HP==", "BOOL:", "Custom" };
				int currentType = 9; // default Custom
				for (int ct = 0; ct < 9; ++ct) {
					if (l.condition.rfind(condTypes[ct], 0) == 0) { currentType = ct; break; }
				}
				if (l.condition == "Always") currentType = 0;

				ImGui::SetNextItemWidth(90.0f);
				if (ImGui::BeginCombo("##cond_type", condTypes[currentType])) {
					for (int ct = 0; ct < 10; ++ct) {
						if (ImGui::Selectable(condTypes[ct], currentType == ct)) {
							if (ct == 0) l.condition = "Always";
							else if (ct < 8) l.condition = std::string(condTypes[ct]) + "1.0";
							else if (ct == 8) l.condition = "BOOL:";
							currentType = ct;
						}
					}
					ImGui::EndCombo();
				}

				if (currentType >= 1 && currentType <= 7) {
					ImGui::SameLine();
					std::string prefix = condTypes[currentType];
					float val = 0.0f;
					if (l.condition.size() > prefix.size()) {
						try { val = std::stof(l.condition.substr(prefix.size())); } catch (...) {}
					}
					ImGui::SetNextItemWidth(80.0f);
					if (currentType <= 2) { // Time
						if (ImGui::DragFloat("##val", &val, 0.1f, 0.0f, 60.0f, "%.1f s")) {
							l.condition = prefix + std::to_string(val);
						}
					} else { // HP
						if (ImGui::DragFloat("##val", &val, 0.01f, 0.0f, 1.0f, "%.2f")) {
							l.condition = prefix + std::to_string(val);
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Condition value");
				} else if (currentType == 8) {
					ImGui::SameLine();
					std::string flag;
					if (l.condition.size() > 5) flag = l.condition.substr(5);
					std::vector<std::string> availBools;
					bool hasWalk = false;
					for (const auto& nd : editingEnemy_.nodes) {
						if (!nd.boundBool.empty()) {
							if (nd.boundBool == "walk" || nd.boundBool == "Walk") hasWalk = true;
							bool dup = false;
							for (const auto& ab : availBools) { if (ab == nd.boundBool) { dup = true; break; } }
							if (!dup) availBools.push_back(nd.boundBool);
						}
					}
					for (const auto& lk : editingEnemy_.links) {
						if (lk.condition.size() > 5 && lk.condition.rfind("BOOL:", 0) == 0) {
							std::string fl = lk.condition.substr(5);
							if (fl == "walk" || fl == "Walk") hasWalk = true;
							bool dup = false;
							for (const auto& ab : availBools) { if (ab == fl) { dup = true; break; } }
							if (!dup) availBools.push_back(fl);
						}
					}
					for (const auto& kv : editingEnemy_.animationMap) {
						if (kv.first.empty()) continue;
						if (kv.first == "walk" || kv.first == "Walk") hasWalk = true;
						bool dup = false;
						for (const auto& ab : availBools) { if (ab == kv.first) { dup = true; break; } }
						if (!dup) availBools.push_back(kv.first);
					}
					if (!hasWalk) availBools.push_back("walk");
					{
						bool hasBoundBool = false;
						bool hasFollowAbove = false;
						for (const auto& ab : availBools) {
							if (ab == "boundBool") hasBoundBool = true;
							if (ab == "followAbove") hasFollowAbove = true;
						}
						if (!hasBoundBool) availBools.push_back("boundBool");
						if (!hasFollowAbove) availBools.push_back("followAbove");
					}
					std::string boolPreview = flag.empty() ? "(select flag)" : flag;
					ImGui::SetNextItemWidth(120.0f);
					if (ImGui::BeginCombo("##bool_flag", boolPreview.c_str())) {
						for (const auto& bf : availBools) {
							if (ImGui::Selectable(bf.c_str(), bf == flag)) {
								l.condition = "BOOL:" + bf;
							}
						}
						ImGui::EndCombo();
					}
				} else if (currentType == 9) {
					ImGui::SameLine();
					char cbuf[256]; strncpy_s(cbuf, sizeof(cbuf), l.condition.c_str(), _TRUNCATE);
					ImGui::SetNextItemWidth(120.0f);
					if (ImGui::InputText("##custom", cbuf, sizeof(cbuf))) l.condition = cbuf;
				}

				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}

	void EnemyActionEditor::PushUndoState() {
		if (!activeFileName_.empty()) {
			perFileRuntimes_[activeFileName_].undoStack.push_back(editingEnemy_);
			if (perFileRuntimes_[activeFileName_].undoStack.size() > undoStackMax_)
				perFileRuntimes_[activeFileName_].undoStack.erase(perFileRuntimes_[activeFileName_].undoStack.begin());
		} else {
			undoStack_.push_back(editingEnemy_);
			if (undoStack_.size() > undoStackMax_) undoStack_.erase(undoStack_.begin());
		}
	}

	void EnemyActionEditor::Undo() {
		if (!activeFileName_.empty()) {
			auto& stk = perFileRuntimes_[activeFileName_].undoStack;
			if (stk.empty()) return;
			editingEnemy_ = stk.back();
			stk.pop_back();
			if (!editingEnemy_.nodes.empty()) currentStateId_ = editingEnemy_.nodes.front().id;
			else currentStateId_ = -1;
			AddLog("[EnemyEditor] Undo performed (per-file)");
		} else {
			if (undoStack_.empty()) return;
			editingEnemy_ = undoStack_.back();
			undoStack_.pop_back();
			if (!editingEnemy_.nodes.empty()) currentStateId_ = editingEnemy_.nodes.front().id;
			else currentStateId_ = -1;
			AddLog("[EnemyEditor] Undo performed");
		}
	}

	void EnemyActionEditor::AddLog(const std::string& msg) {
		std::string line = msg;
		if (!line.empty() && line.back() != '\n') line.push_back('\n');
		editorLog_.push_back(line);
		if (editorLog_.size() > editorLogMax_) editorLog_.erase(editorLog_.begin(), editorLog_.begin() + static_cast<long long>(editorLog_.size() - editorLogMax_));
	}

	void EnemyActionEditor::DrawEditorLogUI() {
		ImGui::SetNextWindowPos(ImVec2(10.0f, 540.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(600.0f, 180.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Editor Log", nullptr, ImGuiWindowFlags_NoCollapse);
		if (ImGui::Button("Clear")) editorLog_.clear();
		ImGui::SameLine();
		ImGui::Text("Lines: %zu", editorLog_.size());
		ImGui::Separator();
		ImGui::BeginChild("LogRegion", ImVec2(0, 0), ImGuiChildFlags_None);
		for (const auto& line : editorLog_) ImGui::TextUnformatted(line.c_str());
		ImGui::SetScrollHereY(1.0f);
		ImGui::EndChild();
		ImGui::End();
	}
#endif
}

