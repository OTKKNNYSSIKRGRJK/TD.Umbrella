export module Game.MotionManager;

import <string>;
import <memory>;
import <unordered_map>;
import <functional>;
import <vector>;

import Hermite;
import Lumina.Core.Math;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using MotionData = std::vector<MathUtils::Spline::Node<Vector3>>;
}

export class MotionManager {
public:
	static MotionManager* GetInstance() {
		if (instance_ == nullptr) {
			instance_ = std::make_unique<MotionManager>();
		}
		return instance_.get();
	}
public:
	void LoadActionData(const std::string& fileName, std::vector<MathUtils::Spline::Node<Vector3>>& outNodes);
	void LoadMotions(const std::string& directoryPath);
	const MotionData& GetMotion(const std::string& name) const;

	// Debug: assign a motion-local node index to be treated as the "walk" trigger
	void SetWalkNodeForMotion(const std::string& motionName, int nodeIndex);
	int GetWalkNodeForMotion(const std::string& motionName) const;
private:
	static std::unique_ptr<MotionManager>instance_;
	std::unordered_map<std::string, MotionData> motions_;
	std::unordered_map<std::string, int> debugWalkNodeForMotion_;
};

export class MotionEditor {
public:
	static MotionEditor* GetInstance() {
		if (instance_ == nullptr) {
			instance_ = std::make_unique<MotionEditor>();
		}
		return instance_.get();
	}
public:
	void NodeImGui();
private:
	void SaveNode(const std::string& filename, const std::vector<MathUtils::Spline::Node<Vector3>>& nodes);
private:
	static std::unique_ptr<MotionEditor>instance_;
	int draggedNodeIndex = -1;
	int draggedHandleType = 0; // 0:Position, 1:TangentIn, 2:TangentOut
	std::vector<MathUtils::Spline::Node<Vector3>>nodes_;
	std::string inputNodeName_;
    int selectedNodeIndex_ = -1; // for editor selection (debugging / marking)
};

export class MotionController {
public:
	/// <summary>
	/// motionを再生する準備
	/// </summary>
	/// <param name="motionName">使用するモーションの名前</param>
	/// <param name="startPosition">開始地点</param>
	/// <param name="motionDuration">全体の長さ</param>
	void Play(const std::string& motionName, const Vector3& startPosition, float motionDuration = 1.0f);
	/// <summary>
	/// Motionを再生し、値を返す
	/// </summary>
	/// <param name="deltaTime"> 1 フレームの値</param>
	/// <param name="direction">向いている方向</param>
	/// <returns></returns>
	Vector3 Update(float deltaTime, const Vector3& direction);
    // Node event callback: motionName, nodeIndex, nodeName (may be empty), isEntering
    using NodeEventCallback = std::function<void(const std::string&, int, const std::string&, bool)>;

    // Set callback invoked when node enter/exit (behavior depends on interval mode)
    void SetNodeEventCallback(NodeEventCallback cb) { nodeEventCallback_ = std::move(cb); }

    // Optional mapping from node index -> node name (used to identify nodes by name)
    void SetNodeNames(const std::vector<std::string>& names) { nodeNames_ = names; }

    // When true, callback will be invoked with isEntering=true when entering a node interval
    // and isEntering=false when leaving it. When false, callback is invoked only once when
    // the active node changes (one-shot trigger).
    void SetUseIntervalMode(bool enable) { useIntervalMode_ = enable; }
private:
	std::string currentMotionName_;// 再生中のモーションの名前
	float motionTimer_ = 0.0f;// モーションのタイマー
	float motionDuration_ = 1.0f; // モーションの総再生時間（秒）
	bool isPlaying_ = false;// 再生中かどうか
	Vector3 actionStartPosition_;// モーション開始時の座標(相対的に動かすため)

    // Node event support
    NodeEventCallback nodeEventCallback_{};
    std::vector<std::string> nodeNames_{};
    bool useIntervalMode_ = true;
    int prevActiveNodeIndex_ = -1;

	///////////////////////////////////
	///
	///  Get や Set 関係
	///
	///////////////////////////////////
public:
	float GetMotionDuration()const { return motionDuration_; }
	float GetCurrentTime()const { return motionTimer_; }
	/// <summary>
	/// 再生中かどうか
	/// </summary>
	/// <returns></returns>
	bool IsPlaying()const { return isPlaying_; }
    int GetActiveNodeIndex() const { return prevActiveNodeIndex_; }
};
