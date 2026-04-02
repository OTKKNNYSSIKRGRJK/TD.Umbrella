module Game.Umbrella : State;

import : Main;

import Lumina.Core.Math;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
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
		// 状態の設定
		top_->ChangeForm(UmbrellaForm::Reverse);
		// 防御力を低く設定
		top_->GetStatusComponent().SetDefense(1.0f);
	}
	/////////////////////////
	/// 
	///  Broken
	///
	/////////////////////////
	void Broken::Enter() {
		// アニメーション開始

	}
	void Broken::Update(float deltaTime) {
		deltaTime;
	}
	void Broken::Exit() {

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

	/////////////////////////
	/// 
	///  Flying
	///
	/////////////////////////
	void Flying::Enter() {
		// 親（持ち手）から自分（かさ）を切り離す！
		// ※ Topクラスが持っているJoint（アタッチメント）をDetachする処理
		top_->GetRootJoint()->Detach();

		top_->ChangeForm(UmbrellaForm::Flying); // 飛んでいる間は「かさがない状態」にする

		// 足場用の当たり判定（コライダー属性）をONにするなどの処理
		top_->GetCollider()->SetMyType(COL_Umbrella_Ground);
	}

	void Flying::Update([[maybe_unused]] float deltaTime) {
		// ③ 座標を更新して飛ばす
		Vector3 pos = top_->GetRootJoint()->GetPos();
		pos.X += velocity_.X * deltaTime * 8.0f;
		pos.Y += velocity_.Y * deltaTime * 8.0f;
		pos.Z += velocity_.Z * deltaTime * 8.0f;
		top_->GetRootJoint()->SetPos(pos);
		// Velocityをだんだん減速させる
		float deceleration = 3.5f; // ブレーキの強さ
		velocity_.X = std::lerp(velocity_.X, 0.0f, deceleration * deltaTime);
		velocity_.Y = std::lerp(velocity_.Y, 0.0f, deceleration * deltaTime);

		// 速度がゼロになったら飛ぶのを終了する
		if (std::abs(velocity_.X) <= 0.1f && std::abs(velocity_.Y) <= 0.1f) {
			top_->ChangeState(new Stationary());
		}
	}

	void Flying::Exit() {
		// 飛び終わった時の処理
	}

	// ==========================================
	//   Stationary (静止・足場状態)
	// ==========================================
	void Stationary::Enter() {
		top_->UpdateColliderShape();
		top_->ChangeForm(UmbrellaForm::AirStop); // 飛び終わったら、開いた傘の状態にする
	}

	void Stationary::Update([[maybe_unused]] float deltaTime) {
		// ここに留まり続ける。
		// もしプレイヤーが「回収ボタン」を押したら、手元に戻るステートへ移行など
		if (top_->IsRecalling()) {
			// 1. プレイヤーへの方向ベクトルを計算
			Vector3 playerPos = top_->GetPlayerPos();
			Vector3 umbrellaPos = top_->GetRootJoint()->GetPos();
			Vector3 toPlayer = playerPos - umbrellaPos;

			// 2. 距離を測っておく（回収判定用）
			float distance = sqrt(toPlayer.X * toPlayer.X + toPlayer.Y * toPlayer.Y);

			if (distance > 1.0f) { // まだ離れている場合
				// 3. 正規化して一定の速度で移動させる
				Vector3 direction = { toPlayer.X / distance, toPlayer.Y / distance, 0.0f };
				float returnSpeed = 20.0f; // 戻るスピード（投げた時より速いと気持ちいい）

				// 速度を適用
				umbrellaPos.X += direction.X * returnSpeed * deltaTime;
				umbrellaPos.Y += direction.Y * returnSpeed * deltaTime;

				top_->GetRootJoint()->SetPos(umbrellaPos);
			}

			top_->GetCollider()->SetMyType(COL_None);
		}
		else {
			top_->GetCollider()->SetMyType(COL_Umbrella_Ground);
		}
	}

	void Stationary::Exit() {
		// 足場判定をOFFにするなど
		top_->GetCollider()->SetMyType(COL_None);
	}
}