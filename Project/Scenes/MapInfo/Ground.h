#pragma once
#include "../TDCollider/Collider.h"
#include <memory>

class Ground
{
public:
	void Initialize(Fngine* fngine);
	void Update();
	void Draw();

	//////////////////////////
	///
	///   当たり判定
	///
	//////////////////////////
public:
	Collider* GetCollider()const { return collider_.get(); }
	void SetVertices(const std::vector<Vector3>vertices){ collider_->SetVertices(vertices); }
private:
	std::unique_ptr<ConvexCollider> collider_;

private:
	std::unique_ptr<ModelObject>obj_ = nullptr;
};