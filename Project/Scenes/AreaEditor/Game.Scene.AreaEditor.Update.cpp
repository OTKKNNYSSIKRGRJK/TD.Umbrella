module Game.Editor.AreaEditor;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import <string>;
import <map>;
import <cmath>;
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
		static float saveNotificationTimer = 0.0f;
		auto centerCameraOnArea = [&]() {
			ImVec2 displaySize = ImGui::GetIO().DisplaySize;
			cameraPos_.x = displaySize.x * 0.5f - (editingArea_.editorPos.x + editingArea_.width * 0.5f) * zoom_;
			cameraPos_.y = displaySize.y * 0.5f + (editingArea_.editorPos.y + editingArea_.height * 0.5f) * zoom_;
		};
		auto trySaveArea = [&](const AreaData& areaToSave, bool sync) {
			for (const auto& cg : areaToSave.collisionGroups) {
				if (!IsConvex(cg.points)) {
					openConvexError = true;
					return false;
				}
			}
			SaveArea(areaToSave, sync);
			saveNotificationTimer = 2.0f;
			return true;
		};

		// 右ドラッグでカメラ移動
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			cameraPos_.x += delta.x;
			cameraPos_.y += delta.y;
		}

		// マウスホイールで拡縮（マウスカーソル位置を中心にズーム）
		if (!ImGui::GetIO().WantCaptureMouse) {
			float wheel = ImGui::GetIO().MouseWheel;
			if (wheel != 0.0f) {
				float oldZoom = zoom_;
				constexpr float zoomSpeed = 0.1f;
				constexpr float zoomMin = 0.05f;
				constexpr float zoomMax = 5.0f;
				zoom_ *= (1.0f + wheel * zoomSpeed);
				zoom_ = (std::max)(zoomMin, (std::min)(zoomMax, zoom_));

				// マウスカーソル位置を中心にズーム（カーソル下のワールド座標が変わらないように補正）
				ImVec2 mPos = ImGui::GetMousePos();
				cameraPos_.x = mPos.x - (mPos.x - cameraPos_.x) * (zoom_ / oldZoom);
				cameraPos_.y = mPos.y - (mPos.y - cameraPos_.y) * (zoom_ / oldZoom);
			}
		}

		// 敵JSONファイルリストをディレクトリ変更時のみ再スキャン
		static std::filesystem::file_time_type lastScanDirTime{};
		try {
			auto currentDirTime = std::filesystem::last_write_time("Assets/Data/Enemy/");
			if (lastScanDirTime != currentDirTime) {
				lastScanDirTime = currentDirTime;
				enemyFiles_.clear();
				for (const auto& entry : std::filesystem::directory_iterator("Assets/Data/Enemy/")) {
					try {
						if (!entry.is_regular_file()) continue;
						std::string fName = entry.path().filename().string();
						if (entry.path().extension() == ".json") {
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

		// エリアJSONファイルのライブリロード
		static std::map<std::string, std::filesystem::file_time_type> lastAreaFileTimes;
		try {
			if (std::filesystem::exists("Assets/Data/Terrain/")) {
				for (const auto& entry : std::filesystem::directory_iterator("Assets/Data/Terrain/")) {
					if (!entry.is_regular_file()) continue;
					std::string fName = entry.path().filename().string();
					if (entry.path().extension() == ".json" && fName.find("area") == 0) {
						auto fTime = std::filesystem::last_write_time(entry);
						if (lastAreaFileTimes.find(fName) == lastAreaFileTimes.end()) {
							lastAreaFileTimes[fName] = fTime;
						} else if (lastAreaFileTimes[fName] != fTime) {
							lastAreaFileTimes[fName] = fTime;
							// ファイルが更新された場合、再読み込みを行う
							AreaData reloadedArea;
							LoadArea(reloadedArea, fName);
							bool found = false;
							for (auto& a : allAreas_) {
								if (a.name == reloadedArea.name) {
									// originalJson などを最新のものに更新しつつ、既存のデータ構造を維持
									a = reloadedArea;
									found = true;
									// 現在編集中のエリアであれば、それも更新
									if (editingArea_.name == reloadedArea.name) {
										editingArea_ = reloadedArea;
									}
									break;
								}
							}
							if (!found) {
								allAreas_.push_back(reloadedArea);
							}
						}
					}
				}
			}
		} catch (...) {}

		float scale = zoom_;
		float cx = cameraPos_.x;
		float cy = cameraPos_.y;
		ImVec2 mousePos = ImGui::GetMousePos();

		// エリア外に出ないように座標補正
		for (auto& conn : editingArea_.connections) {
			conn.position.x = (std::max)(0.0f, (std::min)(conn.position.x, static_cast<float>(editingArea_.width)));
			conn.position.y = (std::max)(0.0f, (std::min)(conn.position.y, static_cast<float>(editingArea_.height)));
		}

		// 敵配置もエリア内に制限
		for (auto& ep : editingArea_.enemies) {
			ep.position.x = (std::max)(0.0f, (std::min)(ep.position.x, static_cast<float>(editingArea_.width)));
			ep.position.y = (std::max)(0.0f, (std::min)(ep.position.y, static_cast<float>(editingArea_.height)));
		}

		// ゴールもエリア内に制限
		if (editingArea_.hasGoal) {
			editingArea_.goalPosition.x = (std::max)(0.0f, (std::min)(editingArea_.goalPosition.x, static_cast<float>(editingArea_.width)));
			editingArea_.goalPosition.y = (std::max)(0.0f, (std::min)(editingArea_.goalPosition.y, static_cast<float>(editingArea_.height)));
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
			draggingGoal_ = -1;

			constexpr float connectionMarkerSize = 16.0f;
			for (int i = 0; i < static_cast<int>(editingArea_.connections.size()); ++i) {
				const auto& conn = editingArea_.connections[i];
				float ccx = cx + (editingArea_.editorPos.x + conn.position.x) * scale;
				float ccy = cy - (editingArea_.editorPos.y + conn.position.y) * scale;
				float cxmin = ccx - connectionMarkerSize * scale;
				float cxmax = ccx + connectionMarkerSize * scale;
				float cymin = ccy - connectionMarkerSize * scale;
				float cymax = ccy + connectionMarkerSize * scale;

				if (mousePos.x >= cxmin && mousePos.x <= cxmax && mousePos.y >= cymin && mousePos.y <= cymax) {
					draggingConnectionIndex_ = i;
					dragOffset_ = { mousePos.x - ccx, mousePos.y - ccy };
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

			// ゴールのドラッグ判定
			if (draggingConnectionIndex_ == -1 && draggingEnemyIndex_ == -1 && editingArea_.hasGoal) {
				constexpr float goalMarkerSize = 20.0f;
				float gcx = cx + (editingArea_.editorPos.x + editingArea_.goalPosition.x) * scale;
				float gcy = cy - (editingArea_.editorPos.y + editingArea_.goalPosition.y) * scale;
				float gxmin = gcx - goalMarkerSize * scale;
				float gxmax = gcx + goalMarkerSize * scale;
				float gymin = gcy - goalMarkerSize * scale;
				float gymax = gcy + goalMarkerSize * scale;

				if (mousePos.x >= gxmin && mousePos.x <= gxmax && mousePos.y >= gymin && mousePos.y <= gymax) {
					draggingGoal_ = 0;
					dragOffset_ = { mousePos.x - gcx, mousePos.y - gcy };
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
								centerCameraOnArea();
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
				float newCcx = mousePos.x - dragOffset_.x;
				float newCcy = mousePos.y - dragOffset_.y;
				conn.position.x = (newCcx - cx) / scale - editingArea_.editorPos.x;
				conn.position.y = (cy - newCcy) / scale - editingArea_.editorPos.y;

				conn.position.x = (std::max)(0.0f, (std::min)(conn.position.x, static_cast<float>(editingArea_.width)));
				conn.position.y = (std::max)(0.0f, (std::min)(conn.position.y, static_cast<float>(editingArea_.height)));
			} else if (draggingEnemyIndex_ != -1) {
				auto& ep = editingArea_.enemies[draggingEnemyIndex_];
				float newEcx = mousePos.x - dragOffset_.x;
				float newEcy = mousePos.y - dragOffset_.y;
				ep.position.x = (newEcx - cx) / scale - editingArea_.editorPos.x;
				ep.position.y = (cy - newEcy) / scale - editingArea_.editorPos.y;

				ep.position.x = (std::max)(0.0f, (std::min)(ep.position.x, static_cast<float>(editingArea_.width)));
				ep.position.y = (std::max)(0.0f, (std::min)(ep.position.y, static_cast<float>(editingArea_.height)));
			} else if (draggingGoal_ != -1) {
				float newGcx = mousePos.x - dragOffset_.x;
				float newGcy = mousePos.y - dragOffset_.y;
				editingArea_.goalPosition.x = (newGcx - cx) / scale - editingArea_.editorPos.x;
				editingArea_.goalPosition.y = (cy - newGcy) / scale - editingArea_.editorPos.y;

				editingArea_.goalPosition.x = (std::max)(0.0f, (std::min)(editingArea_.goalPosition.x, static_cast<float>(editingArea_.width)));
				editingArea_.goalPosition.y = (std::max)(0.0f, (std::min)(editingArea_.goalPosition.y, static_cast<float>(editingArea_.height)));
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
			if (draggingConnectionIndex_ != -1 || draggingAreaIndex_ != -1 || draggingEnemyIndex_ != -1 || draggingCollisionGroupIndex_ != -1 || draggingGoal_ != -1) {
				trySaveArea(editingArea_, true);
			}
			draggingConnectionIndex_ = -1;
			draggingAreaIndex_ = -1;
			draggingEnemyIndex_ = -1;
			draggingCollisionGroupIndex_ = -1;
			draggingCollisionPointIndex_ = -1;
			draggingGoal_ = -1;
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
									//エリア外に点があった場合描画上はクランプ
									float sx = cx + (drawData.editorPos.x + px) * scale;
									float sy = cy - (drawData.editorPos.y + drawData.height - py) * scale;
									sx = (std::max)(areaMin.x, (std::min)(sx, areaMax.x));
									sy = (std::max)(areaMin.y, (std::min)(sy, areaMax.y));
									points.push_back(ImVec2(sx, sy));
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

			constexpr float connectionMarkerSize = 16.0f;
			std::map<int, int> targetCount;
			for (const auto& conn : drawData.connections) {
				int currentIdx = targetCount[conn.targetAreaIndex]++;
				float ccx = cx + (drawData.editorPos.x + conn.position.x) * scale;
				float ccy = cy - (drawData.editorPos.y + conn.position.y) * scale;
				float ms = connectionMarkerSize * scale;

				bool isPlayerStart = (drawData.index == 0 && conn.targetAreaIndex == 0);
				ImU32 fillColor = isPlayerStart ? MakeCol32(255, 120, 0, isEditing ? 100 : 50) : MakeCol32(0, 150, 255, isEditing ? 100 : 50);
				ImU32 outlineColor = isPlayerStart ? MakeCol32(255, 200, 0, 255) : MakeCol32(0, 255, 255, 255);

				// Draw Diamond shaped
				ImVec2 diamond[4] = {
					ImVec2(ccx, ccy - ms),
					ImVec2(ccx + ms, ccy),
					ImVec2(ccx, ccy + ms),
					ImVec2(ccx - ms, ccy),
				};
				drawList->AddConvexPolyFilled(diamond, 4, fillColor);
				drawList->AddPolyline(diamond, 4, outlineColor, ImDrawFlags_Closed, 1.5f);

				std::string targetText = isPlayerStart ? "Player Start" : "To: " + std::to_string(conn.targetAreaIndex);
				drawList->AddText(ImVec2(ccx - ms, ccy - ms - 15.0f), isPlayerStart ? MakeCol32(255, 200, 0, 255) : MakeCol32(255, 255, 0, 255), targetText.c_str());

				if (isPlayerStart) continue; // 初期位置マーカーの場合はターゲットへの線引きをスキップ

				for (const auto& target : allAreas_) {
					const AreaData& tData = (target.name == editingArea_.name) ? editingArea_ : target;
					if (tData.index == conn.targetAreaIndex) {
						ImVec2 triggerCenter(ccx, ccy);
						bool foundMutualTarget = false;
						ImVec2 targetCenter;
						int matchedCount = 0;
						for (const auto& tConn : tData.connections) {
							if (tConn.targetAreaIndex == drawData.index) {
								if (matchedCount == currentIdx) {
									float tccx = cx + (tData.editorPos.x + tConn.position.x) * scale;
									float tccy = cy - (tData.editorPos.y + tConn.position.y) * scale;
									targetCenter = ImVec2(tccx, tccy);
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

					// 敵名ラベル + サイズ
					const char* szLabels[] = { "S", "M", "L" };
					int szc = ep.sizeCategory;
					std::string enemyLabel = ep.enemyName + " [" + ((szc >= 0 && szc < 3) ? szLabels[szc] : "?") + "]";
					drawList->AddText(ImVec2(ecx2 - ms, ecy2 - ms - 15.0f), MakeCol32(255, 180, 180, 255), enemyLabel.c_str());

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

				// ゴールの描画
				if (drawData.hasGoal) {
					constexpr float goalMarkerSize = 20.0f;
					float gcx2 = cx + (drawData.editorPos.x + drawData.goalPosition.x) * scale;
					float gcy2 = cy - (drawData.editorPos.y + drawData.goalPosition.y) * scale;
					float gms = goalMarkerSize * scale;

					// 金色の円マーカー
					ImU32 goalFill = isEditing ? MakeCol32(255, 215, 0, 180) : MakeCol32(255, 215, 0, 80);
					ImU32 goalOutline = MakeCol32(255, 255, 100, 255);
					drawList->AddCircleFilled(ImVec2(gcx2, gcy2), gms, goalFill);
					drawList->AddCircle(ImVec2(gcx2, gcy2), gms, goalOutline, 0, 2.5f);
					drawList->AddText(ImVec2(gcx2 - gms, gcy2 - gms - 15.0f), MakeCol32(255, 255, 0, 255), "GOAL");
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
		if (fs2::exists("Assets/Data/Terrain/")) {
			for (const auto& entry : fs2::directory_iterator("Assets/Data/Terrain/")) {
				std::string fName = entry.path().filename().string();
				if (entry.path().extension() == ".json" && fName.find("area") == 0) {
					bool isSelected = false;
					if (ImGui::Selectable(fName.c_str(), isSelected)) {
						if (trySaveArea(editingArea_, true)) {
							LoadArea(editingArea_, fName);
							centerCameraOnArea();
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
			int inputIndex = editingArea_.index;
			if (ImGui::InputInt("Area Name / Index", &inputIndex)) {
				bool exists = false;
				for (const auto& a : allAreas_) {
					if (a.index == inputIndex) {
						exists = true;
						break;
					}
				}

				if (exists && inputIndex != editingArea_.index) {
					if (trySaveArea(editingArea_, true)) {
						LoadArea(editingArea_, "area" + std::to_string(inputIndex) + ".json");
						centerCameraOnArea();
					}
				} else {
					editingArea_.index = inputIndex;
					editingArea_.name = inputIndex;
				}
			}
			ImGui::Text("Width: %d", editingArea_.width);
			ImGui::Text("Height: %d", editingArea_.height);
			char musicBuf[256];
			strncpy_s(musicBuf, editingArea_.backgroundMusic.c_str(), sizeof(musicBuf));
			if (ImGui::InputText("Background Music", musicBuf, sizeof(musicBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
				editingArea_.backgroundMusic = musicBuf;
				trySaveArea(editingArea_, true);
			} else if (ImGui::IsItemDeactivatedAfterEdit()) {
				editingArea_.backgroundMusic = musicBuf;
				trySaveArea(editingArea_, true);
			}
		}

		if (ImGui::CollapsingHeader("Area Connections", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Add Connection", ImVec2(-1, 0))) {
				editingArea_.connections.push_back({});
				trySaveArea(editingArea_, true);
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

					ImGui::Text("Portal Coordinates");
					ImGui::DragFloat2("Position", &editingArea_.connections[i].position.x, 1.0f);
					if (ImGui::IsItemDeactivatedAfterEdit()) trySaveArea(editingArea_, true);

					if (ImGui::Button("Remove Connection")) {
						editingArea_.connections.erase(editingArea_.connections.begin() + i);
						trySaveArea(editingArea_, true);
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
				trySaveArea(editingArea_, true);
			}
			ImGui::Separator();

			for (size_t i = 0; i < editingArea_.enemies.size(); ++i) {
				ImGui::PushID(static_cast<int>(i) + 10000);
				const char* sizeLabels[] = { "S", "M", "L" };
				int sc = editingArea_.enemies[i].sizeCategory;
				std::string sizeStr = (sc >= 0 && sc < 3) ? sizeLabels[sc] : "?";
				std::string dirStr = editingArea_.enemies[i].facingRight ? "Right" : "Left";
				std::string label = "Enemy " + std::to_string(i) + " (" + editingArea_.enemies[i].enemyName + ", " + sizeStr + ", " + dirStr + ")###EnemyNode";
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
									trySaveArea(editingArea_, true);
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
						if (ImGui::IsItemDeactivatedAfterEdit()) trySaveArea(editingArea_, true);
					}

					// サイズ段階選択
					const char* sizeNames[] = { "Small", "Medium", "Large" };
					if (ImGui::Combo("Size", &editingArea_.enemies[i].sizeCategory, sizeNames, 3)) {
						trySaveArea(editingArea_, true);
					}

					ImGui::DragFloat2("Position", &editingArea_.enemies[i].position.x, 1.0f);
					if (ImGui::IsItemDeactivatedAfterEdit()) trySaveArea(editingArea_, true);

					// 向き設定
					bool facingRight = editingArea_.enemies[i].facingRight;
					if (ImGui::Checkbox("Facing Right", &facingRight)) {
						editingArea_.enemies[i].facingRight = facingRight;
						trySaveArea(editingArea_, true);
					}
					ImGui::SameLine();
					ImGui::TextDisabled(facingRight ? "(->)" : "(<-)");

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
					if (ImGui::Button("Remove Enemy")) {
						editingArea_.enemies.erase(editingArea_.enemies.begin() + i);
						trySaveArea(editingArea_, true);
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
				trySaveArea(editingArea_, true);
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
					if (ImGui::IsItemDeactivatedAfterEdit()) trySaveArea(editingArea_, true);

					if (ImGui::Button("Add Point")) {
						CollisionPoint p;
						p.position = { static_cast<float>(editingArea_.width) / 2.0f, static_cast<float>(editingArea_.height) / 2.0f };
						editingArea_.collisionGroups[i].points.push_back(p);
						trySaveArea(editingArea_, true);
					}
					
					ImGui::Separator();

					for (size_t p = 0; p < editingArea_.collisionGroups[i].points.size(); ++p) {
						ImGui::PushID(static_cast<int>(p) + 30000);
						ImGui::Text("Point %llu", p);
						ImGui::DragFloat2("Position", &editingArea_.collisionGroups[i].points[p].position.x, 1.0f);
						if (ImGui::IsItemDeactivatedAfterEdit()) trySaveArea(editingArea_, true);
						ImGui::DragFloat("Radius", &editingArea_.collisionGroups[i].points[p].radius, 1.0f, 1.0f, 1000.0f);
						if (ImGui::IsItemDeactivatedAfterEdit()) trySaveArea(editingArea_, true);
						if (ImGui::Button("Remove Point")) {
							editingArea_.collisionGroups[i].points.erase(editingArea_.collisionGroups[i].points.begin() + p);
							trySaveArea(editingArea_, true);
							ImGui::PopID();
							break;
						}
						ImGui::PopID();
						ImGui::Separator();
					}

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
					if (ImGui::Button("Remove Group")) {
						editingArea_.collisionGroups.erase(editingArea_.collisionGroups.begin() + i);
						trySaveArea(editingArea_, true);
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

		if (ImGui::CollapsingHeader("Goal", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool hasGoal = editingArea_.hasGoal;
			if (ImGui::Checkbox("Place Goal in this Area", &hasGoal)) {
				editingArea_.hasGoal = hasGoal;
				if (hasGoal) {
					editingArea_.goalPosition = { static_cast<float>(editingArea_.width) / 2.0f, static_cast<float>(editingArea_.height) / 2.0f };
				}
				trySaveArea(editingArea_, true);
			}
			if (editingArea_.hasGoal) {
				ImGui::DragFloat2("Goal Position", &editingArea_.goalPosition.x, 1.0f);
				if (ImGui::IsItemDeactivatedAfterEdit()) trySaveArea(editingArea_, true);
				ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Goal marker is shown on canvas.");
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Drag the marker to reposition)");
			}
		}

		ImGui::Separator();
		if (ImGui::Button("SAVE AREA", ImVec2(-1, 40))) {
			trySaveArea(editingArea_, true);
		}

		if (saveNotificationTimer > 0.0f) {
			saveNotificationTimer -= ImGui::GetIO().DeltaTime;
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "  Saved Successfully!");
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
		if (ImGui::Button("CREATE NEW AREA", ImVec2(-1, 40))) {
			editingArea_.Reset();

			int newIdx = 0;
			float maxX = -999999.0f;
			float assocY = 0.0f;
			for (const auto& a : allAreas_) {
				if (a.index >= newIdx) {
					newIdx = a.index + 1;
				}
				float rightEdge = a.editorPos.x + static_cast<float>(a.width);
				if (rightEdge > maxX) {
					maxX = rightEdge;
					assocY = a.editorPos.y;
				}
			}

			editingArea_.index = newIdx;
			editingArea_.name = newIdx;
			if (static_cast<int>(maxX) != -999999) {
				editingArea_.editorPos = { maxX + 100.0f, assocY };
			} else {
				editingArea_.editorPos = { 0.0f, 0.0f };
			}
			centerCameraOnArea();
		}
		ImGui::End();
	}
	void AreaEditor::DrawAreaMap(int currentAreaIndex) {
		if (allAreas_.empty()) return;

		// === マップウィンドウ配置 ===
		constexpr float mapW = 420.0f; // 横長の枠に広げる
		constexpr float mapH = 240.0f; // 高さを少し抑える
		constexpr float margin = 12.0f;
		constexpr float padding = 14.0f;

		ImVec2 displaySize = ImGui::GetIO().DisplaySize;
		float mapLeft = displaySize.x - mapW - margin;
		float mapTop  = displaySize.y - mapH - margin;

		ImDrawList* drawList = ImGui::GetForegroundDrawList();

		// === 背景パネル（羊皮紙風） ===
		drawList->AddRectFilled(
			ImVec2(mapLeft, mapTop),
			ImVec2(mapLeft + mapW, mapTop + mapH),
			MakeCol32(45, 38, 30, 220),
			8.0f
		);
		// 内側グラデーション風の枠
		drawList->AddRect(
			ImVec2(mapLeft + 2, mapTop + 2),
			ImVec2(mapLeft + mapW - 2, mapTop + mapH - 2),
			MakeCol32(120, 95, 60, 180),
			6.0f, 0, 1.0f
		);
		drawList->AddRect(
			ImVec2(mapLeft, mapTop),
			ImVec2(mapLeft + mapW, mapTop + mapH),
			MakeCol32(80, 65, 40, 255),
			8.0f, 0, 2.0f
		);

		// === ラベル ===
		drawList->AddText(ImVec2(mapLeft + 10, mapTop + 6), MakeCol32(210, 190, 140, 255), "AREA MAP");

		// === 描画可能領域 ===
		float innerLeft   = mapLeft  + padding;
		float innerTop    = mapTop   + padding + 18.0f;
		float innerWidth  = mapW     - padding * 2.0f;
		float innerHeight = mapH     - padding * 2.0f - 18.0f;

		// === つながり（ネットワーク）から網の目のように配置位置を計算 ===
		struct GridPos {
			int x, y;
			bool operator<(const GridPos& other) const {
				if (x != other.x) return x < other.x;
				return y < other.y;
			}
		};

		// 1. 各エリアの隣接リストを作成
		std::map<int, std::vector<int>> adjList;
		for (const auto& a : allAreas_) {
			for (const auto& conn : a.connections) {
				int targetIdx = conn.targetAreaIndex;
				if (a.index == 0 && targetIdx == 0) continue; // PlayerStart無視
				adjList[a.index].push_back(targetIdx);
				adjList[targetIdx].push_back(a.index); // 双方向保証
			}
		}

		// 重複削除
		for (auto& pair : adjList) {
			auto& vec = pair.second;
			std::sort(vec.begin(), vec.end());
			vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
		}

		// 探索開始順（Area 0 があれば最優先）
		std::vector<int> startNodes;
		auto hasNode = [&](int idx) {
			for (const auto& a : allAreas_) if (a.index == idx) return true;
			return false;
		};
		if (hasNode(0)) startNodes.push_back(0);
		for (const auto& a : allAreas_) {
			if (std::find(startNodes.begin(), startNodes.end(), a.index) == startNodes.end()) {
				startNodes.push_back(a.index);
			}
		}

		// BFSでグリッドレイアウトを決定する
		std::map<int, GridPos> gridLayout;
		std::map<GridPos, int> gridOccupancy;

		for (int startIdx : startNodes) {
			if (gridLayout.count(startIdx) > 0) continue;

			std::vector<int> queue = { startIdx };
			
			// 独立したグラフ塊の場合、重複しないY座標の開始位置を探す
			int startX = 0, startY = 0;
			while (gridOccupancy.count({startX, startY}) > 0) {
				startY++;
			}
			gridLayout[startIdx] = {startX, startY};
			gridOccupancy[{startX, startY}] = startIdx;

			size_t head = 0;
			while (head < queue.size()) {
				int curr = queue[head++];
				GridPos cPos = gridLayout[curr];

				int yOffset = 0;
				for (int neighbor : adjList[curr]) {
					if (gridLayout.count(neighbor) == 0) {
						// 進行方向（つながりの深さ）は常に横(X+1)に進める
						int nx = cPos.x + 1;
						int ny = cPos.y + yOffset;
						// 空いているY座標をジグザグに探す (0, -1, 1, -2, 2...)
						while (gridOccupancy.count({nx, ny}) > 0) {
							yOffset = (yOffset <= 0) ? -yOffset + 1 : -yOffset;
							ny = cPos.y + yOffset;
						}
						gridLayout[neighbor] = {nx, ny};
						gridOccupancy[{nx, ny}] = neighbor;
						queue.push_back(neighbor);
						// 次の枝分かれノード用のオフセット更新
						yOffset = (yOffset <= 0) ? -yOffset + 1 : -yOffset;
					}
				}
			}
		}

		// グリッド座標を画面上の論理座標に変換
		struct AreaNode {
			int index;
			float cx, cy;
		};
		std::vector<AreaNode> nodes;
		float minCX = 1e9f, minCY = 1e9f, maxCX = -1e9f, maxCY = -1e9f;
		
		constexpr float gridSpacingX = 100.0f;
		constexpr float gridSpacingY = 80.0f;

		for (const auto& a : allAreas_) {
			GridPos gp = gridLayout[a.index];
			float cx = gp.x * gridSpacingX;
			float cy = gp.y * gridSpacingY;
			nodes.push_back({ a.index, cx, cy });
			
			minCX = (std::min)(minCX, cx);
			minCY = (std::min)(minCY, cy);
			maxCX = (std::max)(maxCX, cx);
			maxCY = (std::max)(maxCY, cy);
		}

		// BBox に余白を追加
		float rangeX = maxCX - minCX;
		float rangeY = maxCY - minCY;
		if (rangeX < 1.0f) rangeX = 1.0f;
		if (rangeY < 1.0f) rangeY = 1.0f;
		float bboxPad = (std::max)(rangeX, rangeY) * 0.15f;
		minCX -= bboxPad; minCY -= bboxPad;
		maxCX += bboxPad; maxCY += bboxPad;
		rangeX = maxCX - minCX;
		rangeY = maxCY - minCY;

		float scaleX = innerWidth  / rangeX;
		float scaleY = innerHeight / rangeY;
		float scale  = (std::min)(scaleX, scaleY);

		float scaledW = rangeX * scale;
		float scaledH = rangeY * scale;
		float offsetX = innerLeft + (innerWidth  - scaledW) * 0.5f;
		float offsetY = innerTop  + (innerHeight - scaledH) * 0.5f;

		// クリップ
		drawList->PushClipRect(ImVec2(mapLeft, mapTop), ImVec2(mapLeft + mapW, mapTop + mapH), true);

		// エディタ座標→画面座標ラムダ (Y軸反転: editorPosはY上)
		auto ToScreen = [&](float ex, float ey) -> ImVec2 {
			return ImVec2(
				offsetX + (ex - minCX) * scale,
				offsetY + (maxCY - ey) * scale   // Y反転
			);
		};

		// 各ノードの画面座標を求めておく
		std::map<int, ImVec2> nodeScreenPos;
		for (const auto& n : nodes) {
			nodeScreenPos[n.index] = ToScreen(n.cx, n.cy);
		}

		// === 接続線の描画（重複排除） ===
		std::vector<std::pair<int,int>> drawnEdges;
		auto edgeDrawn = [&](int a, int b) -> bool {
			for (const auto& e : drawnEdges) {
				if ((e.first == a && e.second == b) || (e.first == b && e.second == a))
					return true;
			}
			return false;
		};

		for (const auto& a : allAreas_) {
			auto itFrom = nodeScreenPos.find(a.index);
			if (itFrom == nodeScreenPos.end()) continue;

			for (const auto& conn : a.connections) {
				int targetIdx = conn.targetAreaIndex;
				// PlayerStart (index==0, target==0) はスキップ
				if (a.index == 0 && targetIdx == 0) continue;

				auto itTo = nodeScreenPos.find(targetIdx);
				if (itTo == nodeScreenPos.end()) continue;

				if (edgeDrawn(a.index, targetIdx)) continue;
				drawnEdges.push_back({ a.index, targetIdx });

				ImVec2 from = itFrom->second;
				ImVec2 to   = itTo->second;

				// 太い接続線（道）
				drawList->AddLine(from, to, MakeCol32(90, 75, 50, 200), 4.0f);
				// 明るい中心線
				drawList->AddLine(from, to, MakeCol32(180, 155, 100, 140), 2.0f);
			}
		}

		// === 各エリアノードの描画 ===
		constexpr float nodeRadius = 20.0f;

		// エリアごとの色テーブル（モンハン風に各エリアが異なる色）
		constexpr int numColors = 12;
		ImU32 areaColors[numColors] = {
			MakeCol32(220, 160,  80, 200),  // 0: 砂色
			MakeCol32(100, 180, 100, 200),  // 1: 森緑
			MakeCol32(160, 120,  80, 200),  // 2: 茶色
			MakeCol32(130, 160, 200, 200),  // 3: 水色
			MakeCol32(180, 130, 160, 200),  // 4: ピンク
			MakeCol32(150, 180, 120, 200),  // 5: 若草
			MakeCol32(200, 180, 100, 200),  // 6: 黄土
			MakeCol32(120, 140, 180, 200),  // 7: 青灰
			MakeCol32(180, 140, 100, 200),  // 8: 琥珀
			MakeCol32(140, 180, 170, 200),  // 9: 翡翠
			MakeCol32(200, 140, 140, 200),  // 10: 紅
			MakeCol32(160, 160, 120, 200),  // 11: カーキ
		};

		for (const auto& n : nodes) {
			ImVec2 pos = nodeScreenPos[n.index];
			bool isCurrent = (n.index == currentAreaIndex);
			int colorIdx = n.index % numColors;
			ImU32 fillColor = areaColors[colorIdx];

			// 現在エリアのグロー
			if (isCurrent) {
				drawList->AddCircleFilled(pos, nodeRadius + 8.0f, MakeCol32(255, 200, 50, 60));
				drawList->AddCircleFilled(pos, nodeRadius + 5.0f, MakeCol32(255, 220, 80, 80));
			}

			// 不規則形状風: 8角形で描画
			constexpr int numSides = 8;
			ImVec2 polyPoints[numSides];
			float baseRadii[numSides] = { 1.0f, 0.88f, 1.05f, 0.92f, 0.97f, 0.85f, 1.02f, 0.90f };
			for (int i = 0; i < numSides; ++i) {
				float angle = (static_cast<float>(i) / numSides) * 2.0f * 3.14159265f;
				float r = nodeRadius * baseRadii[i];
				polyPoints[i] = ImVec2(pos.x + r * cosf(angle), pos.y + r * sinf(angle));
			}

			drawList->AddConvexPolyFilled(polyPoints, numSides, fillColor);

			// 境界線
			ImU32 borderCol = isCurrent ? MakeCol32(255, 220, 80, 255) : MakeCol32(60, 50, 35, 255);
			float borderThk = isCurrent ? 3.0f : 2.0f;
			drawList->AddPolyline(polyPoints, numSides, borderCol, ImDrawFlags_Closed, borderThk);

			// エリア番号テキスト（大きめに中央表示）
			std::string numStr = std::to_string(n.index);
			// ImGui デフォルトフォントは約7x13px
			float textW = numStr.size() * 7.0f;
			float textH = 13.0f;
			ImU32 textCol = isCurrent ? MakeCol32(50, 30, 0, 255) : MakeCol32(230, 220, 200, 255);
			drawList->AddText(
				ImVec2(pos.x - textW * 0.5f, pos.y - textH * 0.5f),
				textCol,
				numStr.c_str()
			);
		}

		// === 現在エリア表示テキスト ===
		std::string currentLabel = "Now: Area " + std::to_string(currentAreaIndex);
		drawList->AddText(
			ImVec2(mapLeft + mapW - 110, mapTop + 6),
			MakeCol32(255, 220, 100, 255),
			currentLabel.c_str()
		);

		drawList->PopClipRect();
	}
#endif
}
