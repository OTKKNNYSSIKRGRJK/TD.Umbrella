module Game.Editor.AreaEditor;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import <string>;
import <map>;
import <algorithm>;
import <filesystem>;
import <vector>;
import <fstream>;
import nlohmann.json;

#if defined(_DEBUG)
namespace {
	constexpr ImU32 MakeCol32(int r, int g, int b, int a) {
		return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r);
	}

	bool IsConvex(const std::vector<Game::Editor::CollisionPoint>& points) {
		if (points.size() < 3) return true;

		bool hasPositive = false;
		bool hasNegative = false;

		for (size_t i = 0; i < points.size(); ++i) {
			size_t prev = (i == 0) ? points.size() - 1 : i - 1;
			size_t next = (i == points.size() - 1) ? 0 : i + 1;

			float dx1 = points[i].position.x - points[prev].position.x;
			float dy1 = points[i].position.y - points[prev].position.y;
			
			float dx2 = points[next].position.x - points[i].position.x;
			float dy2 = points[next].position.y - points[i].position.y;

			float cross = dx1 * dy2 - dy1 * dx2;

			if (cross > 0.001f) hasPositive = true;
			if (cross < -0.001f) hasNegative = true;

			if (hasPositive && hasNegative) return false;
		}

		return true;
	}
}
#endif

namespace Game::Editor {
	void AreaEditor::Update() {
#if defined(_DEBUG)
		DrawEditorUI();
#endif
	}

#if defined(_DEBUG)
	void AreaEditor::DrawEditorUI() {
		bool openConvexError = false;
		auto trySaveArea = [&](const AreaData& areaToSave, bool sync) {
			for (const auto& cg : areaToSave.collisionGroups) {
				if (!IsConvex(cg.points)) {
					openConvexError = true;
					return false;
				}
			}
			SaveArea(areaToSave, sync);
			return true;
		};

		// 右ドラッグでカメラ移動
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			cameraPos_.x += delta.x;
			cameraPos_.y += delta.y;
		}

		// 敵JSONファイルリストをディレクトリ変更時のみ再スキャン
		static std::filesystem::file_time_type lastScanDirTime{};
		try {
			auto currentDirTime = std::filesystem::last_write_time("./");
			if (lastScanDirTime != currentDirTime) {
				lastScanDirTime = currentDirTime;
				enemyFiles_.clear();
				for (const auto& entry : std::filesystem::directory_iterator("./")) {
					try {
						if (!entry.is_regular_file()) continue;
						std::string fName = entry.path().filename().string();
						if (entry.path().extension() == ".json" && fName.find("area") != 0) {
							std::ifstream ifs(entry.path());
							if (ifs.is_open()) {
								nlohmann::json j;
								ifs >> j;
								// 敵データ固有のプロパティの有無で判別
								if (j.is_object() && j.contains("hp") && j.contains("gltfPath") && j.contains("aggroRadius")) {
									std::string baseName = fName.substr(0, fName.size() - 5);
									enemyFiles_.push_back(baseName);
								}
							}
						}
					} catch (...) {
						// パースエラーの無関係なJSON（vcpkg.json等）は無視
					}
				}
			}
		} catch (...) {}

		float scale = 0.5f;
		float cx = cameraPos_.x;
		float cy = cameraPos_.y;
		ImVec2 mousePos = ImGui::GetMousePos();

		// エリア外に出ないように座標補正
		for (auto& conn : editingArea_.connections) {
			conn.trigger.position.x = (std::max)(0.0f, (std::min)(conn.trigger.position.x, (std::max)(0.0f, static_cast<float>(editingArea_.width) - conn.trigger.size.x)));
			conn.trigger.position.y = (std::max)(0.0f, (std::min)(conn.trigger.position.y, (std::max)(0.0f, static_cast<float>(editingArea_.height) - conn.trigger.size.y)));
		}

		// 敵配置もエリア内に制限
		for (auto& ep : editingArea_.enemies) {
			ep.position.x = (std::max)(0.0f, (std::min)(ep.position.x, static_cast<float>(editingArea_.width)));
			ep.position.y = (std::max)(0.0f, (std::min)(ep.position.y, static_cast<float>(editingArea_.height)));
		}

		// 当たり判定もエリア内に制限
		for (auto& cg : editingArea_.collisionGroups) {
			for (auto& p : cg.points) {
				p.position.x = (std::max)(0.0f, (std::min)(p.position.x, static_cast<float>(editingArea_.width)));
				p.position.y = (std::max)(0.0f, (std::min)(p.position.y, static_cast<float>(editingArea_.height)));
			}
		}

		constexpr float enemyMarkerSize = 16.0f;

		// 左クリックによるドラッグ＆ドロップ判定
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse) {
			draggingConnectionIndex_ = -1;
			draggingAreaIndex_ = -1;
			draggingEnemyIndex_ = -1;
			draggingCollisionGroupIndex_ = -1;
			draggingCollisionPointIndex_ = -1;

			for (int i = 0; i < static_cast<int>(editingArea_.connections.size()); ++i) {
				const auto& conn = editingArea_.connections[i];
				float cxmin = cx + (editingArea_.editorPos.x + conn.trigger.position.x) * scale;
				float cymin = cy - (editingArea_.editorPos.y + conn.trigger.position.y + conn.trigger.size.y) * scale;
				float cxmax = cxmin + conn.trigger.size.x * scale;
				float cymax = cy - (editingArea_.editorPos.y + conn.trigger.position.y) * scale;

				if (mousePos.x >= cxmin && mousePos.x <= cxmax && mousePos.y >= cymin && mousePos.y <= cymax) {
					draggingConnectionIndex_ = i;
					dragOffset_ = { mousePos.x - cxmin, mousePos.y - cymin };
					break;
				}
			}

			// 敵配置のドラッグ判定
			if (draggingConnectionIndex_ == -1) {
				for (int i = 0; i < static_cast<int>(editingArea_.enemies.size()); ++i) {
					const auto& ep = editingArea_.enemies[i];
					float ecx = cx + (editingArea_.editorPos.x + ep.position.x) * scale;
					float ecy = cy - (editingArea_.editorPos.y + ep.position.y) * scale;
					float exmin = ecx - enemyMarkerSize * scale;
					float exmax = ecx + enemyMarkerSize * scale;
					float eymin = ecy - enemyMarkerSize * scale;
					float eymax = ecy + enemyMarkerSize * scale;

					if (mousePos.x >= exmin && mousePos.x <= exmax && mousePos.y >= eymin && mousePos.y <= eymax) {
						draggingEnemyIndex_ = i;
						dragOffset_ = { mousePos.x - ecx, mousePos.y - ecy };
						break;
					}
				}
			}

			// 当たり判定のドラッグ判定
			if (draggingConnectionIndex_ == -1 && draggingEnemyIndex_ == -1) {
				for (int g = 0; g < static_cast<int>(editingArea_.collisionGroups.size()); ++g) {
					auto& cg = editingArea_.collisionGroups[g];
					for (int p = 0; p < static_cast<int>(cg.points.size()); ++p) {
						const auto& pt = cg.points[p];
						float pcx = cx + (editingArea_.editorPos.x + pt.position.x) * scale;
						float pcy = cy - (editingArea_.editorPos.y + pt.position.y) * scale;
						float pxmin = pcx - pt.radius * scale;
						float pxmax = pcx + pt.radius * scale;
						float pymin = pcy - pt.radius * scale;
						float pymax = pcy + pt.radius * scale;

						if (mousePos.x >= pxmin && mousePos.x <= pxmax && mousePos.y >= pymin && mousePos.y <= pymax) {
							draggingCollisionGroupIndex_ = g;
							draggingCollisionPointIndex_ = p;
							dragOffset_ = { mousePos.x - pcx, mousePos.y - pcy };
							break;
						}
					}
					if (draggingCollisionPointIndex_ != -1) break;
				}
			}

			if (draggingConnectionIndex_ == -1 && draggingEnemyIndex_ == -1 && draggingCollisionGroupIndex_ == -1) {
				float axmin = cx + editingArea_.editorPos.x * scale;
				float aymin = cy - (editingArea_.editorPos.y + editingArea_.height) * scale;
				float axmax = axmin + editingArea_.width * scale;
				float aymax = cy - editingArea_.editorPos.y * scale;

				if (mousePos.x >= axmin && mousePos.x <= axmax && mousePos.y >= aymin && mousePos.y <= aymax) {
					draggingAreaIndex_ = 0;
					dragOffset_ = { mousePos.x - axmin, mousePos.y - aymin };
				} else {
					for (const auto& a : allAreas_) {
						if (a.name == editingArea_.name) continue;
						float paxmin = cx + a.editorPos.x * scale;
						float paymin = cy - (a.editorPos.y + a.height) * scale;
						float paxmax = paxmin + a.width * scale;
						float paymax = cy - a.editorPos.y * scale;
						if (mousePos.x >= paxmin && mousePos.x <= paxmax && mousePos.y >= paymin && mousePos.y <= paymax) {
							if (trySaveArea(editingArea_, true)) {
								LoadArea(editingArea_, "area" + std::to_string(a.name) + ".json");
							}
							break;
						}
					}
				}
			}
		}

		// 左ドラッグ中
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			if (draggingConnectionIndex_ != -1) {
				auto& conn = editingArea_.connections[draggingConnectionIndex_];
				float new_cxmin = mousePos.x - dragOffset_.x;
				float new_cymin = mousePos.y - dragOffset_.y;
				conn.trigger.position.x = (new_cxmin - cx) / scale - editingArea_.editorPos.x;
				conn.trigger.position.y = (cy - new_cymin) / scale - editingArea_.editorPos.y - conn.trigger.size.y;

				conn.trigger.position.x = (std::max)(0.0f, (std::min)(conn.trigger.position.x, (std::max)(0.0f, static_cast<float>(editingArea_.width) - conn.trigger.size.x)));
				conn.trigger.position.y = (std::max)(0.0f, (std::min)(conn.trigger.position.y, (std::max)(0.0f, static_cast<float>(editingArea_.height) - conn.trigger.size.y)));
			} else if (draggingEnemyIndex_ != -1) {
				auto& ep = editingArea_.enemies[draggingEnemyIndex_];
				float newEcx = mousePos.x - dragOffset_.x;
				float newEcy = mousePos.y - dragOffset_.y;
				ep.position.x = (newEcx - cx) / scale - editingArea_.editorPos.x;
				ep.position.y = (cy - newEcy) / scale - editingArea_.editorPos.y;

				ep.position.x = (std::max)(0.0f, (std::min)(ep.position.x, static_cast<float>(editingArea_.width)));
				ep.position.y = (std::max)(0.0f, (std::min)(ep.position.y, static_cast<float>(editingArea_.height)));
			} else if (draggingCollisionGroupIndex_ != -1) {
				auto& cg = editingArea_.collisionGroups[draggingCollisionGroupIndex_];
				if (draggingCollisionPointIndex_ != -1) {
					auto& pt = cg.points[draggingCollisionPointIndex_];
					float new_pcx = mousePos.x - dragOffset_.x;
					float new_pcy = mousePos.y - dragOffset_.y;
					pt.position.x = (new_pcx - cx) / scale - editingArea_.editorPos.x;
					pt.position.y = (cy - new_pcy) / scale - editingArea_.editorPos.y;

					pt.position.x = (std::max)(0.0f, (std::min)(pt.position.x, static_cast<float>(editingArea_.width)));
					pt.position.y = (std::max)(0.0f, (std::min)(pt.position.y, static_cast<float>(editingArea_.height)));
				}
			} else if (draggingAreaIndex_ == 0) {
				float new_axmin = mousePos.x - dragOffset_.x;
				float new_aymin = mousePos.y - dragOffset_.y;
				editingArea_.editorPos.x = (new_axmin - cx) / scale;
				editingArea_.editorPos.y = (cy - new_aymin) / scale - editingArea_.height;
			}
		}

		// 左クリック離し
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if (draggingConnectionIndex_ != -1 || draggingAreaIndex_ != -1 || draggingEnemyIndex_ != -1 || draggingCollisionGroupIndex_ != -1) {
				trySaveArea(editingArea_, true);
			}
			draggingConnectionIndex_ = -1;
			draggingAreaIndex_ = -1;
			draggingEnemyIndex_ = -1;
			draggingCollisionGroupIndex_ = -1;
			draggingCollisionPointIndex_ = -1;
		}

		// バックグラウンド描画リストでエリアとコネクションを描画
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();

		for (const auto& a : allAreas_) {
			bool isEditing = (a.name == editingArea_.name);
			const AreaData& drawData = isEditing ? editingArea_ : a;

			ImVec2 areaMin(cx + drawData.editorPos.x * scale, cy - (drawData.editorPos.y + drawData.height) * scale);
			ImVec2 areaMax(cx + (drawData.editorPos.x + drawData.width) * scale, cy - drawData.editorPos.y * scale);

			ImU32 bgColor = isEditing ? MakeCol32(80, 80, 80, 255) : MakeCol32(40, 40, 40, 255);
			ImU32 borderColor = isEditing ? MakeCol32(255, 255, 255, 255) : MakeCol32(150, 150, 150, 255);

			drawList->AddRectFilled(areaMin, areaMax, bgColor);
			drawList->AddRect(areaMin, areaMax, borderColor, 0.0f, 0, isEditing ? 2.0f : 1.0f);
			drawList->AddText(ImVec2(areaMin.x + 5, areaMin.y + 5), MakeCol32(220, 220, 220, 255), std::to_string(drawData.name).c_str());

			if (isEditing) {
				drawList->AddText(ImVec2(areaMin.x + 5, areaMin.y + 20), MakeCol32(100, 255, 100, 255), "[Editing]");
			}

			// Draw TerrainEditor Polygons
			if (drawData.originalJson.contains("Polygons") && drawData.originalJson["Polygons"].is_array()) {
				for (const auto& poly : drawData.originalJson["Polygons"]) {
					if (poly.contains("Vertices") && poly["Vertices"].is_array()) {
						const auto& verts = poly["Vertices"];
						if (verts.size() >= 3) {
							std::vector<ImVec2> points;
							for (const auto& v : verts) {
								if (v.contains("Pos") && v["Pos"].is_array() && v["Pos"].size() >= 2) {
									float px = v["Pos"][0].get<float>();
									float py = v["Pos"][1].get<float>();
									points.push_back(ImVec2(
										cx + (drawData.editorPos.x + px) * scale,
										cy - (drawData.editorPos.y + drawData.height - py) * scale
									));
								}
							}
							if (points.size() >= 3) {
								drawList->AddConvexPolyFilled(points.data(), static_cast<int>(points.size()), MakeCol32(100, 200, 100, isEditing ? 80 : 30));
								drawList->AddPolyline(points.data(), static_cast<int>(points.size()), MakeCol32(150, 255, 150, isEditing ? 255 : 100), ImDrawFlags_Closed, 1.5f);
							}
						}
					}
				}
			}

			// Draw TerrainEditor GroundPoints
			if (drawData.originalJson.contains("GroundPoints") && drawData.originalJson["GroundPoints"].is_array()) {
				const auto& gp = drawData.originalJson["GroundPoints"];
				for (const auto& pt : gp) {
					if (pt.contains("NextID") && pt.contains("Pos") && pt["Pos"].is_array() && pt["Pos"].size() >= 2) {
						int nextID = pt["NextID"].template get<int>();
						if (nextID != -1) {
							for (const auto& npt : gp) {
								if (npt.contains("ID") && npt["ID"].template get<int>() == nextID && npt.contains("Pos") && npt["Pos"].is_array() && npt["Pos"].size() >= 2) {
									float x1 = pt["Pos"][0].template get<float>();
									float y1 = pt["Pos"][1].template get<float>();
									float x2 = npt["Pos"][0].template get<float>();
									float y2 = npt["Pos"][1].template get<float>();
									
									ImVec2 startP(
										cx + (drawData.editorPos.x + x1) * scale,
										cy - (drawData.editorPos.y + drawData.height - y1) * scale
									);
									ImVec2 endP(
										cx + (drawData.editorPos.x + x2) * scale,
										cy - (drawData.editorPos.y + drawData.height - y2) * scale
									);
									drawList->AddLine(startP, endP, MakeCol32(100, 255, 100, isEditing ? 255 : 150), 4.0f * scale);
									break;
								}
							}
						}
					}
				}
			}

			std::map<int, int> targetCount;
			for (const auto& conn : drawData.connections) {
				int currentIdx = targetCount[conn.targetAreaIndex]++;
				ImVec2 connMin(cx + (drawData.editorPos.x + conn.trigger.position.x) * scale, cy - (drawData.editorPos.y + conn.trigger.position.y + conn.trigger.size.y) * scale);
				ImVec2 connMax(cx + (drawData.editorPos.x + conn.trigger.position.x + conn.trigger.size.x) * scale, cy - (drawData.editorPos.y + conn.trigger.position.y) * scale);

				bool isPlayerStart = (drawData.index == 0 && conn.targetAreaIndex == 0);
				ImU32 fillColor = isPlayerStart ? MakeCol32(255, 120, 0, isEditing ? 100 : 50) : MakeCol32(0, 150, 255, isEditing ? 100 : 50);
				ImU32 outlineColor = isPlayerStart ? MakeCol32(255, 200, 0, 255) : MakeCol32(0, 255, 255, 255);

				drawList->AddRectFilled(connMin, connMax, fillColor);
				drawList->AddRect(connMin, connMax, outlineColor, 0.0f, 0, 1.0f);

				std::string targetText = isPlayerStart ? "Player Start" : "To: " + std::to_string(conn.targetAreaIndex);
				drawList->AddText(ImVec2(connMin.x, connMin.y - 15.0f), isPlayerStart ? MakeCol32(255, 200, 0, 255) : MakeCol32(255, 255, 0, 255), targetText.c_str());

				if (isPlayerStart) continue; // 初期位置マーカーの場合はターゲットへの線引きをスキップ

				for (const auto& target : allAreas_) {
					const AreaData& tData = (target.name == editingArea_.name) ? editingArea_ : target;
					if (tData.index == conn.targetAreaIndex) {
						ImVec2 triggerCenter(connMin.x + conn.trigger.size.x * scale * 0.5f, connMin.y + conn.trigger.size.y * scale * 0.5f);
						bool foundMutualTarget = false;
						ImVec2 targetCenter;
						int matchedCount = 0;
						for (const auto& tConn : tData.connections) {
							if (tConn.targetAreaIndex == drawData.index) {
								if (matchedCount == currentIdx) {
									targetCenter = ImVec2(
										cx + (tData.editorPos.x + tConn.trigger.position.x + tConn.trigger.size.x * 0.5f) * scale,
										cy - (tData.editorPos.y + tConn.trigger.position.y + tConn.trigger.size.y * 0.5f) * scale
									);
									foundMutualTarget = true;
									break;
								}
								matchedCount++;
							}
						}
						if (!foundMutualTarget) {
							targetCenter = ImVec2(
								cx + (tData.editorPos.x + tData.width * 0.5f) * scale,
								cy - (tData.editorPos.y + tData.height * 0.5f) * scale
							);
						}
						drawList->AddLine(triggerCenter, targetCenter, MakeCol32(255, 255, 0, 150), 2.0f);
						break;
					}
				}
			}

			// 敵配置の描画
			if (isEditing) {
				for (int ei = 0; ei < static_cast<int>(drawData.enemies.size()); ++ei) {
					const auto& ep = drawData.enemies[ei];
					float ecx2 = cx + (drawData.editorPos.x + ep.position.x) * scale;
					float ecy2 = cy - (drawData.editorPos.y + ep.position.y) * scale;
					float ms = enemyMarkerSize * scale;

					// ダイヤモンド型マーカー
					ImVec2 diamond[4] = {
						ImVec2(ecx2, ecy2 - ms),
						ImVec2(ecx2 + ms, ecy2),
						ImVec2(ecx2, ecy2 + ms),
						ImVec2(ecx2 - ms, ecy2),
					};
					ImU32 enemyColor = (draggingEnemyIndex_ == ei) ? MakeCol32(255, 100, 100, 200) : MakeCol32(255, 50, 50, 150);
					drawList->AddConvexPolyFilled(diamond, 4, enemyColor);
					drawList->AddPolyline(diamond, 4, MakeCol32(255, 200, 200, 255), ImDrawFlags_Closed, 1.5f);

					// 向き矢印
					float arrowDir = ep.facingRight ? 1.0f : -1.0f;
					ImVec2 arrowStart(ecx2, ecy2);
					ImVec2 arrowEnd(ecx2 + arrowDir * ms * 1.5f, ecy2);
					drawList->AddLine(arrowStart, arrowEnd, MakeCol32(255, 255, 100, 255), 2.0f);
					// 矢じり
					ImVec2 arrowHead1(arrowEnd.x - arrowDir * ms * 0.4f, arrowEnd.y - ms * 0.3f);
					ImVec2 arrowHead2(arrowEnd.x - arrowDir * ms * 0.4f, arrowEnd.y + ms * 0.3f);
					drawList->AddTriangleFilled(arrowEnd, arrowHead1, arrowHead2, MakeCol32(255, 255, 100, 255));

					// 敵名ラベル
					drawList->AddText(ImVec2(ecx2 - ms, ecy2 - ms - 15.0f), MakeCol32(255, 180, 180, 255), ep.enemyName.c_str());

					// 向きテキスト
					const char* dirText = ep.facingRight ? "R" : "L";
					drawList->AddText(ImVec2(ecx2 + ms + 2.0f, ecy2 - 6.0f), MakeCol32(255, 255, 100, 255), dirText);
				}

				// 当たり判定の描画
				for (int g = 0; g < static_cast<int>(drawData.collisionGroups.size()); ++g) {
					const auto& cg = drawData.collisionGroups[g];
					
					// ポイントを結ぶ線を描画 (ポリゴン)
					if (cg.points.size() >= 2) {
						for (size_t i = 0; i < cg.points.size(); ++i) {
							size_t nextIdx = (i + 1) % cg.points.size();
							const auto& p1 = cg.points[i];
							const auto& p2 = cg.points[nextIdx];
							// ポイントが2つだけの時は戻りの重複線を描画しない
							if (cg.points.size() == 2 && i == 1) continue;

							float p1cx = cx + (drawData.editorPos.x + p1.position.x) * scale;
							float p1cy = cy - (drawData.editorPos.y + p1.position.y) * scale;
							float p2cx = cx + (drawData.editorPos.x + p2.position.x) * scale;
							float p2cy = cy - (drawData.editorPos.y + p2.position.y) * scale;
							
							drawList->AddLine(ImVec2(p1cx, p1cy), ImVec2(p2cx, p2cy), MakeCol32(255, 100, 255, 180), 3.0f);
						}
					}

					for (int p = 0; p < static_cast<int>(cg.points.size()); ++p) {
						const auto& pt = cg.points[p];
						float pcx = cx + (drawData.editorPos.x + pt.position.x) * scale;
						float pcy = cy - (drawData.editorPos.y + pt.position.y) * scale;
						float radius = pt.radius * scale;
						ImU32 col = (draggingCollisionGroupIndex_ == g && draggingCollisionPointIndex_ == p) ? MakeCol32(255, 100, 255, 150) : MakeCol32(200, 50, 200, 100);
						drawList->AddCircleFilled(ImVec2(pcx, pcy), radius, col);
						drawList->AddCircle(ImVec2(pcx, pcy), radius, MakeCol32(255, 100, 255, 255), 0, 1.5f);
						drawList->AddText(ImVec2(pcx - radius, pcy - radius - 15.0f), MakeCol32(255, 150, 255, 255), ("Point: " + cg.name).c_str());
					}
				}
			}
		}

		// 左ペイン: Asset Browser
		ImGui::SetNextWindowPos(ImVec2(0, 18), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(300, 700), ImGuiCond_Always);
		ImGui::Begin("Area Asset Browser", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::TextDisabled("JSON FILES");
		ImGui::Separator();
		ImGui::BeginChild("AreaFileList", ImVec2(0, 0), false);

		namespace fs2 = std::filesystem;
		if (fs2::exists("Assets/Data/Terrain")) {
			for (const auto& entry : fs2::directory_iterator("Assets/Data/Terrain")) {
				std::string fName = entry.path().filename().string();
				if (entry.path().extension() == ".json" && fName.find("area") == 0) {
					bool isSelected = false;
					if (ImGui::Selectable(fName.c_str(), isSelected)) {
						if (trySaveArea(editingArea_, true)) {
							LoadArea(editingArea_, fName);
						}
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::End();

		// 右ペイン: Inspector
		ImGui::SetNextWindowPos(ImVec2(1280.0f - 350.0f, 18.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(350, 700), ImGuiCond_Always);
		ImGui::Begin("Inspector", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		if (ImGui::CollapsingHeader("Area Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::InputInt("Area Name / Index", &editingArea_.index)) {
				editingArea_.name = editingArea_.index;
			}
			ImGui::Text("Width: %d", editingArea_.width);
			ImGui::Text("Height: %d", editingArea_.height);
			char musicBuf[256];
			strncpy_s(musicBuf, editingArea_.backgroundMusic.c_str(), sizeof(musicBuf));
			if (ImGui::InputText("Background Music", musicBuf, sizeof(musicBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
				editingArea_.backgroundMusic = musicBuf;
			} else if (ImGui::IsItemDeactivatedAfterEdit()) {
				editingArea_.backgroundMusic = musicBuf;
			}
		}

		if (ImGui::CollapsingHeader("Area Connections", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Add Connection", ImVec2(-1, 0))) {
				editingArea_.connections.push_back({});
			}
			ImGui::Separator();

			for (size_t i = 0; i < editingArea_.connections.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				bool isPlayerStart = (editingArea_.index == 0 && editingArea_.connections[i].targetAreaIndex == 0);
				std::string label = isPlayerStart ? "Player Start (Connection " + std::to_string(i) + ")###ConnNode" : "Connection " + std::to_string(i) + " (Target Index: " + std::to_string(editingArea_.connections[i].targetAreaIndex) + ")###ConnNode";
				if (ImGui::TreeNode(label.c_str())) {
					if (isPlayerStart) {
						ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[Player Start Point]");
					}
					ImGui::InputInt("Target Area Index", &editingArea_.connections[i].targetAreaIndex);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						trySaveArea(editingArea_, true);
					}

					ImGui::Text("Trigger Collision (Rect)");
					ImGui::DragFloat2("Position", &editingArea_.connections[i].trigger.position.x, 1.0f);
					ImGui::DragFloat2("Size", &editingArea_.connections[i].trigger.size.x, 1.0f);

					if (ImGui::Button("Remove Connection")) {
						editingArea_.connections.erase(editingArea_.connections.begin() + i);
						ImGui::TreePop();
						ImGui::PopID();
						break;
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}

		if (ImGui::CollapsingHeader("Enemy Placements", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Add Enemy", ImVec2(-1, 0))) {
				EnemyPlacement newEnemy;
				newEnemy.position = { static_cast<float>(editingArea_.width) / 2.0f, static_cast<float>(editingArea_.height) / 2.0f };
				editingArea_.enemies.push_back(newEnemy);
			}
			ImGui::Separator();

			for (size_t i = 0; i < editingArea_.enemies.size(); ++i) {
				ImGui::PushID(static_cast<int>(i) + 10000);
				std::string dirStr = editingArea_.enemies[i].facingRight ? "Right" : "Left";
				std::string label = "Enemy " + std::to_string(i) + " (" + editingArea_.enemies[i].enemyName + ", " + dirStr + ")###EnemyNode";
				if (ImGui::TreeNode(label.c_str())) {
					// 敵名選択（コンボボックス）
					if (!enemyFiles_.empty()) {
						int currentItem = -1;
						for (int k = 0; k < static_cast<int>(enemyFiles_.size()); ++k) {
							if (enemyFiles_[k] == editingArea_.enemies[i].enemyName) {
								currentItem = k;
								break;
							}
						}
						if (ImGui::BeginCombo("Enemy Type", editingArea_.enemies[i].enemyName.c_str())) {
							for (int k = 0; k < static_cast<int>(enemyFiles_.size()); ++k) {
								bool isSelected = (currentItem == k);
								if (ImGui::Selectable(enemyFiles_[k].c_str(), isSelected)) {
									editingArea_.enemies[i].enemyName = enemyFiles_[k];
								}
								if (isSelected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
					} else {
						char nameBuf[256];
						strncpy_s(nameBuf, editingArea_.enemies[i].enemyName.c_str(), sizeof(nameBuf));
						if (ImGui::InputText("Enemy Name", nameBuf, sizeof(nameBuf))) {
							editingArea_.enemies[i].enemyName = nameBuf;
						}
					}

					ImGui::DragFloat2("Position", &editingArea_.enemies[i].position.x, 1.0f);

					// 向き設定
					bool facingRight = editingArea_.enemies[i].facingRight;
					if (ImGui::Checkbox("Facing Right", &facingRight)) {
						editingArea_.enemies[i].facingRight = facingRight;
					}
					ImGui::SameLine();
					ImGui::TextDisabled(facingRight ? "(->)" : "(<-)");

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
					if (ImGui::Button("Remove Enemy")) {
						editingArea_.enemies.erase(editingArea_.enemies.begin() + i);
						ImGui::PopStyleColor();
						ImGui::TreePop();
						ImGui::PopID();
						break;
					}
					ImGui::PopStyleColor();
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}

		if (ImGui::CollapsingHeader("Collision Groups", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Add Collision Group", ImVec2(-1, 0))) {
				CollisionGroup newGroup;
				newGroup.name = "Group_" + std::to_string(editingArea_.collisionGroups.size());
				editingArea_.collisionGroups.push_back(newGroup);
			}
			ImGui::Separator();

			for (size_t i = 0; i < editingArea_.collisionGroups.size(); ++i) {
				ImGui::PushID(static_cast<int>(i) + 20000);
				std::string label = "Group " + std::to_string(i) + " (" + editingArea_.collisionGroups[i].name + ")###CGNode";
				if (ImGui::TreeNode(label.c_str())) {
					char nameBuf[256];
					strncpy_s(nameBuf, editingArea_.collisionGroups[i].name.c_str(), sizeof(nameBuf));
					if (ImGui::InputText("Group Name", nameBuf, sizeof(nameBuf))) {
						editingArea_.collisionGroups[i].name = nameBuf;
					}

					if (ImGui::Button("Add Point")) {
						CollisionPoint p;
						p.position = { static_cast<float>(editingArea_.width) / 2.0f, static_cast<float>(editingArea_.height) / 2.0f };
						editingArea_.collisionGroups[i].points.push_back(p);
					}
					
					ImGui::Separator();

					for (size_t p = 0; p < editingArea_.collisionGroups[i].points.size(); ++p) {
						ImGui::PushID(static_cast<int>(p) + 30000);
						ImGui::Text("Point %llu", p);
						ImGui::DragFloat2("Position", &editingArea_.collisionGroups[i].points[p].position.x, 1.0f);
						ImGui::DragFloat("Radius", &editingArea_.collisionGroups[i].points[p].radius, 1.0f, 1.0f, 1000.0f);
						if (ImGui::Button("Remove Point")) {
							editingArea_.collisionGroups[i].points.erase(editingArea_.collisionGroups[i].points.begin() + p);
							ImGui::PopID();
							break;
						}
						ImGui::PopID();
						ImGui::Separator();
					}

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
					if (ImGui::Button("Remove Group")) {
						editingArea_.collisionGroups.erase(editingArea_.collisionGroups.begin() + i);
						ImGui::PopStyleColor();
						ImGui::TreePop();
						ImGui::PopID();
						break;
					}
					ImGui::PopStyleColor();

					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}

		ImGui::Separator();
		if (ImGui::Button("SAVE AREA", ImVec2(-1, 40))) {
			trySaveArea(editingArea_, true);
		}

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
		if (ImGui::Button("DELETE AREA", ImVec2(-1, 30))) {
			ImGui::OpenPopup("Delete Confirmation");
		}
		ImGui::PopStyleColor();

		if (ImGui::BeginPopupModal("Delete Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Are you sure you want to delete area %d?\nThis operation cannot be undone!", editingArea_.index);
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				DeleteArea(editingArea_.index);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (openConvexError) {
			ImGui::OpenPopup("Save Error");
		}

		if (ImGui::BeginPopupModal("Save Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Cannot save!\nThe collision shape must be a convex polygon.\n(Check for self-intersections or concave angles.)");
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::EndPopup();
		}

		ImGui::Separator();
		if (ImGui::Button("RESET")) {
			editingArea_.Reset();
		}
		ImGui::End();
	}
#endif
}
