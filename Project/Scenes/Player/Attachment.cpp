module Game.Attachment;

import Game.MathUtils;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
}

//////////////////////
///
///   コンストラクタ・デストラクタ
/// 
//////////////////////
Attachment::Attachment() {
	matWorld_ = std::make_unique<Matrix4x4>();
}

Attachment::~Attachment() {

}
//////////////////////
///
///   更新処理
/// 
//////////////////////
void Attachment::Update() {

	//Matrix4x4 scaleMat = Game::MathUtils::Scale({1.0f, 1.0f, 1.0f});
	//Matrix4x4 rotationMat = Game::MathUtils::RotateEulerXYZ(rotation_);
	//Matrix4x4 translateMat = Game::MathUtils::Translate(position_);

	// S * R * T
	// スケールは使わないからscaleMatは不要
	//matWorld_ = Matrix4x4::Multiply(scaleMat, rotationMat);
	//Matrix4x4::Multiply(*matWorld_, rotationMat, translateMat);
	
	*matWorld_ = Game::MathUtils::SRT({ 1.0f, 1.0f, 1.0f }, rotation_, position_);
	//(*matWorld_)[3].Set(2, 0.0f);
	// 親子関係なら
	if (parent_) {
		Matrix4x4 aWholeNewWorld;
		Matrix4x4::Multiply(aWholeNewWorld, *matWorld_, parent_->GetMatrix());
		*matWorld_ = aWholeNewWorld;
		//(*matWorld_)[3].Set(2, 0.0f);
	}
}
//////////////////////
///
///   接続処理
/// 
//////////////////////
void Attachment::AttachTo(Attachment* parentJoint) {
	// アタッチを許可されているか確認する -> 不許可なら終了
	if ((acceptType_ & parentJoint->GetType()) == 0)return;
	// 接続処理
	parent_ = parentJoint;
	attached_ = true;
}
//////////////////////
///
///   解除処理
/// 
//////////////////////
void Attachment::Detach() {
	// 解除処理
	parent_ = nullptr;
	attached_ = false;
	// ローカル座標なのでワールドに変換する
	position_ = GetWorldPos();
}