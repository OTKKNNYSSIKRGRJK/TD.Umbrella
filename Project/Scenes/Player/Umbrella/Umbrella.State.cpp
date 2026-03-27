module Game.Umbrella : State;

import : Main;

import Lumina.Core.Math;

namespace {
	using Umbrella::Top;
}

namespace UmbrellaStates {
	/////////////////////////
	/// 
	///  Close
	///
	/////////////////////////
	void Close::Enter() {
		// アニメーション開始
	}
	void Close::Update([[maybe_unused]] float deltaTime) {
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
	void Open::Update([[maybe_unused]] float deltaTime) {
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
	void Reverse::Update([[maybe_unused]] float deltaTime) {
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
	void Attached::Update([[maybe_unused]] float deltaTime) {
		
	}
	void Attached::Exit() {

	}

	/////////////////////////
	/// 
	///  NormalAttack
	///
	/////////////////////////
	void NormalAttack::Enter() {
		// 傘の当たり判定をON
		top_->EnableAttackCollision();
	}

	void NormalAttack::Update([[maybe_unused]] float deltaTime) {
		
		//// モーション（振り）が終わったら、自動的に「いつもの手持ち状態」に戻る！
		//if (!motion_.IsPlaying()) {
		//	top_->ChangeState(new UmbrellaStates::Attached());
		//}
	}

	void NormalAttack::Exit() {
		// 状態が終わる時（Attachedに戻る瞬間）に、当たり判定をOFFにする
		top_->DisableAttackCollision();
	}

	//////////////////////////
	///
	///   Thrown
	///
	//////////////////////////
	void Thrown::Enter() {
		using Vector3 = Lumina::Math::F32x3;
		using Matrix4x4 = Lumina::Math::F32x4x4<>;
		Matrix4x4 const& mat{ top_->GetRootJoint()->GetMatrix() };
		auto const& matRow3{ mat[3] };
		Vector3 startPos = { matRow3.Get(0), matRow3.Get(1), matRow3.Get(2) };
		motion_.Play("", startPos, 1.0f);
	}
	void Thrown::Update([[maybe_unused]] float deltaTime) {

	}
	void Thrown::Exit() {

	}
}