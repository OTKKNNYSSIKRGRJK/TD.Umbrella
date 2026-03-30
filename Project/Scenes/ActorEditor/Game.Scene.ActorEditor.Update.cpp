module Game.Editor.ActorEditor;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import <string>;
import <filesystem>;
import <cmath>;

import Game.MotionManager;

namespace fs = std::filesystem;

#if defined(_DEBUG)
namespace {
	constexpr ImU32 MakeCol32(int r, int g, int b, int a) {
		return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r);
	}

	float EaseLinear(float t) { return t; }
	float EaseSine(float t) { return (1.0f - std::cos(t * 3.14159265f)) * 0.5f; }
	float EaseInOut(float t) { return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f; }

	float ApplyEasing(float t, Game::Editor::EasingType type) {
		switch (type) {
		case Game::Editor::EasingType::Sine:    return EaseSine(t);
		case Game::Editor::EasingType::EaseInOut: return EaseInOut(t);
		default: return EaseLinear(t);
		}
	}

	using Vector3 = Lumina::Math::F32x3;

	// cachedNodes_ + nodeTimings から時刻 timeSec での位置を求める
	Vector3 EvalSplineAtTime(
		const std::vector<MathUtils::Spline::Node<Vector3>>& nodes,
		const std::vector<Game::Editor::NodeTiming>& timings,
		float timeSec)
	{
		if (nodes.empty()) return Vector3{};
		if (nodes.size() == 1) return nodes[0].position;

		int n = static_cast<int>(nodes.size());
		int tn = static_cast<int>(timings.size());

		// timings が足りない場合は均等割り
		auto getTime = [&](int idx) -> float {
			if (idx < tn) return timings[idx].arrivalTime;
			float maxT = (tn > 0) ? timings[tn - 1].arrivalTime : 1.0f;
			return maxT * static_cast<float>(idx) / static_cast<float>(n - 1);
		};
		auto getEasing = [&](int idx) -> Game::Editor::EasingType {
			if (idx < tn) return timings[idx].easing;
			return Game::Editor::EasingType::Linear;
		};

		// クランプ
		if (timeSec <= getTime(0)) return nodes[0].position;
		if (timeSec >= getTime(n - 1)) return nodes[n - 1].position;

		// どの区間か探す
		int seg = 0;
		for (int i = 0; i < n - 1; ++i) {
			if (timeSec >= getTime(i) && timeSec <= getTime(i + 1)) {
				seg = i;
				break;
			}
		}

		float segDur = getTime(seg + 1) - getTime(seg);
		float localT = (segDur > 0.0001f) ? (timeSec - getTime(seg)) / segDur : 0.0f;

		// 区間ごとのイージング
		localT = ApplyEasing(localT, getEasing(seg));

		// Hermite補間（エンジンの GetPointSpline と同じ式）
		return MathUtils::Spline::HermiteInterpolation(
			nodes[seg].position,
			nodes[seg].TangentOut,
			nodes[seg + 1].position,
			nodes[seg + 1].TangentIn,
			localT
		);
	}
}
#endif

namespace Game::Editor {
	void ActorEditor::Update() {
#if defined(_DEBUG)
		DrawEditorUI();
#endif
	}

#if defined(_DEBUG)
	void ActorEditor::DrawEditorUI() {
		// ===========================================================
		// 左パネル: ファイル一覧
		// ===========================================================
		ImGui::SetNextWindowPos(ImVec2(0, 18), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(250, 702), ImGuiCond_Always);
		ImGui::Begin("Actor Browser", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::TextDisabled("ACTOR JSON FILES");
		ImGui::Separator();
		ImGui::BeginChild("ActorFileList", ImVec2(0, -40), false);
		if (fs::exists("./")) {
			for (const auto& entry : fs::directory_iterator("./")) {
				if (entry.path().extension() == ".json") {
					std::string fName = entry.path().filename().string();
					if (fName.find("actor_") != 0) continue;
					bool isSelected = ("actor_" + editingActor_.name + ".json" == fName);
					if (ImGui::Selectable(fName.c_str(), isSelected)) {
						LoadActor(editingActor_, fName);
						cachedMotionName_.clear(); // 再読み込みを強制
					}
				}
			}
		}
		ImGui::EndChild();

		ImGui::Separator();
		if (ImGui::Button("+ New Actor", ImVec2(-1, 30))) {
			editingActor_.Reset();
			cachedMotionName_.clear();
			cachedNodes_.clear();
		}
		ImGui::End();

		// ===========================================================
		// 右パネル: インスペクター
		// ===========================================================
		ImGui::SetNextWindowPos(ImVec2(1280.0f - 400.0f, 18.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(400, 700), ImGuiCond_Always);
		ImGui::Begin("Actor Inspector", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		// --- Base Properties ---
		if (ImGui::CollapsingHeader("Base Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
			char nameBuf[256];
			strncpy_s(nameBuf, editingActor_.name.c_str(), sizeof(nameBuf));
			if (ImGui::InputText("Object Name", nameBuf, sizeof(nameBuf))) {
				editingActor_.name = nameBuf;
			}
			ImGui::Spacing();
			ImGui::TextDisabled("Transform");
			ImGui::DragFloat3("Position", &editingActor_.transform.posX, 0.1f, -1000.0f, 1000.0f, "%.2f");
			ImGui::DragFloat3("Rotation", &editingActor_.transform.rotX, 0.5f, -360.0f, 360.0f, "%.1f deg");
			ImGui::DragFloat3("Scale", &editingActor_.transform.scaleX, 0.01f, 0.01f, 100.0f, "%.2f");
		}

		if (ImGui::CollapsingHeader("Visual", ImGuiTreeNodeFlags_DefaultOpen)) {
			char meshBuf[256];
			strncpy_s(meshBuf, editingActor_.visual.meshPath.c_str(), sizeof(meshBuf));
			if (ImGui::InputText("Mesh Path", meshBuf, sizeof(meshBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
				editingActor_.visual.meshPath = meshBuf;
			} else if (ImGui::IsItemDeactivatedAfterEdit()) {
				editingActor_.visual.meshPath = meshBuf;
			}
			ImGui::DragInt("Material Index", &editingActor_.visual.materialIndex, 1, 0, 32);
		}

		if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
			const char* colliderTypes[] = { "None", "Box", "Sphere" };
			int colType = static_cast<int>(editingActor_.collider.type);
			if (ImGui::Combo("Collider Type", &colType, colliderTypes, 3)) {
				editingActor_.collider.type = static_cast<ColliderType>(colType);
			}
			if (editingActor_.collider.type == ColliderType::Box) {
				ImGui::DragFloat3("Box Half-Extents", &editingActor_.collider.sizeX, 0.05f, 0.01f, 100.0f, "%.2f");
			} else if (editingActor_.collider.type == ColliderType::Sphere) {
				ImGui::DragFloat("Sphere Radius", &editingActor_.collider.sizeX, 0.05f, 0.01f, 100.0f, "%.2f");
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// --- Movement Module ---
		if (ImGui::CollapsingHeader("Movement Module", ImGuiTreeNodeFlags_DefaultOpen)) {
			const char* moveTypes[] = { "None", "Linear", "Ping-Pong", "Spline" };
			int mvType = static_cast<int>(editingActor_.movement.type);
			if (ImGui::Combo("Movement Type", &mvType, moveTypes, 4)) {
				editingActor_.movement.type = static_cast<MovementType>(mvType);
			}

			// --- Linear / PingPong ---
			if (editingActor_.movement.type == MovementType::Linear ||
				editingActor_.movement.type == MovementType::PingPong) {
				ImGui::DragFloat("Speed", &editingActor_.movement.speed, 0.1f, 0.0f, 100.0f, "%.1f");
				ImGui::DragFloat3("Direction", &editingActor_.movement.dirX, 0.05f, -1.0f, 1.0f, "%.2f");
				ImGui::DragFloat("Range", &editingActor_.movement.range, 0.1f, 0.0f, 500.0f, "%.1f");
				const char* easingTypes[] = { "Linear", "Sine", "EaseInOut" };
				int eType = static_cast<int>(editingActor_.movement.easing);
				if (ImGui::Combo("Easing", &eType, easingTypes, 3)) {
					editingActor_.movement.easing = static_cast<EasingType>(eType);
				}
			}

			// --- Spline ---
			if (editingActor_.movement.type == MovementType::Spline) {
				ImGui::Spacing();
				ImGui::TextDisabled("--- Spline (MotionEditor Curve) ---");

				// モーションファイルリスト更新
				if (motionFiles_.empty()) ScanMotionFiles();
				ImGui::SameLine();
				if (ImGui::SmallButton("Refresh")) ScanMotionFiles();

				// 曲線選択コンボ
				int currentIdx = 0;
				for (int k = 0; k < static_cast<int>(motionFiles_.size()); ++k) {
					if (motionFiles_[k] == editingActor_.movement.splineMotionName) {
						currentIdx = k; break;
					}
				}
				std::string preview = editingActor_.movement.splineMotionName.empty()
					? "(none)" : editingActor_.movement.splineMotionName;
				if (ImGui::BeginCombo("Motion Curve", preview.c_str())) {
					for (int k = 0; k < static_cast<int>(motionFiles_.size()); ++k) {
						bool isSel = (currentIdx == k);
						std::string label = motionFiles_[k].empty() ? "(none)" : motionFiles_[k];
						if (ImGui::Selectable(label.c_str(), isSel)) {
							editingActor_.movement.splineMotionName = motionFiles_[k];
							cachedMotionName_.clear(); // 再読み込みをトリガー
						}
						if (isSel) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("MotionEditor for curve creation");

				ImGui::DragFloat("Total Duration (sec)", &editingActor_.movement.totalDuration, 0.1f, 0.1f, 999.0f, "%.1f");
				ImGui::Checkbox("Loop", &editingActor_.movement.loopSpline);

				// --- 曲線データの読み込み / キャッシュ ---
				if (!editingActor_.movement.splineMotionName.empty() &&
					cachedMotionName_ != editingActor_.movement.splineMotionName) {
					cachedMotionName_ = editingActor_.movement.splineMotionName;
					std::string fullPath = "Assets/Data/Motion/" + cachedMotionName_;
					MotionManager::GetInstance()->LoadActionData(fullPath, cachedNodes_);
					SyncNodeTimings();
				}

				// --- ノードごとのタイミング調整 ---
				if (!cachedNodes_.empty()) {
					ImGui::Spacing();
					ImGui::TextDisabled("--- Node Timings (%d nodes) ---",
						static_cast<int>(cachedNodes_.size()));

					auto& timings = editingActor_.movement.nodeTimings;
					const char* easingNames[] = { "Linear", "Sine", "EaseInOut" };

					for (int i = 0; i < static_cast<int>(cachedNodes_.size()); ++i) {
						ImGui::PushID(i);
						char nodeLabel[48];
						snprintf(nodeLabel, sizeof(nodeLabel), "Node %d (%.1f, %.1f)",
							i, cachedNodes_[i].position.X, cachedNodes_[i].position.Y);

						if (ImGui::TreeNodeEx(nodeLabel, (i == 0) ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
							// 位置は読み取り専用で表示
							ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),
								"Pos: (%.2f, %.2f, %.2f)",
								cachedNodes_[i].position.X,
								cachedNodes_[i].position.Y,
								cachedNodes_[i].position.Z);

							if (i < static_cast<int>(timings.size())) {
								ImGui::DragFloat("Arrival (sec)", &timings[i].arrivalTime,
									0.05f, 0.0f, 999.0f, "%.2f");
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Time to reach this node");

								if (i < static_cast<int>(cachedNodes_.size()) - 1) {
									int eIdx = static_cast<int>(timings[i].easing);
									if (ImGui::Combo("Easing##seg", &eIdx, easingNames, 3)) {
										timings[i].easing = static_cast<EasingType>(eIdx);
									}
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Easing for segment %d -> %d", i, i + 1);
								}
							}
							ImGui::TreePop();
						}
						ImGui::PopID();
					}

					// 均等割り当てボタン
					if (cachedNodes_.size() >= 2) {
						if (ImGui::Button("Auto-distribute Timing", ImVec2(-1, 22))) {
							float totalDur = editingActor_.movement.totalDuration;
							int numNodes = static_cast<int>(cachedNodes_.size());
							for (int i = 0; i < numNodes && i < static_cast<int>(timings.size()); ++i) {
								timings[i].arrivalTime = totalDur * static_cast<float>(i)
									/ static_cast<float>(numNodes - 1);
							}
						}
					}
				}
			}
		}

		// --- Interaction Module ---
		if (ImGui::CollapsingHeader("Interaction Module", ImGuiTreeNodeFlags_DefaultOpen)) {
			const char* interTypes[] = { "None", "Damage Dealer", "Solid Platform", "Trigger" };
			int iType = static_cast<int>(editingActor_.interaction.type);
			if (ImGui::Combo("Interaction Type", &iType, interTypes, 4)) {
				editingActor_.interaction.type = static_cast<InteractionType>(iType);
			}
			if (editingActor_.interaction.type == InteractionType::DamageDealer) {
				ImGui::DragFloat("Damage Value", &editingActor_.interaction.damageValue, 0.5f, 0.0f, 9999.0f, "%.1f");
				ImGui::DragFloat("Push Force", &editingActor_.interaction.pushForce, 0.1f, 0.0f, 100.0f, "%.1f");
			} else if (editingActor_.interaction.type == InteractionType::SolidPlatform) {
				ImGui::DragFloat("Push Force", &editingActor_.interaction.pushForce, 0.1f, 0.0f, 100.0f, "%.1f");
			} else if (editingActor_.interaction.type == InteractionType::Trigger) {
				char buf[256];
				strncpy_s(buf, editingActor_.interaction.activationTriggerID.c_str(), sizeof(buf));
				if (ImGui::InputText("Trigger ID", buf, sizeof(buf)))
					editingActor_.interaction.activationTriggerID = buf;
			}
		}

		// --- Lifecycle & Timing ---
		if (ImGui::CollapsingHeader("Lifecycle & Timing", ImGuiTreeNodeFlags_DefaultOpen)) {
			const char* spawnTypes[] = { "Time", "Distance", "Event" };
			int sType = static_cast<int>(editingActor_.lifecycle.spawnTrigger);
			if (ImGui::Combo("Spawn Trigger", &sType, spawnTypes, 3)) {
				editingActor_.lifecycle.spawnTrigger = static_cast<SpawnTriggerType>(sType);
			}
			if (editingActor_.lifecycle.spawnTrigger == SpawnTriggerType::Time) {
				ImGui::DragFloat("Spawn Time (sec)", &editingActor_.lifecycle.spawnValue, 0.1f, 0.0f, 9999.0f, "%.1f");
			} else if (editingActor_.lifecycle.spawnTrigger == SpawnTriggerType::Distance) {
				ImGui::DragFloat("Spawn Distance", &editingActor_.lifecycle.spawnValue, 0.5f, 0.0f, 99999.0f, "%.1f");
			} else if (editingActor_.lifecycle.spawnTrigger == SpawnTriggerType::Event) {
				char buf[256];
				strncpy_s(buf, editingActor_.lifecycle.spawnEventID.c_str(), sizeof(buf));
				if (ImGui::InputText("Spawn Event ID", buf, sizeof(buf)))
					editingActor_.lifecycle.spawnEventID = buf;
			}
			ImGui::Spacing();
			ImGui::DragFloat("Lifetime (sec)", &editingActor_.lifecycle.lifetime, 0.1f, 0.0f, 999.0f, "%.1f");
			ImGui::Checkbox("Auto-destroy off-screen", &editingActor_.lifecycle.autoDestroyOffscreen);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		if (ImGui::Button("SAVE ACTOR", ImVec2(-1, 40))) {
			SaveActor(editingActor_);
		}
		ImGui::End();

		DrawPreviewCanvas();
	}

	void ActorEditor::DrawPreviewCanvas() {
		const float canvasX = 255.0f;
		const float canvasW = 1280.0f - 400.0f - canvasX - 5.0f;
		const float canvasY = 18.0f;
		const float canvasH = 700.0f;

		ImGui::SetNextWindowPos(ImVec2(canvasX, canvasY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(canvasW, canvasH), ImGuiCond_Always);
		ImGui::Begin("Actor Preview", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
		ImVec2 canvasSz = ImGui::GetContentRegionAvail();
		if (canvasSz.x < 50.0f) canvasSz.x = 50.0f;
		if (canvasSz.y < 50.0f) canvasSz.y = 50.0f;
		ImVec2 canvasP1(canvasP0.x + canvasSz.x, canvasP0.y + canvasSz.y);

		drawList->AddRectFilled(canvasP0, canvasP1, MakeCol32(25, 25, 30, 255));
		drawList->AddRect(canvasP0, canvasP1, MakeCol32(70, 70, 80, 255));

		ImGui::InvisibleButton("actor_canvas", canvasSz,
			ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		bool isHovered = ImGui::IsItemHovered();

		drawList->AddText(ImVec2(canvasP0.x + 10, canvasP0.y + 5),
			MakeCol32(180, 180, 200, 255), "Movement Preview");
		char zoomBuf[32];
		snprintf(zoomBuf, sizeof(zoomBuf), "Zoom: %.1fx", previewZoom_);
		drawList->AddText(ImVec2(canvasP1.x - 100, canvasP0.y + 5),
			MakeCol32(150, 150, 170, 200), zoomBuf);

		if (isHovered) {
			float wheel = ImGui::GetIO().MouseWheel;
			if (wheel != 0.0f) {
				previewZoom_ += wheel * 0.3f;
				if (previewZoom_ < 0.5f) previewZoom_ = 0.5f;
				if (previewZoom_ > 15.0f) previewZoom_ = 15.0f;
			}
		}

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			canvasOffsetX_ += delta.x;
			canvasOffsetY_ += delta.y;
		}

		ImVec2 center(canvasP0.x + canvasSz.x * 0.5f + canvasOffsetX_,
			canvasP0.y + canvasSz.y * 0.5f + canvasOffsetY_);
		float scale = previewZoom_ * 30.0f;

		// Grid
		float gridStep = scale;
		if (gridStep < 15.0f) gridStep *= 2.0f;
		if (gridStep < 15.0f) gridStep *= 2.0f;
		for (float gx = center.x; gx < canvasP1.x; gx += gridStep)
			drawList->AddLine(ImVec2(gx, canvasP0.y), ImVec2(gx, canvasP1.y), MakeCol32(40, 40, 45, 255));
		for (float gx = center.x - gridStep; gx > canvasP0.x; gx -= gridStep)
			drawList->AddLine(ImVec2(gx, canvasP0.y), ImVec2(gx, canvasP1.y), MakeCol32(40, 40, 45, 255));
		for (float gy = center.y; gy < canvasP1.y; gy += gridStep)
			drawList->AddLine(ImVec2(canvasP0.x, gy), ImVec2(canvasP1.x, gy), MakeCol32(40, 40, 45, 255));
		for (float gy = center.y - gridStep; gy > canvasP0.y; gy -= gridStep)
			drawList->AddLine(ImVec2(canvasP0.x, gy), ImVec2(canvasP1.x, gy), MakeCol32(40, 40, 45, 255));

		drawList->AddLine(ImVec2(canvasP0.x, center.y), ImVec2(canvasP1.x, center.y), MakeCol32(90, 90, 100, 200));
		drawList->AddLine(ImVec2(center.x, canvasP0.y), ImVec2(center.x, canvasP1.y), MakeCol32(90, 90, 100, 200));
		drawList->AddCircleFilled(center, 4.0f, MakeCol32(255, 200, 50, 200));
		drawList->AddText(ImVec2(center.x + 6, center.y - 14), MakeCol32(200, 200, 200, 200), "Origin");

		auto localToScreen = [&](float lx, float ly) -> ImVec2 {
			return ImVec2(center.x + lx * scale, center.y - ly * scale);
		};

		float baseX = editingActor_.transform.posX;
		float baseY = editingActor_.transform.posY;
		float offX = 0.0f, offY = 0.0f;

		// ===========================================================
		// Spline: MotionEditorの曲線を表示 + タイミングでプレビュー
		// ===========================================================
		if (editingActor_.movement.type == MovementType::Spline && !cachedNodes_.empty()) {
			auto& timings = editingActor_.movement.nodeTimings;
			int nodeCount = static_cast<int>(cachedNodes_.size());
			float totalDur = editingActor_.movement.totalDuration;
			float maxTime = (!timings.empty()) ? timings.back().arrivalTime : totalDur;
			if (maxTime < 0.001f) maxTime = totalDur;

			// --- 曲線描画 ---
			constexpr int SEGMENTS = 120;
			ImVec2 prevPt = localToScreen(
				baseX + cachedNodes_[0].position.X,
				baseY + cachedNodes_[0].position.Y);
			for (int i = 1; i <= SEGMENTS; ++i) {
				float t = maxTime * static_cast<float>(i) / static_cast<float>(SEGMENTS);
				Vector3 pos = EvalSplineAtTime(cachedNodes_, timings, t);
				ImVec2 scrPt = localToScreen(baseX + pos.X, baseY + pos.Y);
				drawList->AddLine(prevPt, scrPt, MakeCol32(255, 200, 50, 180), 2.0f);
				prevPt = scrPt;
			}

			// --- ノード描画 ---
			for (int i = 0; i < nodeCount; ++i) {
				auto& nd = cachedNodes_[i];
				ImVec2 ndScr = localToScreen(baseX + nd.position.X, baseY + nd.position.Y);
				ImVec2 tiScr = localToScreen(baseX + nd.TangentIn.X, baseY + nd.TangentIn.Y);
				ImVec2 toScr = localToScreen(baseX + nd.TangentOut.X, baseY + nd.TangentOut.Y);

				// タンジェントライン + ハンドル
				drawList->AddLine(ndScr, tiScr, MakeCol32(100, 200, 100, 120), 1.0f);
				drawList->AddLine(ndScr, toScr, MakeCol32(200, 100, 100, 120), 1.0f);
				drawList->AddCircleFilled(tiScr, 3.0f, MakeCol32(100, 200, 100, 180));
				drawList->AddCircleFilled(toScr, 3.0f, MakeCol32(200, 100, 100, 180));

				// ノード本体
				ImU32 ndCol = (i == 0) ? MakeCol32(100, 255, 100, 255) :
					(i == nodeCount - 1 ? MakeCol32(255, 100, 100, 255) : MakeCol32(255, 255, 255, 255));
				drawList->AddCircleFilled(ndScr, 6.0f, ndCol);
				drawList->AddCircle(ndScr, 6.0f, MakeCol32(255, 255, 255, 180), 0, 1.5f);

				// ラベル: ノード番号 + 到達時間
				float nodeTime = (i < static_cast<int>(timings.size())) ? timings[i].arrivalTime : 0.0f;
				char ndLabel[48];
				snprintf(ndLabel, sizeof(ndLabel), "%d (%.1fs)", i, nodeTime);
				drawList->AddText(ImVec2(ndScr.x + 9, ndScr.y - 14),
					MakeCol32(255, 255, 255, 200), ndLabel);
			}

			// --- アニメーションプレビュー ---
			if (previewPlaying_) {
				previewTime_ += 1.0f / 60.0f;
				if (editingActor_.movement.loopSpline) {
					previewTime_ = std::fmod(previewTime_, maxTime);
				} else if (previewTime_ > maxTime) {
					previewTime_ = maxTime;
				}
				Vector3 animPos = EvalSplineAtTime(cachedNodes_, timings, previewTime_);
				offX = animPos.X;
				offY = animPos.Y;
			}
		}
		// ===========================================================
		// Linear / PingPong 
		// ===========================================================
		else if (editingActor_.movement.type != MovementType::None) {
			if (previewPlaying_) {
				previewTime_ += 1.0f / 60.0f;
				float range = editingActor_.movement.range;
				float speed = editingActor_.movement.speed;
				float period = (speed > 0.001f) ? (range / speed) : 99999.0f;
				float t = 0.0f;
				if (editingActor_.movement.type == MovementType::Linear) {
					t = std::fmod(previewTime_ * speed, range) / range;
				} else {
					float totalCycle = period * 2.0f;
					float cycleTime = std::fmod(previewTime_, totalCycle);
					t = (cycleTime < period) ? cycleTime / period : 1.0f - (cycleTime - period) / period;
				}
				t = ApplyEasing(t, editingActor_.movement.easing);
				offX = editingActor_.movement.dirX * range * t;
				offY = editingActor_.movement.dirY * range * t;
			}

			// 経路描画
			ImVec2 startPos = localToScreen(baseX, baseY);
			ImVec2 endPos = localToScreen(
				baseX + editingActor_.movement.dirX * editingActor_.movement.range,
				baseY + editingActor_.movement.dirY * editingActor_.movement.range);
			for (int i = 0; i < 30; ++i) {
				float tt = static_cast<float>(i) / 30.0f;
				drawList->AddCircleFilled(
					ImVec2(startPos.x + (endPos.x - startPos.x) * tt, startPos.y + (endPos.y - startPos.y) * tt),
					2.0f, MakeCol32(255, 200, 50, 120));
			}
			drawList->AddCircle(startPos, 6.0f, MakeCol32(100, 255, 100, 180), 0, 1.5f);
			drawList->AddCircle(endPos, 6.0f, MakeCol32(255, 100, 100, 180), 0, 1.5f);
		}

		// --- Actor body ---
		ImVec2 actorScr = localToScreen(baseX + offX, baseY + offY);

		// Collider
		if (editingActor_.collider.type == ColliderType::Box) {
			float hw = editingActor_.collider.sizeX * scale, hh = editingActor_.collider.sizeY * scale;
			drawList->AddRectFilled(ImVec2(actorScr.x - hw, actorScr.y - hh), ImVec2(actorScr.x + hw, actorScr.y + hh), MakeCol32(0, 180, 255, 40));
			drawList->AddRect(ImVec2(actorScr.x - hw, actorScr.y - hh), ImVec2(actorScr.x + hw, actorScr.y + hh), MakeCol32(0, 200, 255, 200), 0, 0, 2.0f);
		} else if (editingActor_.collider.type == ColliderType::Sphere) {
			float r = editingActor_.collider.sizeX * scale;
			drawList->AddCircleFilled(actorScr, r, MakeCol32(0, 180, 255, 40));
			drawList->AddCircle(actorScr, r, MakeCol32(0, 200, 255, 200), 0, 2.0f);
		}

		// Interaction color
		ImU32 actorColor = MakeCol32(80, 255, 160, 255);
		const char* iLabel = "";
		switch (editingActor_.interaction.type) {
		case InteractionType::DamageDealer:  actorColor = MakeCol32(255, 80, 80, 255);  iLabel = "DMG";  break;
		case InteractionType::SolidPlatform: actorColor = MakeCol32(80, 130, 255, 255); iLabel = "PLAT"; break;
		case InteractionType::Trigger:       actorColor = MakeCol32(255, 220, 50, 255); iLabel = "TRIG"; break;
		default: break;
		}
		drawList->AddCircleFilled(actorScr, 10.0f, actorColor);
		drawList->AddCircle(actorScr, 10.0f, MakeCol32(255, 255, 255, 200), 0, 2.0f);
		drawList->AddText(ImVec2(actorScr.x + 14, actorScr.y - 6), MakeCol32(255, 255, 255, 220), iLabel);
		drawList->AddText(ImVec2(actorScr.x - 20, actorScr.y + 14), MakeCol32(200, 200, 200, 180), editingActor_.name.c_str());

		// --- 再生コントロール ---
		ImVec2 btnPos(canvasP0.x + 10, canvasP1.y - 35);
		drawList->AddRectFilled(ImVec2(btnPos.x - 2, btnPos.y - 2), ImVec2(btnPos.x + 220, btnPos.y + 22), MakeCol32(30, 30, 35, 220), 4.0f);
		drawList->AddText(btnPos, previewPlaying_ ? MakeCol32(100, 255, 100, 255) : MakeCol32(180, 180, 190, 255),
			previewPlaying_ ? "[Playing]" : "[Paused]");
		drawList->AddText(ImVec2(btnPos.x + 80, btnPos.y), MakeCol32(150, 150, 170, 200), "Click: Play/Pause");

		if (previewPlaying_ || previewTime_ > 0.0f) {
			char timeBuf[32];
			snprintf(timeBuf, sizeof(timeBuf), "t=%.2fs", previewTime_);
			drawList->AddText(ImVec2(btnPos.x, btnPos.y - 18), MakeCol32(255, 200, 50, 220), timeBuf);
		}

		if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			previewPlaying_ = !previewPlaying_;
			if (!previewPlaying_) previewTime_ = 0.0f;
		}

		// 座標表示
		if (isHovered) {
			ImVec2 mp = ImGui::GetIO().MousePos;
			char cb[64];
			snprintf(cb, sizeof(cb), "(%.2f, %.2f)", (mp.x - center.x) / scale, -(mp.y - center.y) / scale);
			drawList->AddText(ImVec2(mp.x + 15, mp.y - 5), MakeCol32(200, 200, 200, 220), cb);
		}

		ImGui::End();
	}
#endif
}
