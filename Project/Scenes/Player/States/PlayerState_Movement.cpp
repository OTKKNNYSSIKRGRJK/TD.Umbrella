module Game.Player;

import : States;
import : Main;
import Game.Umbrella;
import Lumina.Core.Math;
import Game.MathUtils;
import Game.Events;
import Game.Events.InGame;
import Lumina.Main;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;

	Player* Dummy_;
}

namespace PlayerStates::Movement {
	////////////////////////////
	//
	//  Grounded
	// 
	////////////////////////////
	void Grounded::Update(float deltaTime) {
		//// 地上にいる際の処理 ////

		// もし接地していない（崖から落ちた）なら、強制的に空中ステートへ
		if (!player_->onGround_ && player_->jumpCoyoteTimer_ >= player_->JUMP_COYOTE_MAX_TIME) {
			player_->ChangeMovementState(player_->airborneState_.get());
			return;
		}

		// 地上の摩擦係数(大きいほどすぐ止まる)
		float groundFriction = 10.0f;

		// 外部からの速度の減衰関係の処理
		// ※ 自発的な速度はIdleで減衰させる(Walk中はしなくていいため)
		player_->externalVelocity_.X = std::lerp(
			player_->externalVelocity_.X,
			0.0f,
			groundFriction * deltaTime
		);
		//player_->externalVelocity_.Z = std::lerp(
		//	player_->externalVelocity_.Z,
		//	0.0f,
		//	groundFriction * deltaTime
		//);

		// 重力の処理(地面にいるからする意味はないが、一応する)
		// ※ 関数でまとめておく

		// Y軸の速度
		player_->externalVelocity_.Y -= 9.8f * deltaTime * 4.5f; // 通常の重力

		// ジャンプ
		player_->Jump();
	}
	////////////////////////////
	//
	//  Airborne
	// 
	////////////////////////////
	void Airborne::Enter() {
		if (player_->GetUmbrella().top_->GetUmbrellaForm() == UmbrellaForm::Opened) {
			Game::Event::InGame::OnPlayerFlying event_OnPlayerFlying{};
			Lumina::Context::Instance().EventContext().TriggerEvent(std::move(event_OnPlayerFlying));
		}
	}

	void Airborne::Update(float deltaTime) {
		float gravity = 40.0f; // 重力加速度

		// --------------------------------------------------------
		// 1. 空中制御のパラメータ
		// --------------------------------------------------------
		float airMaxSpeed = 9.0f;       // 空中での最高速度（抜刀時などと同じくらい）
		float airAcceleration = 4.0f;   // ★ココが重要！地上が15.0fなら、かなり小さくする

		// --------------------------------------------------------
		// 2. 目標速度の計算とLerp
		// --------------------------------------------------------
		float targetVelocityX = player_->GetInput().moveDirection.X * airMaxSpeed;

		// 現在の速度から、目標速度に向けてゆっくり加速（減速）させる
		player_->myVelocity_.X = std::lerp(
			player_->myVelocity_.X,
			targetVelocityX,
			airAcceleration * deltaTime
		);

		if (player_->GetUmbrella().top_->GetUmbrellaForm() == UmbrellaForm::Opened) {
			// 傘を開いている間は、空中での制御をさらに弱くする（ふわっとさせる）
			airAcceleration = 2.0f; // さらに小さくする
			if (player_->externalVelocity_.Y < 0.0f) {
				// 落ちるときにふわふわする
				gravity = 10.0f; // 落ちるのも遅くする
			}
		}

		if (player_->GetCurrentActionState() == player_->reverseChargeState_.get()) {
			// チャージ量によって落ちるのを速くする
			gravity = 25.2f + 15.0f * (player_->GetUmbrella().top_->GetManaComponent().GetCurrentMana() / (player_->GetUmbrella().top_->GetManaComponent().GetMaxMana() * 0.4f));
		}

		// Y軸には常に重力をかけ続ける
		player_->externalVelocity_.Y -= gravity * deltaTime;

		if (player_->onGround_ == true) {

			// 落下速度をリセット
			player_->externalVelocity_.Y = 0.0f;

			// 地上に着いたので Idle ステートに戻す！
			player_->ChangeMovementState(player_->idleState_.get());
		}

		player_->Jump();// コヨーテタイムのため

		const auto& input = player_->GetInput();
		if (input.debugRevive) {
			player_->ChangeMovementState(player_->idleState_.get());
			player_->ChangeActionState(player_->normalDrawnState_.get());
			player_->SetPosition(Game::Event::RespawnPos);
			player_->GetStatusComponent().Heal(100.0f);
		}

		// アニメーションの制御
		if(player_->GetUmbrella().top_->GetUmbrellaForm() == UmbrellaForm::Opened) {
			if (player_->GetCurrentAnimationName() != "Swinging") {
				player_->PlayAnimation("Swinging", true);
			}
		}

	}

	/*void Airborne::Exit() {

	}*/
	////////////////////////////
	//
	//  Idle
	// 
	////////////////////////////
	void Idle::Enter() {
		if(player_->GetUmbrella().top_->GetUmbrellaForm() == UmbrellaForm::Opened || player_->GetUmbrella().top_->GetUmbrellaForm() == UmbrellaForm::Reverse) {
			player_->PlayAnimation("IdleOpen", true);
		} else {
			player_->PlayAnimation("Idle", true);
		}
	}

	void Idle::Update(float deltaTime) {
		if (parentState_) {
			parentState_->Update(deltaTime);
		}

		if (player_->GetUmbrella().top_->GetUmbrellaForm() == UmbrellaForm::Opened) {
			if (player_->GetCurrentAnimationName() != "IdleOpen") {
				player_->PlayAnimation("IdleOpen", true);
			}
		}
		else if (player_->GetUmbrella().top_->GetUmbrellaForm() == UmbrellaForm::Closed) {
			if (player_->GetCurrentAnimationName() != "Idle") {
				player_->PlayAnimation("Idle", true);
			}
		}

		// 【追加】Grounded内でジャンプして空中に移行したなら、以降の処理をキャンセル！
		if (!player_->onGround_) return;

		const auto& input = player_->GetInput();

		// 入力方向がゼロじゃない（スティックが倒された）なら、Walkingへ遷移
		if (input.moveDirection.X != 0.0f || input.moveDirection.Z != 0.0f) {
			player_->ChangeMovementState(player_->walkingState_.get());
			return; // 遷移したらこのフレームの処理は終了
		}

		// スティックが倒されていないなら、自発的な速度(myVelocity_)を摩擦でゼロに近づける
		float deceleration = 15.0f; // ブレーキの強さ
		player_->myVelocity_.X = std::lerp(player_->myVelocity_.X, 0.0f, deceleration * deltaTime);
		player_->myVelocity_.Y = std::lerp(player_->myVelocity_.Y, 0.0f, deceleration * deltaTime);
		//player_->myVelocity_.Z = std::lerp(player_->myVelocity_.Z, 0.0f, deceleration * deltaTime);

		if (input.debugRevive) {
			player_->ChangeMovementState(player_->idleState_.get());
			player_->ChangeActionState(player_->normalDrawnState_.get());
			player_->SetPosition(Game::Event::RespawnPos);
			player_->GetStatusComponent().Heal(100.0f);
		}
	}

	void Idle::Exit() {

	}
	////////////////////////////
	//
	//  Walking
	// 
	////////////////////////////
	void Walking::Enter() {
		player_->PlayAnimation("Run",true);
	}

	void Walking::Update(float deltaTime) {
		if (parentState_) {
			parentState_->Update(deltaTime);
		}

		const auto& input = player_->GetInput();

		// スティックが離されたら、Idleへ遷移！
		if (input.moveDirection.X == 0.0f && input.moveDirection.Z == 0.0f) {
			player_->ChangeMovementState(player_->idleState_.get());
			return;
		}

		float targetSpeed = 0.0f;

		// 武器の状態で切り替え
		switch (player_->GetWeaponStance()) {
		case WeaponStance::Sheathed:
			targetSpeed = 17.0f;
			break;
		case WeaponStance::Drawn:
			targetSpeed = 15.0f;
			break;
		}

		if (player_->GetCurrentActionState() == player_->reverseChargeState_.get()) {
			targetSpeed = 8.0f - 8.0f * (player_->GetUmbrella().top_->GetManaComponent().GetCurrentMana() / (player_->GetUmbrella().top_->GetManaComponent().GetMaxMana()));
		}
		else if (player_->GetCurrentActionState() == player_->guardState_.get()) {
			targetSpeed = 5.0f;
		}

		float acceleration = 15.0f;
		float targetVelocityX = input.moveDirection.X * targetSpeed;
		//float targetVelocityZ = player_->moveDirection_.Z * targetSpeed;//今回はいらない

		// 目標速度に加速させる
		player_->myVelocity_.X = std::lerp(player_->myVelocity_.X, targetVelocityX, acceleration * deltaTime);
		//player_->myVelocity_.Z = std::lerp(player_->myVelocity_.Z, targetVelocityZ, acceleration * deltaTime);

		// もし、トップスピードになったらRunningに移行
		// ※もしキャラクターのモデルの向きを移動方向に合わせるなら、ここに書く
	}

	void Walking::Exit() {

	}
	////////////////////////////
	//
	//  Restricted
	// 
	////////////////////////////
	void Restricted::Enter() {

	}

	void Restricted::Update(float deltaTime) {
		// 親（Grounded）のUpdateを呼び、重力や外部速度（ノックバック等）の減衰処理
		if (parentState_) {
			parentState_->Update(deltaTime);
		}

		// ！！！ スティック入力は一切見ない（移動させない） ！！！

		// 攻撃などで付与された myVelocity_（踏み込み速度）を摩擦で減衰させる
		float deceleration = 15.0f; // ※ここの値が踏み込みの「滑り具合」
		player_->myVelocity_.X = std::lerp(player_->myVelocity_.X, 0.0f, deceleration * deltaTime);
		//player_->myVelocity_.Z = std::lerp(player_->myVelocity_.Z, 0.0f, deceleration * deltaTime);
	}

	void Restricted::Exit() {

	}
}