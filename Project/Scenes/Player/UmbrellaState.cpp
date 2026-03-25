#include "Umbrella.h"
#include "UmbrellaState.h"

namespace UmbrellaStates {
	/////////////////////////
	/// 
	///  Close
	///
	/////////////////////////
	void Close::Enter() {
		// アニメーション開始
	}
	void Close::Update(float deltaTime) {
		if (true/*アニメーションが終わったら*/) {
			top_->ChangeState(new UmbrellaStates::Attached());
		}
	}
	void Close::Exit() {
		top_->ChangeForm(UmbrellaForm::Closed);
	}
	/////////////////////////
	/// 
	///  Open
	///
	/////////////////////////
	void Open::Enter() {
		// アニメーション開始
	}
	void Open::Update(float deltaTime) {
		if (true/*アニメーションが終わったら*/) {
			top_->ChangeState(new UmbrellaStates::Attached());
		}
	}
	void Open::Exit() {
		top_->ChangeForm(UmbrellaForm::Opened);
	}
	/////////////////////////
	/// 
	///  Reverse
	///
	/////////////////////////
	void Reverse::Enter() {
		// アニメーション開始
	}
	void Reverse::Update(float deltaTime) {
		if (true/*アニメーションが終わったら*/) {
			top_->ChangeState(new Attached());
		}
	}
	void Reverse::Exit() {
		top_->ChangeForm(UmbrellaForm::Reverse);
	}
	/////////////////////////
	/// 
	///  Attached
	///
	/////////////////////////
	void Attached::Enter() {

	}
	void Attached::Update(float deltaTime) {
		
	}
	void Attached::Exit() {

	}

	/////////////////////////
	/// 
	///  NormalAttack
	///
	/////////////////////////
	void NormalAttack::Enter() {
		// ① 攻撃の軌道（Hermite曲線）を再生！
		// ※ 例えば、振りかぶってから振り下ろす軌道を "Swing" として作っておく
		//Vector3 startPos = { top_->GetRootJoint()->GetMatrix().m[3][0],top_->GetRootJoint()->GetMatrix().m[3][1] ,top_->GetRootJoint()->GetMatrix().m[3][2] };
		///motion_.Play("Swing", startPos, 0.4f); // 0.4秒で素早く振る！

		// 傘の当たり判定をON
		// top_->SetCollisionEnabled(true);
	}

	void NormalAttack::Update(float deltaTime) {
		
		// モーション（振り）が終わったら、自動的に「いつもの手持ち状態」に戻る！
		if (!motion_.IsPlaying()) {
			top_->ChangeState(new UmbrellaStates::Attached());
		}
	}

	void NormalAttack::Exit() {
		// 状態が終わる時（Attachedに戻る瞬間）に、当たり判定をOFFにする
		// top_->SetCollisionEnabled(false);
	}

	//////////////////////////
	///
	///   Thrown
	///
	//////////////////////////
	void Thrown::Enter() {
		Vector3 startPos = { top_->GetRootJoint()->GetMatrix().m[3][0],top_->GetRootJoint()->GetMatrix().m[3][1] ,top_->GetRootJoint()->GetMatrix().m[3][2] };
		motion_.Play("", startPos, 1.0f);
	}
	void Thrown::Update(float deltaTime) {

	}
	void Thrown::Exit() {

	}
}