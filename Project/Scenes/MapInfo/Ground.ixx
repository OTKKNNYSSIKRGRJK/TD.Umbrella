export module Ground;

import <memory>;
import <vector>;

import Collider;

import Lumina.Core.Math;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
}

class Ground
{
public:
	void Initialize();
	void Update();
	void Draw();

	//////////////////////////
	///
	///   当たり判定
	///
	//////////////////////////
public:
	Collider* GetCollider() const& { return collider_.get(); }
	void SetVertices(std::vector<Vector3> const& vertices){ collider_->SetVertices(vertices); }
private:
	std::unique_ptr<ConvexCollider> collider_;

private:
	//std::unique_ptr<ModelObject>obj_ = nullptr;
};