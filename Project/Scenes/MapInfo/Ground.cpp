#include "Ground.h"

void Ground::Initialize(Fngine* fngine) {
	obj_ = std::make_unique<ModelObject>();
	obj_->modelName_ = "ground";
	obj_->textureName_ = "Legends_Ground";
	obj_->Initialize(fngine);

	// =============================
	// 【 当たり判定についての処理 】
	// =============================
	collider_ = std::make_unique<ConvexCollider>();

	// 属性の設定（自分は地面、当たる相手はプレイヤーや敵）
	collider_->SetMyType(COL_Ground);
	collider_->SetYourType(COL_Player | COL_Enemy);

	// 地面の頂点データ（横幅100、高さ1の長方形）
	// GJKが2D計算なので、XY平面上に作ります
	std::vector<Vector3> groundVertices = {
		{-50.0f, -5.0f, 0.0f}, // 左下
		{ 50.0f, -5.0f, 0.0f}, // 右下
		{-50.0f,  0.0f, 0.0f}, // 左上
		{ 50.0f,  0.0f, 0.0f}  // 右上
	};
	collider_->SetVertices(groundVertices);

	// 地面の位置（原点の少し下あたりに配置）
	collider_->SetWorldPosition({ 0.0f, 2.0f, 0.0f });

	// 持ち主を登録
	collider_->SetUserData(this);

	// 当たった時のテスト処理
	collider_->onCollisionCallback = [this](Collider* other, const Vector3& pushOut) {
		if (other->GetMyType() == COL_Player) {
			// プレイヤーが地面に触れたらログを出す（確認用）
			// printf("プレイヤーが地面に着地！\n");
		}
	};
}

void Ground::Update() {
	collider_->SetWorldPosition(obj_->worldTransform_.get_.Translation());
	obj_->LocalToWorld();
	collider_->SetWorldMatrix(obj_->worldTransform_.mat_);
	collider_->UpdateAABB();
}

void Ground::Draw() {
	obj_->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(obj_->worldTransform_.mat_));
	obj_->Draw();
}