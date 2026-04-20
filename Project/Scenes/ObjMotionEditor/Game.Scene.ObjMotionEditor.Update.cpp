module Game.Editor.ObjMotionEditor;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import <string>;
import <filesystem>;
import <array>;
import <vector>;
import <algorithm>;
import <cmath>;

namespace fs = std::filesystem;

#if defined(_DEBUG)
namespace {
	constexpr ImU32 MakeCol32(int r, int g, int b, int a) {
		return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r);
	}
}
#endif

namespace Game::Editor {

	void ObjMotionEditor::PushUndo() {
		undoStack_.push_back(editingMotion_);
		redoStack_.clear();
		if (undoStack_.size() > 50) {
			undoStack_.erase(undoStack_.begin());
		}
	}

	void ObjMotionEditor::Undo() {
		if (!undoStack_.empty()) {
			redoStack_.push_back(editingMotion_);
			editingMotion_ = undoStack_.back();
			undoStack_.pop_back();
			selectedKeyframeIndex_ = -1;
		}
	}

	void ObjMotionEditor::Redo() {
		if (!redoStack_.empty()) {
			undoStack_.push_back(editingMotion_);
			editingMotion_ = redoStack_.back();
			redoStack_.pop_back();
			selectedKeyframeIndex_ = -1;
		}
	}

	void ObjMotionEditor::Update() {
#if defined(_DEBUG)
		auto& io = ImGui::GetIO();
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
			Undo();
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
			Redo();
		}

		if (isPlaying_) {
			float dt = ImGui::GetIO().DeltaTime;
			currentTime_ += dt;
			if (currentTime_ > editingMotion_.duration) {
				if (editingMotion_.isLoop) {
					currentTime_ = std::fmod(currentTime_, std::max(0.001f, editingMotion_.duration));
				} else {
					currentTime_ = editingMotion_.duration;
					isPlaying_ = false;
				}
			}
		}

		DrawEditorUI();
#endif
	}

#if defined(_DEBUG)
	void ObjMotionEditor::AddKeyframe(float time) {
		// すでに同時間のキーがあるか探す
		for (size_t i = 0; i < editingMotion_.keyframes.size(); ++i) {
			if (std::abs(editingMotion_.keyframes[i].time - time) < 0.001f) {
				selectedKeyframeIndex_ = static_cast<int>(i);
				return;
			}
		}

		ObjMotionKeyframe kf;
		kf.time = time;
		// 補間した値を初期値にするとなお良いが、とりあえず0か現在の設定を引き継ぐ
		if (selectedKeyframeIndex_ >= 0 && selectedKeyframeIndex_ < static_cast<int>(editingMotion_.keyframes.size())) {
			auto& sel = editingMotion_.keyframes[selectedKeyframeIndex_];
			kf.px = sel.px; kf.py = sel.py; kf.pz = sel.pz;
			kf.rx = sel.rx; kf.ry = sel.ry; kf.rz = sel.rz;
			kf.sx = sel.sx; kf.sy = sel.sy; kf.sz = sel.sz;
		} else {
			kf.sx = 1.0f; kf.sy = 1.0f; kf.sz = 1.0f;
		}

		editingMotion_.keyframes.push_back(kf);
		// 時間順にソート
		std::sort(editingMotion_.keyframes.begin(), editingMotion_.keyframes.end(), [](const ObjMotionKeyframe& a, const ObjMotionKeyframe& b) {
			return a.time < b.time;
		});

		// 選択し直す
		for (size_t i = 0; i < editingMotion_.keyframes.size(); ++i) {
			if (std::abs(editingMotion_.keyframes[i].time - time) < 0.001f) {
				selectedKeyframeIndex_ = static_cast<int>(i);
				break;
			}
		}
	}

	void ObjMotionEditor::DrawEditorUI() {
		// 左カラム: モーションファイル一覧
		ImGui::SetNextWindowPos(ImVec2(0, 18), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(250, 702), ImGuiCond_Always);
		ImGui::Begin("Motion Browser", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::TextDisabled("ENEMY FILES");
		ImGui::Separator();
		ImGui::BeginChild("FileList", ImVec2(0, 0), false);
		
		const std::string dir = "./";
		if (fs::exists(dir)) {
			for (const auto& entry : fs::directory_iterator(dir)) {
				if (entry.path().extension() == ".json") {
					std::string fPath = entry.path().string();
					std::string fName = entry.path().filename().string();
					if (fName.find("area") == 0 || fName.find("actor") == 0) continue;

					bool isSelected = (editingEnemyFile_ == fPath);
					if (ImGui::Selectable(fName.c_str(), isSelected)) {
						LoadEnemy(fPath);
						selectedKeyframeIndex_ = -1;
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::End();

		// 右カラム: インスペクター
		ImGui::SetNextWindowPos(ImVec2(1280.0f - 350.0f, 18.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(350, 700), ImGuiCond_Always);
		ImGui::Begin("Motion Inspector", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		if (ImGui::CollapsingHeader("Enemy & Motion Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Enemy Name: %s", cachedEnemyData_.name.c_str());
			ImGui::TextDisabled("Model: %s", cachedEnemyData_.gltfPath.c_str());

			ImGui::Separator();
			
			if (cachedEnemyData_.motionMap.empty()) {
				ImGui::TextDisabled("No motion entries in this enemy.");
			} else {
				if (ImGui::BeginCombo("Select Action", currentActionName_.empty() ? "(None)" : currentActionName_.c_str())) {
					for (const auto& [action, motionFile] : cachedEnemyData_.motionMap) {
						bool isSelected = (currentActionName_ == action);
						if (ImGui::Selectable(action.c_str(), isSelected)) {
							currentActionName_ = action;
							LoadMotion(editingMotion_, "Assets/Data/Motion/" + motionFile + ".json");
							// 念のためMotion Nameをファイルパスに合わせる
							editingMotion_.name = motionFile;
							selectedKeyframeIndex_ = -1;
						}
					}
					ImGui::EndCombo();
				}
			}
			
			ImGui::Spacing();
			char nameBuf[256];
			strncpy_s(nameBuf, editingMotion_.name.c_str(), sizeof(nameBuf));
			if (ImGui::InputText("Motion Save Name", nameBuf, sizeof(nameBuf))) {
				editingMotion_.name = nameBuf;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) { PushUndo(); }

			if (ImGui::DragFloat("Duration (s)", &editingMotion_.duration, 0.01f, 0.01f, 60.0f, "%.2f")) {
				// To avoid tons of undo frames, we do it in IsItemDeactivatedAfterEdit
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) { PushUndo(); }

			if (ImGui::Checkbox("Loop", &editingMotion_.isLoop)) {
				PushUndo();
			}
		}

		DrawKeyframeProperties();

		ImGui::Separator();
		ImGui::Spacing();
		if (ImGui::Button("SAVE ASSET", ImVec2(-1, 40))) {
			SaveMotion(editingMotion_);
		}
		ImGui::End();

		// 下部カラム: タイムライン
		DrawTimelineUI();

		// 中央カラム: プレビュー用キャンバス
		DrawPreviewCanvas();
	}

	void ObjMotionEditor::DrawKeyframeProperties() {
		if (ImGui::CollapsingHeader("Selected Keyframe", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (selectedKeyframeIndex_ >= 0 && selectedKeyframeIndex_ < static_cast<int>(editingMotion_.keyframes.size())) {
				auto& kf = editingMotion_.keyframes[selectedKeyframeIndex_];
				
				float previousTime = kf.time;
				if (ImGui::DragFloat("Time", &kf.time, 0.01f, 0.0f, editingMotion_.duration, "%.3f")) {
					// ソートが必要になるかもしれないが、ドラッグ中はややこしいのでここではソートしない
					// 本当はマウスを離した時にソートする処理が要る
				}

				ImGui::Text("Position Offset");
				if (ImGui::DragFloat3("Pos", &kf.px, 0.05f)) { currentTime_ = kf.time; }
				if (ImGui::IsItemDeactivatedAfterEdit()) { PushUndo(); }

				ImGui::Text("Rotation (Degrees)");
				if (ImGui::DragFloat3("Rot", &kf.rx, 1.0f)) { currentTime_ = kf.time; }
				if (ImGui::IsItemDeactivatedAfterEdit()) { PushUndo(); }

				ImGui::Text("Scale");
				if (ImGui::DragFloat3("Scl", &kf.sx, 0.05f)) { currentTime_ = kf.time; }
				if (ImGui::IsItemDeactivatedAfterEdit()) { PushUndo(); }

				ImGui::Spacing();
				if (ImGui::Button("Delete Keyframe", ImVec2(-1, 0))) {
					PushUndo();
					editingMotion_.keyframes.erase(editingMotion_.keyframes.begin() + selectedKeyframeIndex_);
					selectedKeyframeIndex_ = -1;
				}

				if (previousTime != kf.time && !ImGui::IsItemActive()) {
					std::sort(editingMotion_.keyframes.begin(), editingMotion_.keyframes.end(), [](const ObjMotionKeyframe& a, const ObjMotionKeyframe& b) {
						return a.time < b.time;
					});
					for (size_t i = 0; i < editingMotion_.keyframes.size(); ++i) {
						if (&editingMotion_.keyframes[i] == &kf) {
							// std::sort でコピーされるため、ポインタ比較はおかしいが、時間で探す
							break;
						}
					}
					// 時間で再検索
					for (size_t i = 0; i < editingMotion_.keyframes.size(); ++i) {
						if (std::abs(editingMotion_.keyframes[i].time - kf.time) < 0.0001f) {
							selectedKeyframeIndex_ = static_cast<int>(i);
							break;
						}
					}
				}

			} else {
				ImGui::TextDisabled("No keyframe selected.");
			}
		}
	}

	void ObjMotionEditor::DrawTimelineUI() {
		const float timelineX = 255.0f;
		const float timelineY = 720.0f - 180.0f;
		const float timelineW = 1280.0f - 350.0f - timelineX - 5.0f;
		const float timelineH = 180.0f;

		ImGui::SetNextWindowPos(ImVec2(timelineX, timelineY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(timelineW, timelineH), ImGuiCond_Always);
		ImGui::Begin("Timeline", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		if (ImGui::Button(isPlaying_ ? "Pause" : "Play ")) {
			isPlaying_ = !isPlaying_;
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop")) {
			isPlaying_ = false;
			currentTime_ = 0.0f;
		}

		ImGui::SameLine();
		ImGui::Text("Time: %.3f / %.3f", currentTime_, editingMotion_.duration);

		ImGui::Separator();

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
		ImVec2 canvasSz = ImGui::GetContentRegionAvail();
		ImVec2 canvasP1 = ImVec2(canvasP0.x + canvasSz.x, canvasP0.y + canvasSz.y);

		drawList->AddRectFilled(canvasP0, canvasP1, MakeCol32(20, 20, 20, 255));

		ImGui::InvisibleButton("timeline_canvas", canvasSz);
		bool isHovered = ImGui::IsItemHovered();
		ImVec2 mousePos = ImGui::GetIO().MousePos;

		float padX = 10.0f;
		float trackW = canvasSz.x - padX * 2.0f;
		float trackY = canvasP0.y + 40.0f;

		// タイムラインの軸
		drawList->AddLine(ImVec2(canvasP0.x + padX, trackY), ImVec2(canvasP1.x - padX, trackY), MakeCol32(100, 100, 100, 255));

		// 現在時刻の線
		float curX = canvasP0.x + padX + (currentTime_ / std::max(0.001f, editingMotion_.duration)) * trackW;
		drawList->AddLine(ImVec2(curX, canvasP0.y), ImVec2(curX, canvasP1.y), MakeCol32(255, 100, 50, 200), 2.0f);

		int hoveredKeyframeIdx = -1;
		for (size_t i = 0; i < editingMotion_.keyframes.size(); ++i) {
			const auto& kf = editingMotion_.keyframes[i];
			float kx = canvasP0.x + padX + (kf.time / std::max(0.001f, editingMotion_.duration)) * trackW;
			
			if (isHovered && std::abs(mousePos.x - kx) < 8.0f && std::abs(mousePos.y - trackY) < 12.0f) {
				hoveredKeyframeIdx = static_cast<int>(i);
			}

			bool isSelected = (selectedKeyframeIndex_ == static_cast<int>(i));
			ImU32 col = isSelected ? MakeCol32(255, 255, 0, 255) : MakeCol32(0, 200, 255, 255);

			drawList->AddCircleFilled(ImVec2(kx, trackY), 6.0f, col);
			drawList->AddCircle(ImVec2(kx, trackY), 7.0f, MakeCol32(255,255,255,200));
		}

		if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			if (hoveredKeyframeIdx >= 0) {
				selectedKeyframeIndex_ = hoveredKeyframeIdx;
				currentTime_ = editingMotion_.keyframes[selectedKeyframeIndex_].time;
			} else {
				float clickTime = ((mousePos.x - (canvasP0.x + padX)) / trackW) * editingMotion_.duration;
				if (clickTime >= 0.0f && clickTime <= editingMotion_.duration) {
					PushUndo();
					AddKeyframe(clickTime);
				}
			}
		}

		if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			if (hoveredKeyframeIdx >= 0) {
				PushUndo();
				editingMotion_.keyframes.erase(editingMotion_.keyframes.begin() + hoveredKeyframeIdx);
				selectedKeyframeIndex_ = -1;
			}
		}

		static bool wasDragging = false;
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			if (!wasDragging) {
				PushUndo(); 
				wasDragging = true;
			}
			if (selectedKeyframeIndex_ >= 0) {
				float newTime = ((mousePos.x - (canvasP0.x + padX)) / trackW) * editingMotion_.duration;
				newTime = std::clamp(newTime, 0.0f, editingMotion_.duration);
				editingMotion_.keyframes[selectedKeyframeIndex_].time = newTime;
				currentTime_ = newTime;
			} else {
				float newTime = ((mousePos.x - (canvasP0.x + padX)) / trackW) * editingMotion_.duration;
				currentTime_ = std::clamp(newTime, 0.0f, editingMotion_.duration);
			}
		} else if (!ImGui::IsItemActive() && wasDragging) {
			wasDragging = false;
			// Sort when drag finishes
			if (selectedKeyframeIndex_ >= 0) {
				auto kf = editingMotion_.keyframes[selectedKeyframeIndex_];
				std::sort(editingMotion_.keyframes.begin(), editingMotion_.keyframes.end(), [](const ObjMotionKeyframe& a, const ObjMotionKeyframe& b) {
					return a.time < b.time;
				});
				for (size_t i = 0; i < editingMotion_.keyframes.size(); ++i) {
					if (std::abs(editingMotion_.keyframes[i].time - kf.time) < 0.0001f) {
						selectedKeyframeIndex_ = static_cast<int>(i);
						break;
					}
				}
			}
		}

		ImGui::End();
	}

	void ObjMotionEditor::DrawPreviewCanvas() {
		const float canvasX = 255.0f;
		const float canvasY = 18.0f;
		const float canvasW = 1280.0f - 350.0f - canvasX - 5.0f;
		const float canvasH = 720.0f - 180.0f - canvasY - 5.0f;

		ImGui::SetNextWindowPos(ImVec2(canvasX, canvasY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(canvasW, canvasH), ImGuiCond_Always);
		ImGui::Begin("Motion Preview", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::SliderFloat("Camera Zoom", &zoom_, 0.1f, 10.0f);
		ImGui::SameLine();
		const char* views[] = { "Front (XY)", "Side (ZY)", "Top (XZ)" };
		ImGui::Combo("View", &meshViewMode_, views, 3);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
		ImVec2 canvasSz = ImGui::GetContentRegionAvail();
		ImVec2 canvasP1 = ImVec2(canvasP0.x + canvasSz.x, canvasP0.y + canvasSz.y);

		drawList->AddRectFilled(canvasP0, canvasP1, MakeCol32(30,30,35,255));
		ImGui::InvisibleButton("preview_canvas", canvasSz, ImGuiButtonFlags_MouseButtonRight);

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			canvasOffsetX_ += ImGui::GetIO().MouseDelta.x;
			canvasOffsetY_ += ImGui::GetIO().MouseDelta.y;
		}

		if (cachedMeshObjPath_ != cachedEnemyData_.gltfPath) {
			ExtractMeshWireframe(cachedEnemyData_.gltfPath);
		}

		ImVec2 center = ImVec2(canvasP0.x + canvasSz.x*0.5f + canvasOffsetX_, canvasP0.y + canvasSz.y*0.5f + canvasOffsetY_);
		float scale = zoom_ * 30.0f;

		// --- Eval Action at currentTime_ ---
		ObjMotionKeyframe eval;
		eval.sx = 1; eval.sy = 1; eval.sz = 1;
		if (!editingMotion_.keyframes.empty()) {
			if (editingMotion_.keyframes.size() == 1) {
				eval = editingMotion_.keyframes[0];
			} else {
				// Find before and after
				ObjMotionKeyframe k0 = editingMotion_.keyframes.front();
				ObjMotionKeyframe k1 = editingMotion_.keyframes.back();
				for (size_t i = 0; i < editingMotion_.keyframes.size() - 1; ++i) {
					if (editingMotion_.keyframes[i].time <= currentTime_ && editingMotion_.keyframes[i+1].time >= currentTime_) {
						k0 = editingMotion_.keyframes[i];
						k1 = editingMotion_.keyframes[i+1];
						break;
					}
				}
				if (k0.time <= currentTime_ && k1.time >= currentTime_ && k0.time != k1.time) {
					float t = (currentTime_ - k0.time) / (k1.time - k0.time);
					eval.px = k0.px + (k1.px - k0.px) * t;
					eval.py = k0.py + (k1.py - k0.py) * t;
					eval.pz = k0.pz + (k1.pz - k0.pz) * t;

					eval.rx = k0.rx + (k1.rx - k0.rx) * t;
					eval.ry = k0.ry + (k1.ry - k0.ry) * t;
					eval.rz = k0.rz + (k1.rz - k0.rz) * t;

					eval.sx = k0.sx + (k1.sx - k0.sx) * t;
					eval.sy = k0.sy + (k1.sy - k0.sy) * t;
					eval.sz = k0.sz + (k1.sz - k0.sz) * t;
				} else if (currentTime_ < editingMotion_.keyframes.front().time) {
					eval = editingMotion_.keyframes.front();
				} else {
					eval = editingMotion_.keyframes.back();
				}
			}
		}

		// Transform Matrix creation (ZYX Euler for rx, ry, rz in degrees)
		auto deg2rad = [](float d) { return d * 3.14159265f / 180.0f; };
		float cx = std::cos(deg2rad(eval.rx)), sx = std::sin(deg2rad(eval.rx));
		float cy = std::cos(deg2rad(eval.ry)), sy = std::sin(deg2rad(eval.ry));
		float cz = std::cos(deg2rad(eval.rz)), sz = std::sin(deg2rad(eval.rz));

		std::array<float, 16> M;
		// Initialize to zero
		for (int i=0; i<16; i++) M[i] = 0;
		M[15] = 1.0f;

		// S * R * T matrix? Direct evaluation:
		// X-axis: P = Rx * Ry * Rz * (S * p) + T ... Wait, Standard order: T * Ry * Rx * Rz * S?
		// Usually Transform is T * R * S
		// R = Ry * Rx * Rz for standard character?

		// M00 S*cy*cz ... let's do manual mult:
		// Rz
		std::array<float, 9> Rz = { cz, sz, 0, -sz, cz, 0, 0, 0, 1 };
		// Rx
		std::array<float, 9> Rx = { 1, 0, 0, 0, cx, sx, 0, -sx, cx };
		// Ry
		std::array<float, 9> Ry = { cy, 0, -sy, 0, 1, 0, sy, 0, cy };

		auto mult3x3 = [](const std::array<float,9>& a, const std::array<float,9>& b) {
			std::array<float,9> c{};
			for(int i=0;i<3;i++) for(int j=0;j<3;j++) for(int k=0;k<3;k++) c[i+j*3] += a[i+k*3] * b[k+j*3];
			return c;
		};
		// R = Ry * Rx * Rz
		auto R = mult3x3(Ry, mult3x3(Rx, Rz));

		auto transformPt = [&](float x, float y, float z) -> std::array<float, 3> {
			float sx_val = x * eval.sx;
			float sy_val = y * eval.sy;
			float sz_val = z * eval.sz;

			float rxx = R[0]*sx_val + R[3]*sy_val + R[6]*sz_val;
			float ryy = R[1]*sx_val + R[4]*sy_val + R[7]*sz_val;
			float rzz = R[2]*sx_val + R[5]*sy_val + R[8]*sz_val;

			return { rxx + eval.px, ryy + eval.py, rzz + eval.pz };
		};


		// Projection
		auto project3D = [&](const std::array<float, 3>& pos) -> ImVec2 {
			float px, py;
			switch (meshViewMode_) {
			case 0: px = pos[0]; py = pos[1]; break; // Front
			case 1: px = pos[2]; py = pos[1]; break; // Side
			case 2: px = pos[0]; py = pos[2]; break; // Top
			default:px = pos[0]; py = pos[1]; break;
			}
			return ImVec2(center.x + px * scale, center.y - py * scale);
		};

		// 基準軸描画
		drawList->AddLine(ImVec2(canvasP0.x, center.y), ImVec2(canvasP1.x, center.y), MakeCol32(100,100,110,200), 1.0f);
		drawList->AddLine(ImVec2(center.x, canvasP0.y), ImVec2(center.x, canvasP1.y), MakeCol32(100,100,110,200), 1.0f);

		// Draw Wireframe
		if (!cachedMeshFaces_.empty()) {
			struct Face {
				ImVec2 p0, p1, p2;
				float depth;
			};
			std::vector<Face> renderFaces;

			for (const auto& face : cachedMeshFaces_) {
				if (face[0] < 0 || face[0] >= cachedMeshPositions_.size()) continue;
				if (face[1] < 0 || face[1] >= cachedMeshPositions_.size()) continue;
				if (face[2] < 0 || face[2] >= cachedMeshPositions_.size()) continue;

				auto t0 = transformPt(cachedMeshPositions_[face[0]][0], cachedMeshPositions_[face[0]][1], cachedMeshPositions_[face[0]][2]);
				auto t1 = transformPt(cachedMeshPositions_[face[1]][0], cachedMeshPositions_[face[1]][1], cachedMeshPositions_[face[1]][2]);
				auto t2 = transformPt(cachedMeshPositions_[face[2]][0], cachedMeshPositions_[face[2]][1], cachedMeshPositions_[face[2]][2]);

				ImVec2 p0 = project3D(t0);
				ImVec2 p1 = project3D(t1);
				ImVec2 p2 = project3D(t2);

				// カリング
				if (p0.x < canvasP0.x && p1.x < canvasP0.x && p2.x < canvasP0.x) continue;
				if (p0.x > canvasP1.x && p1.x > canvasP1.x && p2.x > canvasP1.x) continue;
				if (p0.y < canvasP0.y && p1.y < canvasP0.y && p2.y < canvasP0.y) continue;
				if (p0.y > canvasP1.y && p1.y > canvasP1.y && p2.y > canvasP1.y) continue;

				float d = 0;
				switch(meshViewMode_) {
				case 0: d = -(t0[2]+t1[2]+t2[2]); break;
				case 1: d = -(t0[0]+t1[0]+t2[0]); break;
				case 2: d = -(t0[1]+t1[1]+t2[1]); break;
				}
				renderFaces.push_back({ p0, p1, p2, d });
			}
			std::sort(renderFaces.begin(), renderFaces.end(), [](const Face& a, const Face& b){
				return a.depth > b.depth;
			});

			for (const auto& f : renderFaces) {
				drawList->AddTriangleFilled(f.p0, f.p1, f.p2, MakeCol32(120,150,200,255));
				drawList->AddTriangle(f.p0, f.p1, f.p2, MakeCol32(50,70,90,80), 1.0f);
			}
		}

		// Draw 2D Collision Box
		if (!cachedEnemyData_.collisionVertices.empty()) {
			std::vector<ImVec2> colRender;
			for (const auto& v : cachedEnemyData_.collisionVertices) {
				// To apply 3D transformation to a 2D collision point (assuming it lies on the XY plane)
				// When in game, it's scaled by modelScale then rotated/translated.
				// For preview, we match what happens to the mesh.
				auto tP = transformPt(v.x, v.y, 0.0f);
				colRender.push_back(project3D(tP));
			}
			
			// カリングテスト
			bool allOutside = true;
			for (const auto& p : colRender) {
				if (p.x >= canvasP0.x && p.x <= canvasP1.x && p.y >= canvasP0.y && p.y <= canvasP1.y) {
					allOutside = false;
					break;
				}
			}
			
			if (!allOutside) {
				// Draw the polygon
				for (size_t i = 0; i < colRender.size(); ++i) {
					size_t next_i = (i + 1) % colRender.size();
					drawList->AddLine(colRender[i], colRender[next_i], MakeCol32(255, 50, 50, 255), 2.0f);
				}
				for (size_t i = 0; i < colRender.size(); ++i) {
					drawList->AddCircleFilled(colRender[i], 4.0f, MakeCol32(255, 100, 100, 255));
				}
			}
		}

		ImGui::End();
	}
#endif
}
