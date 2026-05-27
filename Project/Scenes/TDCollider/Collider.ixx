module;

#include <functional>

export module Collider;

import <vector>;
import <memory>;

import Lumina.Core.Math;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
}

export struct AABB {
	Vector3 Min;
	Vector3 Max;
};

export auto IsCollided(AABB const& lhs_, AABB const& rhs_) -> bool {
	return (
		(lhs_.Min.X <= rhs_.Max.X) &&
		(lhs_.Max.X >= rhs_.Min.X) &&
		(lhs_.Min.Y <= rhs_.Max.Y) &&
		(lhs_.Max.Y >= rhs_.Min.Y) &&
		(lhs_.Min.Z <= rhs_.Max.Z) &&
		(lhs_.Max.Z >= rhs_.Min.Z)
	);
}

export enum COLLISIONATTRIBUTE : int{
	COL_None = 0,
	COL_Player = 1 << 0,
	COL_Enemy = 1 << 1,
	COL_Player_Attack = 1 << 2,
	COL_Enemy_Attack = 1 << 3,
	COL_Ground = 1 << 4,
	COL_Umbrella_Ground = 1 << 5,
	COL_Player_Attack_Smash = 1 << 6,
	COL_Player_Attack_SmashWave = 1 << 7,
	COL_Warp = 1 << 8,
};

export enum class ColliderShape {
	Sphere,
	AABB,
	Convex // GJK用
};

export class Collider
{
public:
	Collider() = default;
	virtual ~Collider() = default;
public:
	using CollisionCallback = std::function<void(Collider*, const Vector3&)>;
	CollisionCallback onCollisionCallback = nullptr;

	virtual void OnCollision(Collider* other, const Vector3& pushOut) {
		if (onCollisionCallback) {
			onCollisionCallback(other, pushOut);
		}
	}

	// --- 各形状に合わせてAABBを再計算する純粋仮想関数 ---
	virtual void UpdateAABB() = 0;

	// --- 形状判定用 ---
	virtual ColliderShape GetShapeType() const = 0;
public:
	/////////////////////////////////
	/// 
	///   Get・Set
	///
	/////////////////////////////////

	Vector3 const& GetWorldPosition() const noexcept { return worldPosition_; }
	void SetWorldPosition(const Vector3& pos) { worldPosition_ = pos; }

	AABB const& GetAABB() const noexcept { return aabb_; }
	void SetAABB(const AABB& aabb) { aabb_ = aabb; }

	uint32_t GetMyType() const noexcept { return collisionAttribute_; }
	void SetMyType(uint32_t type) { collisionAttribute_ = type; }
	
	uint32_t GetYourType() const noexcept { return collisionMask_; }
	void SetYourType(uint32_t type) { collisionMask_ = type; }
protected:
	// AABB
	AABB aabb_;

	Vector3 worldPosition_ = { 0.0f,0.0f,0.0f };

	uint32_t collisionAttribute_ = 0xffffffff;
	uint32_t collisionMask_ = 0xffffffff;
public:
	// 持ち主（PlayerやEnemyなど）を登録・取得するための関数
	void SetUserData(void* userData) { userData_ = userData; }
	void* GetUserData() const { return userData_; }

private:
	void* userData_ = nullptr; // 持ち主のポインタを保存
/////////////////////////////////
/// 
///   当たり判定の保存に関する機能
///
/////////////////////////////////
public:
	// 履歴をリセットする関数（時間経過やアニメーションで呼ぶ）
	void ClearHitHistory() { hitHistory_.clear(); }
	void SetEnableHitHistory(bool enable) { enableHitHistory = enable; }
	// 相手が自分の履歴にいるか確認する
	bool HaveWeCollisionBefore(Collider* other) const {
		if (!enableHitHistory) return false;
		return std::find(hitHistory_.begin(), hitHistory_.end(), other) != hitHistory_.end();
	}

	bool IsHitHistory()const{return !hitHistory_.empty();}
	// 相手を履歴に追加する
	void AddToHistory(Collider* other) {
		if (enableHitHistory) {
			hitHistory_.push_back(other);
		}
	}
private:
	bool enableHitHistory = false; // デフォルトはOFF（体や壁用）
	std::vector<Collider*> hitHistory_;
};

export class ConvexCollider : public Collider
{
public:
	ConvexCollider() {
		worldMatrix_ = std::make_unique<Matrix4x4>();
		*worldMatrix_ = Matrix4x4::Identity;
	}
	virtual ~ConvexCollider() {}

public:
	ColliderShape GetShapeType() const override { return ColliderShape::Convex; }

	// GJKに必要な頂点データ
	void SetVertices(std::vector<Vector3> const& vertices) { vertices_ = vertices; }
	std::vector<Vector3> const& GetVertices() const { return vertices_; }
	void ClearVertices() { vertices_.clear(); }

	// 自身の頂点群からAABBを計算して更新する
	void UpdateAABB() override;
public:
	// PositionではなくMatrixを持たせる
	void SetWorldMatrix(Matrix4x4 const& mat) { *worldMatrix_ = mat; }
	Matrix4x4 const& GetWorldMatrix() const { return *worldMatrix_; }

private:
	std::vector<Vector3> vertices_;
	std::unique_ptr<Matrix4x4> worldMatrix_;
};

