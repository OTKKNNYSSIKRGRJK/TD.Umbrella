module Game.MotionManager; 

import <fstream>;
import <filesystem>;

import nlohmann.json;
#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace {
    using Vector3 = Lumina::Math::F32x3;
    using MotionData = std::vector<MathUtils::Spline::Node<Vector3>>;
    using json = nlohmann::json;
    using namespace MathUtils;
}

void MotionManager::SetWalkNodeForMotion(const std::string& motionName, int nodeIndex) {
    debugWalkNodeForMotion_[motionName] = nodeIndex;
}

int MotionManager::GetWalkNodeForMotion(const std::string& motionName) const {
    auto it = debugWalkNodeForMotion_.find(motionName);
    if (it == debugWalkNodeForMotion_.end()) return -1;
    return it->second;
}


std::unique_ptr<MotionManager> MotionManager::instance_ = nullptr;
std::unique_ptr<MotionEditor> MotionEditor::instance_ = nullptr;

void MotionManager::LoadActionData(const std::string& fileName, std::vector<MathUtils::Spline::Node<Vector3>>& outNodes) {
	std::string fullPath = fileName + ".json";
	std::ifstream file(fullPath);

	if (file.is_open()) {
		json j;
		file >> j; // ファイルからJSONを読み込む
		file.close();

		// JSONからNodeの配列に復元して上書き
		outNodes = MathUtils::Spline::DeserializeNodes<Vector3>(j);
	}
}

void MotionManager::LoadMotions(const std::string& directoryPath) {
	motions_.clear();
	if (!std::filesystem::exists(directoryPath)) return;
	for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
		if (entry.is_regular_file() && entry.path().extension() == ".json") {
			std::string motionName = entry.path().stem().string();
			MotionData motionData;
			try {
				LoadActionData(directoryPath + motionName, motionData);
				motions_[motionName] = motionData;
			} catch (...) {
				// JSON形式が異なるファイル（ObjMotionEditor等）はスキップ
			}
		}
	}
}

const MotionData& MotionManager::GetMotion(const std::string& name) const {
  // If no motions were loaded, return a static empty MotionData to avoid
    // dereferencing motions_.begin() when the map is empty.
    if (motions_.empty()) {
        static MotionData kEmptyMotion{};
        return kEmptyMotion;
    }

    auto it = motions_.find(name);
    if (it != motions_.end()) {
        return it->second;
    }

    // Fallback: return the first available motion when the requested one
    // is not found.
    return motions_.begin()->second;
}

void MotionController::Play(const std::string& motionName, const Vector3& startPosition, float motionDuration) {
	currentMotionName_ = motionName;
	motionDuration_ = motionDuration;
	motionTimer_ = 0.0f;
	isPlaying_ = true;
	actionStartPosition_ = startPosition;
	lastLocalOffset_ = Vector3{};
}

Vector3 MotionController::Update(float deltaTime, const Vector3& direction) {
	if (!isPlaying_) return Vector3{};
	auto& motionData = MotionManager::GetInstance()->GetMotion(currentMotionName_);
    motionTimer_ += deltaTime;
    float t = motionTimer_ / motionDuration_;

    // Determine active node index based on t and node positions (node.position.X assumed to be normalized 0..1)
    int activeNode = -1;
    int nodeCount = static_cast<int>(motionData.size());
    if (nodeCount > 0) {
        // Find interval i where t is between node[i].position.X and node[i+1].position.X
        for (int i = 0; i < nodeCount - 1; ++i) {
            float left = motionData[i].position.X;
            float right = motionData[i + 1].position.X;
            if (left > right) std::swap(left, right);
            if (t >= left && t <= right) { activeNode = i; break; }
        }
        // Edge case: if t is exactly at final node position, mark last index
        if (activeNode == -1) {
            if (t >= motionData.back().position.X) activeNode = nodeCount - 1;
            else if (t <= motionData.front().position.X) activeNode = 0;
        }
    }

    // Node event dispatching
    if (prevActiveNodeIndex_ != activeNode) {
        // leaving previous
        if (prevActiveNodeIndex_ >= 0 && nodeEventCallback_) {
            std::string nodeName = (prevActiveNodeIndex_ < (int)nodeNames_.size()) ? nodeNames_[prevActiveNodeIndex_] : std::string();
            if (useIntervalMode_) nodeEventCallback_(currentMotionName_, prevActiveNodeIndex_, nodeName, false);
        }
        // entering new
        if (activeNode >= 0 && nodeEventCallback_) {
            std::string nodeName = (activeNode < (int)nodeNames_.size()) ? nodeNames_[activeNode] : std::string();
            nodeEventCallback_(currentMotionName_, activeNode, nodeName, true);
        }
        prevActiveNodeIndex_ = activeNode;
    } else if (activeNode >= 0 && !useIntervalMode_ && nodeEventCallback_ && motionTimer_ == deltaTime) {
        // one-shot mode: invoke when motion starts on the node (handled above on change). No-op here.
    }

    Vector3 startOffset = MathUtils::Spline::GetPointSpline(motionData, 0.0f);
    Vector3 localOffset = MathUtils::Spline::GetPointSpline(motionData, t);
    localOffset.X -= startOffset.X;
    localOffset.Y -= startOffset.Y;
    localOffset.Z -= startOffset.Z;

	localOffset.Y *= -1.0f;
	localOffset.X *= direction.X >= 0 ? 1.0f : -1.0f; // 方向に応じて左右反転

	lastLocalOffset_ = localOffset;

	if (motionTimer_ >= motionDuration_) {
		isPlaying_ = false; // 再生終了
	}

	return actionStartPosition_ + localOffset;
}

#if defined(_DEBUG)
namespace {
    constexpr ImU32 MakeCol32(int r, int g, int b, int a) {
        return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r);
    }
}
#endif

void MotionEditor::NodeImGui() {
#if defined(_DEBUG)
    if (ImGui::IsKeyPressed(ImGuiKey_P)) {
        nodes_.push_back(Spline::Node<Vector3>({ 0.0f,0.0f,0.0f }));
    }

    ImGui::Begin("Action Editor (Hermite Spline)");

    // --- 1. キャンバスの準備 ---
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // ピクセル設定
    const float DISPLAY_SCALE = 75.0f;

    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
    if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

    draw_list->AddRectFilled(canvas_p0, canvas_p1, MakeCol32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, MakeCol32(255, 255, 255, 255));

    ImGui::InvisibleButton("canvas", canvas_sz);
    ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_p0.x, ImGui::GetIO().MousePos.y - canvas_p0.y);
    bool is_hovered = ImGui::IsItemHovered();

    // --- 2. スプライン曲線の描画 ---
    if (nodes_.size() >= 2) {
        const int num_segments = 100;
        // キャンバス中心を使ってノード描画と同じ基準にする
        ImVec2 canvas_center = ImVec2(canvas_p0.x + canvas_sz.x * 0.5f, canvas_p0.y + canvas_sz.y * 0.5f);

        Vector3 prev_point = nodes_[0].position;
        for (int i = 1; i <= num_segments; ++i) {
            float t = (float)i / (float)num_segments;
            Vector3 current_point = MathUtils::Spline::GetPointSpline(nodes_, t);

            // 中心基準でスクリーン座標に変換
            ImVec2 p1 = ImVec2(canvas_center.x + prev_point.X * DISPLAY_SCALE, canvas_center.y + prev_point.Y * DISPLAY_SCALE);
            ImVec2 p2 = ImVec2(canvas_center.x + current_point.X * DISPLAY_SCALE, canvas_center.y + current_point.Y * DISPLAY_SCALE);

            draw_list->AddLine(p1, p2, MakeCol32(255, 200, 0, 255), 2.0f);
            prev_point = current_point;
        }
    }

    // --- 3. ノードとハンドルの操作＆描画 ---
    const float NODE_RADIUS = 6.0f;
    const float HANDLE_RADIUS = 4.0f;

    for (int i = 0; i < (int)nodes_.size(); ++i) {
        auto& node = nodes_[i];

        ImVec2 canvas_center = ImVec2(canvas_p0.x + canvas_sz.x * 0.5f, canvas_p0.y + canvas_sz.y * 0.5f);
        ImVec2 pos_screen = ImVec2(canvas_center.x + node.position.X * DISPLAY_SCALE, canvas_center.y + node.position.Y * DISPLAY_SCALE);
        ImVec2 in_screen = ImVec2(canvas_center.x + node.TangentIn.X * DISPLAY_SCALE, canvas_center.y + node.TangentIn.Y * DISPLAY_SCALE);
        ImVec2 out_screen = ImVec2(canvas_center.x + node.TangentOut.X * DISPLAY_SCALE, canvas_center.y + node.TangentOut.Y * DISPLAY_SCALE);

        draw_list->AddLine(pos_screen, in_screen, MakeCol32(150, 150, 150, 200), 1.0f);
        draw_list->AddLine(pos_screen, out_screen, MakeCol32(150, 150, 150, 200), 1.0f);

        // --- クリック判定（左/右/中 を全てチェック） ---
        if (is_hovered && ImGui::IsMouseClicked(0)) {
            auto dist = [](ImVec2 a, ImVec2 b) { return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y); };
            ImVec2 mouse_p = ImGui::GetIO().MousePos;
            if (dist(mouse_p, pos_screen) < NODE_RADIUS * NODE_RADIUS * 4.0f) {
                draggedNodeIndex = i; draggedHandleType = 0; // left on node
                selectedNodeIndex_ = i;
            }
        }
        else if (is_hovered && ImGui::IsMouseClicked(1)) {
            auto dist = [](ImVec2 a, ImVec2 b) { return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y); };
            ImVec2 mouse_p = ImGui::GetIO().MousePos;
            if (dist(mouse_p, in_screen) < HANDLE_RADIUS * HANDLE_RADIUS * 4.0f) {
                draggedNodeIndex = i; draggedHandleType = 1; // right -> in
            }
        }
        else if (is_hovered && ImGui::IsMouseClicked(2)) {
            auto dist = [](ImVec2 a, ImVec2 b) { return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y); };
            ImVec2 mouse_p = ImGui::GetIO().MousePos;
            if (dist(mouse_p, out_screen) < HANDLE_RADIUS * HANDLE_RADIUS * 4.0f) {
                draggedNodeIndex = i; draggedHandleType = 2; // middle -> out
            }
        }

        ImU32 colorNode = MakeCol32(255, 255, 255, 255);
        if (MotionEditor::GetInstance()->selectedNodeIndex_ == i) {
            colorNode = MakeCol32(100, 255, 100, 255);
        }
        draw_list->AddCircleFilled(in_screen, HANDLE_RADIUS, MakeCol32(100, 200, 100, 255));
        draw_list->AddCircleFilled(out_screen, HANDLE_RADIUS, MakeCol32(200, 100, 100, 255));
        draw_list->AddCircleFilled(pos_screen, NODE_RADIUS, colorNode);
    }

    // --- 4. ドラッグ中の座標更新 ---
    if (draggedNodeIndex >= 0 && draggedHandleType >= 0 && ImGui::IsMouseDragging(draggedHandleType)) {
        auto& node = nodes_[draggedNodeIndex];
        ImVec2 delta = ImGui::GetIO().MouseDelta;

        bool isAltDown = ImGui::IsKeyDown(ImGuiKey_LeftAlt);
        node.isBroken = isAltDown;

        if (draggedHandleType == 0) {
            node.position.X += delta.x / DISPLAY_SCALE; node.position.Y += delta.y / DISPLAY_SCALE;
            node.TangentIn.X += delta.x / DISPLAY_SCALE; node.TangentIn.Y += delta.y / DISPLAY_SCALE;
            node.TangentOut.X += delta.x / DISPLAY_SCALE; node.TangentOut.Y += delta.y / DISPLAY_SCALE;
        }
        else if (draggedHandleType == 1) {
            node.TangentIn.X += delta.x / DISPLAY_SCALE; node.TangentIn.Y += delta.y / DISPLAY_SCALE;
            if (node.isBroken) {
                node.TangentOut.X = node.position.X + (node.position.X - node.TangentIn.X);
                node.TangentOut.Y = node.position.Y + (node.position.Y - node.TangentIn.Y);
            }
        }
        else if (draggedHandleType == 2) {
            node.TangentOut.X += delta.x / DISPLAY_SCALE; node.TangentOut.Y += delta.y / DISPLAY_SCALE;
            if (node.isBroken) {
                node.TangentIn.X = node.position.X + (node.position.X - node.TangentOut.X);
                node.TangentIn.Y = node.position.Y + (node.position.Y - node.TangentOut.Y);
            }
        }
    }

    // ドラッグ解除（押していたボタンが離れたら解除）
    if (draggedNodeIndex >= 0) {
        if (!ImGui::IsMouseDown(draggedHandleType)) {
            draggedNodeIndex = -1;
            draggedHandleType = -1;
        }
    }

    // 保存システム ここから↓↓↓
    char buf[128];
    strncpy_s(buf, sizeof(buf), inputNodeName_.c_str(), _TRUNCATE);
    buf[sizeof(buf) - 1] = 0; // 確実にNULL終端
    if (ImGui::InputText("Node Name", buf, sizeof(buf))) {
        inputNodeName_ = std::string(buf);
    }
    if (ImGui::Button("Save")) {
        SaveNode(inputNodeName_, nodes_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        nodes_ = MotionManager::GetInstance()->GetMotion(inputNodeName_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
		nodes_.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("ReLoad")) {
		MotionManager::GetInstance()->LoadMotions("Assets/Data/Motion/");
    }

    if (selectedNodeIndex_ >= 0 && selectedNodeIndex_ < static_cast<int>(nodes_.size())) {
        ImGui::SeparatorText("Selected Motion Node");
        ImGui::Text("Selected Index: %d", selectedNodeIndex_);
        if (ImGui::Button("Use selected node as Walk trigger")) {
            MotionManager::GetInstance()->SetWalkNodeForMotion(inputNodeName_, selectedNodeIndex_);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Current Walk Node: %d", MotionManager::GetInstance()->GetWalkNodeForMotion(inputNodeName_));
    }

    // 保存システム ここまで↑↑↑

    ImGui::End();
    ImGui::Begin("Hermite Positions");
    int index = 0;
    if (ImGui::TreeNodeEx("Positions")) {
        // 参照渡しで実際のノードを編集する
        for (int idx = 0; idx < (int)nodes_.size(); ++idx) {
            if (ImGui::TreeNodeEx(std::to_string(index).c_str())) {
                auto& node = nodes_[idx];
                ImGui::DragFloat3((std::string("node") + std::to_string(index)).c_str(), &node.position.X);
                ImGui::DragFloat3((std::string("in") + std::to_string(index)).c_str(), &node.TangentIn.X);
                ImGui::DragFloat3((std::string("out") + std::to_string(index)).c_str(), &node.TangentOut.X);
                if (ImGui::Button("Delete")) {
                    nodes_.erase(nodes_.begin() + idx);
                }
                index++;
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::End();
#endif // _DEBUG
}

void MotionEditor::SaveNode(const std::string& filename, const std::vector<Spline::Node<Vector3>>& nodes) {
    std::string filePath = "Assets/Data/Motion/" + filename + ".json";
    json j = MathUtils::Spline::SerializeNodes(nodes);
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << j.dump(4); // インデント幅を4スペースにして保存
        file.close();
    }
}