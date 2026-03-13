export module MotionManager;

import <string>;
import <memory>;
import <unordered_map>;
import <vector>;
import Hermite;
import Lumina.Core.Math;

using namespace Lumina::Math;
using MotionData = std::vector<MathUtils::Spline::Node<F32x3>>;

export class MotionManager {
public:
	static MotionManager* GetInstance() {
		if (instance_ == nullptr) {
			instance_ = std::make_unique<MotionManager>();
		}
		return instance_.get();
	}
public:
	void LoadActionData(const std::string& fileName, std::vector<MathUtils::Spline::Node<F32x3>>& outNodes);
	void LoadMotions(const std::string& directoryPath);
	const MotionData& GetMotion(const std::string& name) const;
private:
	static std::unique_ptr<MotionManager>instance_;
	std::unordered_map<std::string, MotionData> motions_;
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
	void SaveNode(const std::string& filename, const std::vector<MathUtils::Spline::Node<F32x3>>& nodes);
private:
	static std::unique_ptr<MotionEditor>instance_;
	int draggedNodeIndex = -1;
	int draggedHandleType = 0; // 0:Position, 1:TangentIn, 2:TangentOut
	std::vector<MathUtils::Spline::Node<F32x3>>nodes_;
	std::string inputNodeName_;
};

export class MotionController {
public:
	/// <summary>
	/// motionを再生する準備
	/// </summary>
	/// <param name="motionName">使用するモーションの名前</param>
	/// <param name="startPosition">開始地点</param>
	/// <param name="motionDuration">全体の長さ</param>
	void Play(const std::string& motionName, const F32x3& startPosition, float motionDuration = 1.0f);
	/// <summary>
	/// Motionを再生し、値を返す
	/// </summary>
	/// <param name="deltaTime"> 1 フレームの値</param>
	/// <param name="direction">向いている方向</param>
	/// <returns></returns>
	F32x3 Update(float deltaTime, const F32x3& direction);
	/// <summary>
	/// 生成中かどうか
	/// </summary>
	/// <returns></returns>
	bool IsPlaying()const { return isPlaying_; }
private:
	std::string currentMotionName_;// 再生中のモーションの名前
	float motionTimer_ = 0.0f;// モーションのタイマー
	float motionDuration_ = 1.0f; // モーションの総再生時間（秒）
	bool isPlaying_ = false;// 再生中かどうか
	F32x3 actionStartPosition_;// モーション開始時の座標(相対的に動かすため)
};