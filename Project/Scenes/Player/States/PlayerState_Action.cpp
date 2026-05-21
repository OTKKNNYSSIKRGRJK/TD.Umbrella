module Game.Player;

import : States;
import : Main;
import Game.Umbrella;
import Lumina.Core.Math;
import Game.MathUtils;
import Game.Events;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;

	// これがなくてもビルド全然通るけど、ないとIntellisenseくんが発狂しちゃう
	// "pointer or reference to incomplete type 'Player' is not allowed"っつって
	Player* Dummy_;
}

namespace PlayerStates::Action {
	////////////////////////////
	//
	//  Dead
	// 
	////////////////////////////
	void Dead::Enter() {
	}

	void Dead::Update([[maybe_unused]] float deltaTime) {
		// 死亡中はなにもできない

		auto const& input{ player_->GetInput() };
		if (input.debugRevive) {
			player_->ChangeMovementState(player_->idleState_.get());
			player_->ChangeActionState(player_->normalDrawnState_.get());
			player_->SetPosition(Game::Event::RespawnPos);
			player_->GetStatusComponent().Heal(100.0f);
		}
	}

	void Dead::Exit() {
	}

	////////////////////////////
	//
	//  NormalSheathed
	// 
	////////////////////////////
	void NormalSheathed::Enter() {

	}

	void NormalSheathed::Update([[maybe_unused]] float deltaTime) {
		const auto& input = player_->GetInput();
		const auto& umbrella = player_->GetUmbrella();
		const auto umbrellaForm = umbrella.top_->GetUmbrellaForm();

		// 【 RightJointの位置決め 】
		//Vector3 handPos = player_->GetPosition();
		//handPos.X += 1.0f * player_->eyesDirection_.X; // プレイヤーの右方向へオフセット
		//handPos.Y += 1.0f; // 少し上へ
		//player_->GetRightHandJoint()->SetPos(handPos);

		if (input.attack == ButtonState::Pressed) {
			// 納刀状態なら攻撃をする前に抜刀するようにする
				// この時、攻撃をする意思があることを伝えるようにする
				// ChangeState -> DrawWeapon -> Attack
			player_->ChangeActionState(player_->drawWeaponState_.get());
			return;
		}

		if (input.sheathe == ButtonState::Pressed) {
			
		}

		if (input.guard == ButtonState::Pressed) {
			
		}

		if (input.reverse == ButtonState::Pressed) {
			
		}

		if (input.repair == ButtonState::Pressed) {
			// 耐久値が減っている時（MAXじゃない時）だけ修理できるようにする
			if (umbrella.top_->GetStatusComponent().GetHp() < umbrella.top_->GetStatusComponent().GetMaxHp()) {
				player_->ChangeActionState(player_->repairUmbrellaState_.get());
				return;
			}
			
		}

		if (input.shoot == ButtonState::Pressed) {
			if (umbrellaForm == UmbrellaForm::Flying) {
				//player_->WarpToUmbrella();
			}
		}
	}

	void NormalSheathed::Exit() {

	}

	///////////////////////
	///
	/// NormalDrawn
	///
	///////////////////////
	void NormalDrawn::Enter() {

	}

	void NormalDrawn::Update([[maybe_unused]] float deltaTime) {
		const auto& input = player_->GetInput();
		const auto& umbrella = player_->GetUmbrella();
		const auto umbrellaForm = umbrella.top_->GetUmbrellaForm();

		// 【 RightJointの位置決め 】
		//Vector3 handPos = player_->GetPosition();
		//handPos.X += 1.0f * player_->eyesDirection_.X; // プレイヤーの右方向へオフセット
		//handPos.Y += 1.0f; // 少し上へ
		//player_->GetRightHandJoint()->SetPos(handPos);

		// 特になにもしていない時のState
		if (input.attack == ButtonState::Pressed) {
			if (umbrellaForm == UmbrellaForm::Closed) {
				if (player_->onGround_ == false && player_->jumpCoyoteTimer_ >= player_->JUMP_COYOTE_MAX_TIME) {
					// 空中にいるなら空中コンボにつながる
					player_->attackState_->SetAttackID("AerialCombo1");
					player_->ChangeActionState(player_->attackState_.get());
					return;
				}

				if (std::abs(player_->myVelocity_.X) > 7.0f && (input.moveDirection.X * player_->eyesDirection_.X) > 0.0f) {
					// 少し速めに移動しているかつ、移動方向がプレイヤーの正面方向と同じなら前方攻撃
					player_->attackState_->SetAttackID("DashThrust");
					player_->ChangeActionState(player_->attackState_.get());
					return;
				}

				//Game::Event::OnAttack();
				player_->attackState_->SetAttackID("Close_Y1");
				player_->ChangeActionState(player_->attackState_.get());
				return;
			}
		}
		else if (input.attack == ButtonState::Held) {
			if (umbrellaForm == UmbrellaForm::Reverse) {
				player_->ChangeActionState(player_->reverseChargeState_.get());
				return;
			}
			else if (umbrellaForm == UmbrellaForm::Opened) {
				player_->attackState_->SetAttackID("Open_Y1");
				player_->ChangeActionState(player_->attackState_.get());
				return;
			}
		}

		if (input.sheathe == ButtonState::Pressed) {
			// 抜刀状態なら納刀のStateに遷移出来る
				// ChangeState -> SheatheWeapon
			if (umbrellaForm != UmbrellaForm::Flying && umbrellaForm != UmbrellaForm::AirStop) {
				if (umbrellaForm == UmbrellaForm::Closed) {
					player_->ChangeActionState(player_->sheatheWeaponState_.get());
					return;
				}
				else if (umbrellaForm == UmbrellaForm::Reverse) {
					player_->ChangeActionState(player_->umbrellaOpenState_.get());
					return;
				}
				else {
					player_->ChangeActionState(player_->umbrellaCloseState_.get());
					return;
				}
			}
		}

		if (input.guard == ButtonState::Pressed) {
			// 抜刀状態なので傘は開いているか確認する
			if ((umbrellaForm == UmbrellaForm::Opened) || (umbrellaForm == UmbrellaForm::Reverse)) {
				player_->ChangeActionState(player_->guardState_.get());
				return;
			}
			else {
				if ((umbrellaForm != UmbrellaForm::Flying) && (umbrellaForm != UmbrellaForm::AirStop)) {
					// 傘を開くStateに遷移する。-> ガードを押していたらガードStateに遷移する。
					player_->ChangeActionState(player_->umbrellaOpenState_.get());
					return;
				}
			}
		}

		if (input.reverse == ButtonState::Pressed) {
			// 抜刀状態なので傘は開いているか確認する
			if ((umbrellaForm == UmbrellaForm::Opened)) {
				player_->ChangeActionState(player_->umbrellaReverseState_.get());
				return;
			}
			else {
				// 傘を開くStateに遷移する。-> ガードを押していたらガードStateに遷移する。
				//player_->ChangeActionState(player_->umbrellaOpenState_.get());
			}
		}

		if (input.repair == ButtonState::Pressed) {
			
		}

		if (input.shoot == ButtonState::Pressed) {
			if (umbrellaForm == UmbrellaForm::AirStop) {
				//Game::Event::OnAttack();
				player_->WarpToUmbrella();
				return;
			}
		}
	}

	void NormalDrawn::Exit() {

	}

	////////////////////////////
	//
	//  Attack
	// 
	////////////////////////////
	void Attack::Enter() {
		attackTimer_ = 0.0f;
		currentAttackID_ = nextAttackID_; // セットされた攻撃IDを現在のにする

		// 1. データベースから攻撃データを引っ張ってくる
		const auto& db = player_->GetAttackDataBase();
		if (db.find(currentAttackID_) != db.end()) {
			currentAttackData_ = db.at(currentAttackID_);
		}
		else {
			// もしJSONにデータがなかったら安全のために抜刀状態に戻す
			player_->ChangeActionState(player_->normalDrawnState_.get());
			return;
		}

		// 2. 移動ステートを制限状態に強制変更
		player_->ChangeMovementState(player_->restrictedState_.get());

		// 3. JSONのデータ通りに踏み込みの初速を与える
		Vector3 forward = player_->eyesDirection_;
		float stepPower = currentAttackData_.physics.velocityX;
		player_->myVelocity_.X = forward.X * stepPower;
		player_->myVelocity_.Z = forward.Z * stepPower;

		// もし縦（Y）への移動（浮き上がり等）が設定されていれば適用！
		if (currentAttackData_.physics.velocityY != 0.0f) {
			player_->myVelocity_.Y = currentAttackData_.physics.velocityY;
		}

		player_->PlayAnimation(currentAttackData_.animationName);

		// 4. JSONのデータ通りにモーションと威力をセット
		//motion_.Play(currentAttackData_.motion, { 0.0f,0.0f,0.0f }, currentAttackData_.duration);
		motion_.Play(currentAttackData_.motion, { 0.0f,0.0f,0.0f }, player_->GetAnimationDuration());
		player_->GetUmbrella().top_->GetStatusComponent().SetAttack(currentAttackData_.damage);
		player_->GetUmbrella().top_->GetStatusComponent().SetHitStop(currentAttackData_.feel.hitStop);
		player_->GetUmbrella().top_->GetStatusComponent().IncrementAttackInstanceId();

		// 5. 傘を攻撃状態にする
		player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::NormalAttack());

		player_->GetUmbrella().top_->GetCollider()->ClearHitHistory();
	}

	void Attack::Update(float deltaTime) {
		attackTimer_ += deltaTime;
		if (currentAttackData_.animationName == "AtkX1") {
			attackTimer_ += deltaTime * 2.0f;
		}
		if (currentAttackData_.animationName == "AtkX2") {
			attackTimer_ += deltaTime * 2.0f;
		}
		if (currentAttackData_.animationName == "AtkX3") {
			attackTimer_ += deltaTime * 2.0f;
		}

		const auto& input = player_->GetInput();

		// 【 手の位置の上下シフト 】
		/*Vector3 handPos = player_->GetPosition();
		handPos.X += 1.0f * player_->eyesDirection_.X;
		float shiftAmount = input.moveDirection.Y * 0.5f;
		handPos.Y += 1.0f + shiftAmount;
		player_->GetRightHandJoint()->SetPos(motion_.Update(deltaTime, player_->eyesDirection_) + handPos);*/

		//   ==================
		// 【 特定の攻撃の処理 】
		//   ==================
		if (player_->attackState_->currentAttackID_ == "AerialComboFinal_Down") {
			if (player_->onGround_) {
				Game::Event::OnAttack();
				// 強制終了して着地ステートへ
				player_->ChangeActionState(player_->normalDrawnState_.get());
				player_->ChangeMovementState(player_->idleState_.get());
				player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::Attached());
				return;
			}
		}

		// ===================
		// 【 派生チェック 】
		// ===================
		for (const auto& branch : currentAttackData_.branches) {
			float currentAnimDuration = player_->GetAnimationDuration();
			// 現在のアニメーションの進行度（0.0 ~ 1.0）を計算
			float normalizedTime = attackTimer_ / currentAnimDuration;

			// 現在の時間が、派生可能な時間（timeMin ~ timeMax）に入っているか？
			if (normalizedTime >= branch.timeMin && normalizedTime <= branch.timeMax) {

				bool canBranch = false;

				// 1. 入力タイプが "Input" の場合（ボタンを押したか）
				if (branch.type == "Input") {
					if (branch.input == "AttackY" || branch.input == "Attack") { // "Attack"は旧データ対応用
						if (input.attack == ButtonState::Pressed) {
							canBranch = true;
						}
					}
					// --- Xボタン（強攻撃） ---
					else if (branch.input == "AttackX") {
						if (input.sheathe == ButtonState::Pressed) canBranch = true;
					}
					// --- Xボタン 長押し（チャージ・特殊派生） ---
					else if (branch.input == "AttackX_Hold") {
						// 長押しの場合は Held 状態で判定する
						if (input.sheathe == ButtonState::Held) {
							holdTimerX_ += deltaTime;
							// 0.2秒以上押しっぱなしなら派生成立！
							if (holdTimerX_ > 0.2f) {
								canBranch = true;
							}
						}
					}
					// --- 回避 ---
					else if (branch.input == "Evasion") {
						if (input.evasion == ButtonState::Pressed) canBranch = true;
					}
				}
				// 2. 入力タイプが "Auto" の場合（時間が来たら自動で派生）
				else if (branch.type == "Auto") {
					canBranch = true;
				}

				// 3.マナ条件のチェック（JSONで minMana が設定されている場合）
				if (canBranch && branch.condition.minMana > 0.0f) {
					if (player_->GetManaComponent().GetCurrentMana() < branch.condition.minMana) {
						canBranch = false; // マナが足りないから派生できない！
					}
					// そもそもマナを使おうとしているかチェック
					if (input.useMana == false) {
						canBranch = false;// 使う気がないので派生なし
					}
				}

				// 条件を全てクリアして派生が決定した場合！
				if (canBranch) {
					// マナ消費が設定されていれば消費する
					if (branch.consumeMana > 0.0f) {
						player_->GetManaComponent().ConsumeMana(branch.consumeMana);
					}

					// 次のステートへ移行！
					if (false/*branch.input == "Evasion"*/) {
						// 回避でキャンセルした場合
						//player_->ChangeActionState(player_->evasionState_.get());
					}
					else {
						// 次の攻撃へ！
						SetAttackID(branch.nextAttack);
						Enter();
					}
					return; // 派生したのでUpdateはここで終わり
				}
			}
		}

		// =================================
		// 【 どの派生もせず、モーションの寿命（duration）が終わった時 】
		// =================================
		if (attackTimer_ >= player_->GetAnimationDuration()) {
			player_->ChangeActionState(player_->normalDrawnState_.get());
			player_->ChangeMovementState(player_->idleState_.get());
			player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::Attached());
		}
	}

	void Attack::Exit() {
		// =================================
		// 【 別のステート（回避や被ダメージなど）で強制終了させられた時のためのリセット 】
		// =================================
		
		// 攻撃判定(Collider)をオフにする処理などもここに書く
	}

	////////////////////////////
	// 
	// ThrowUmbrella 
	// 
	////////////////////////////
	void ThrowUmbrella::Enter() {
		// アニメーション再生
	}

	void ThrowUmbrella::Update([[maybe_unused]] float deltaTime) {
		// プレイヤーの手の位置に傘を追従させる
		//Vector3 handPos = player_->GetPosition();
		//handPos.X += 1.0f * player_->eyesDirection_.X; // プレイヤーの右方向へオフセット
		//handPos.Y += 1.0f; // 少し上へ
		//player_->GetRightHandJoint()->SetPos(handPos);

		if (true/*再生が終わったら*/) {
			Vector3 throwVelocity = player_->GetTargetPos() - player_->GetPosition();
			float throwSpeed = 1.0f; // 投げる速度の調整用の定数
			throwVelocity.X *= throwSpeed;
			throwVelocity.Y *= throwSpeed;
			player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::Flying(throwVelocity));
			player_->ChangeActionState(player_->normalDrawnState_.get());
		}

	}

	void ThrowUmbrella::Exit() {

	}

	////////////////////////////
	// 
	// ReverseChargeAttack 
	// 
	////////////////////////////
	void ReverseCharge::Enter() {

	}

	void ReverseCharge::Update(float deltaTime) {
		// =================================
		// 【 手のJoint位置の設定 】
		// =================================
		//Vector3 handPos = player_->GetPosition();
		//handPos.X += 1.0f * player_->eyesDirection_.X; // プレイヤーの右方向へオフセット
		//handPos.Y += 1.0f; // 少し上へ
		//player_->GetRightHandJoint()->SetPos(handPos);

		// 1秒間に溜まるマナの量
		float chargeSpeed = 10.0f * deltaTime;

		// プレイヤーのマナコンポーネントを取得
		ManaComponent& mana = player_->GetManaComponent();

		// マナが満タンじゃなければ、通常のマナを回復！
		if (mana.GetCurrentMana() < mana.GetMaxMana()) {
			mana.AddMana(chargeSpeed); // ※ManaComponentの回復関数を呼ぶ
		}
		// 満タンなら、傘に「過剰マナ（水）」を溜める！
		else {
			player_->GetUmbrella().top_->GetManaComponent().AddMana(chargeSpeed);

			// ※ここで傘のモデルを少し膨張させたり、水のエフェクトを濃くする
			player_->GetSmashCollider()->ClearVertices();
			player_->GetSmashCollider()->SetVertices({
				{-0.125f * player_->GetUmbrella().top_->GetManaComponent().GetCurrentMana(),-1.5f,0.0f},
				{0.0f,2.0f,0.0f},
				{0.125f * player_->GetUmbrella().top_->GetManaComponent().GetCurrentMana(),-1.5f,0.0f}
			});
		}

		// 攻撃ボタンを離したら、チャージ攻撃ステートへ移行！
		if (player_->GetInput().attack == ButtonState::Released) {
			player_->ChangeActionState(player_->reverseAttackState_.get());
		}

		//ImGui::Text("Charging... Current Mana: %.1f / %.1f, Umbrella Mana: %.1f / %.1f", mana.GetCurrentMana(), mana.GetMaxMana(), player_->GetUmbrella().top_->GetManaComponent().GetCurrentMana(), player_->GetUmbrella().top_->GetManaComponent().GetMaxMana());
	}

	void ReverseCharge::Exit() {

	}
	////////////////////////////
	//
	//  ReverseAttack
	// 
	////////////////////////////
	void ReverseAttack::Enter() {
		// 傘に溜まった過剰マナをすべて取り出す！
		float extraDamage = player_->GetUmbrella().top_->GetManaComponent().GetCurrentMana();

		// 基礎攻撃力 ＋ 過剰マナによるボーナスダメージ！
		float baseAttack = 0.0f; // 通常の逆さ攻撃の威力
		float finalAttack = baseAttack + extraDamage;
		player_->GetUmbrella().top_->GetManaComponent().ConsumeMana(extraDamage); // 傘のマナを0にする

		// 傘の StatusComponent の攻撃力を一時的に書き換える
		player_->GetUmbrella().top_->GetStatusComponent().SetAttack(finalAttack);
		player_->GetUmbrella().top_->GetStatusComponent().IncrementAttackInstanceId();

		// 傘自体を「攻撃ステート」にして当たり判定をONにする
		player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::NormalAttack());

		// ※ プレイヤーの攻撃モーション（バシャーン！と水をぶちまける）を再生
		motion_.Play("Swing", { 0.0f,0.0f,0.0f }, 0.5f);

		player_->GetSmashCollider()->SetMyType(COL_Player_Attack_Smash);
	}

	void ReverseAttack::Update([[maybe_unused]] float deltaTime) {
		//Vector3 handPos = player_->GetPosition();
		//handPos.X += 1.0f * player_->eyesDirection_.X; // プレイヤーの右方向へオフセット
		//handPos.Y += 1.0f; // 少し上へ
		//player_->GetRightHandJoint()->SetPos(motion_.Update(deltaTime, player_->eyesDirection_) + handPos);

		Vector3 pos = motion_.Update(deltaTime, player_->eyesDirection_);

		// 攻撃モーションが終わったら、通常の攻撃状態に戻す
		if (motion_.IsPlaying() == false) {
			// 傘の攻撃ステートを解除して通常状態に戻す
			player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::Attached());
			// プレイヤーの攻撃モーションも終了させる
			player_->ChangeActionState(player_->normalDrawnState_.get());
		}
	}

	void ReverseAttack::Exit() {
		// 傘の攻撃力を元に戻す
		player_->GetUmbrella().top_->GetStatusComponent().SetAttack(10.0f); // 基礎攻撃力にリセット
		// 攻撃判定(Collider)をオフにする処理などもここに書く
		player_->GetSmashCollider()->SetMyType(COL_None);

		player_->GetSmashCollider()->ClearVertices();
		player_->GetSmashCollider()->SetMyType(COL_None);
	}

	////////////////////////////
	//
	//  Guard
	// 
	////////////////////////////
	void Guard::Enter() {
		// プレイヤーのガードのアニメーションを開始

		// 座標の設定をする

		//player_->GetCollider()->SetYourType(COL_Ground | COL_Umbrella_Ground);
	}

	void Guard::Update([[maybe_unused]] float deltaTime) {
		const auto& input = player_->GetInput();

		if (input.guard == ButtonState::Released) {
			// ガードボタンを離したら終わる
			player_->ChangeActionState(player_->normalDrawnState_.get());
		}
	}

	void Guard::Exit() {
		// ここでなにかするかも
		//player_->GetCollider()->SetYourType(COL_Enemy | COL_Enemy_Attack | COL_Ground | COL_Umbrella_Ground);
	}

	////////////////////////////
	//
	//  Sheathe
	// 
	////////////////////////////
	void SheatheWeapon::Enter() {
		// 納刀のアニメーション再生
		// 予約の初期化
		player_->ConsumeReservedAction();
	}

	void SheatheWeapon::Update([[maybe_unused]] float deltaTime) {
		const auto& input = player_->GetInput();
		if (input.attack == ButtonState::Pressed) {
			// 攻撃の予約を行う
			//player_->ReserveActionState(player_->attackState_.get());
		}
		else if (input.evasion == ButtonState::Pressed) {
			// 回避の予約を行う
			//player_->ReserveActionState(player_->evasionState_.get());
		}
		
		if (true/*再生が終わったら*/) {
			// 状態を納刀状態に変化
			player_->SetWeaponStance(WeaponStance::Sheathed);

			// 「傘」のJointを背中に移す
			player_->UmbrellaAttachBack();

			// 予約が存在するか確認する
			PlayerStates::Base* nextAction = player_->ConsumeReservedAction();


			if (nextAction != nullptr) {
				// 予約が存在すればそのまま送る
				player_->ChangeActionState(nextAction);
			}
			else {
				// 終わったので通常状態に戻す
			// ※ もしなにか他にあるならこれより前に書く
				player_->ChangeActionState(player_->normalSheathedState_.get());
			}
		}
	}

	void SheatheWeapon::Exit() {

	}
	////////////////////////////
	//
	//  Draw
	// 
	////////////////////////////
	void DrawWeapon::Enter() {
		// 抜刀のアニメーションを再生
		// 予約の初期化
		player_->ConsumeReservedAction();
	}

	void DrawWeapon::Update([[maybe_unused]] float deltaTime) {
		const auto& input = player_->GetInput();
		if (IsButtonDown(input.attack)) {

			//Game::Event::OnAttack();
			// 攻撃の予約を行う
			player_->ReserveActionState(player_->attackState_.get());
		}

		if (true/*再生が終わったら*/) {
			// 状態を抜刀状態に変化
			player_->SetWeaponStance(WeaponStance::Drawn);

			// UmbrellaのJointを背中に移す
			player_->UmbrellaAttachRHand();

			// 予約が存在するか確認する
			PlayerStates::Base* nextAction = player_->ConsumeReservedAction();

			if (nextAction != nullptr) {
				// 予約が存在すればそのまま送る
				if (player_->myVelocity_.X > 7.0f) {
					player_->attackState_->SetAttackID("DashThrust");
				}
				else {
					player_->attackState_->SetAttackID("Close_Y1");
				}

				player_->ChangeActionState(nextAction);
			}
			else {
				// 終わったので通常状態に戻す
			// ※ もしなにか他にあるならこれより前に書く
				player_->ChangeActionState(player_->normalDrawnState_.get());
			}
		}
	}

	void DrawWeapon::Exit() {

	}
	////////////////////////////
	//
	//  Umbrella Open
	// 
	////////////////////////////
	void UmbrellaOpen::Enter() {
		// 傘を開くプレイヤーのアニメーションを開始

		// 傘に開くよう命令を下す
		player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::Open());
		// 予約の初期化
		player_->ConsumeReservedAction();

		// 空中で開いたら少し上昇する
		if (player_->onGround_ == false) {
			if (IsButtonUp(player_->GetInput().aim)) {
				if (player_->GetManaComponent().HasEnoughMana(25.0f)) {
					player_->GetManaComponent().ConsumeMana(25.0f);
					player_->externalVelocity_.Y = 9.0f; // 上昇の初速を与える（数値は調整用）
				}
			}
		}
	}
	void UmbrellaOpen::Update([[maybe_unused]] float deltaTime) {
		const auto& umbrella = player_->GetUmbrella();
		const auto& input = player_->GetInput();

		if (input.guard == ButtonState::Pressed) {
			// 行動の予約を行う
			player_->ReserveActionState(player_->guardState_.get());
		}

		// 傘側のStateで、開き終わったら型が変わる
		// -> 変わったらこちらのStateも別のステートに変更する
		if (umbrella.top_->GetUmbrellaForm() == UmbrellaForm::Opened) {
			// 予約が存在するか確認する
			PlayerStates::Base* nextAction = player_->ConsumeReservedAction();

			if (nextAction != nullptr) {
				// 予約が存在すればそのまま送る
				player_->ChangeActionState(nextAction);
			}
			else {
				// 終わったので通常状態に戻す
			// ※ もしなにか他にあるならこれより前に書く
				player_->ChangeActionState(player_->normalDrawnState_.get());
			}
		}
	}
	void UmbrellaOpen::Exit() {
		// 特になし

	}
	////////////////////////////
	//
	//  Umbrella Close
	// 
	////////////////////////////
	void UmbrellaClose::Enter() {
		// 傘を開くプレイヤーのアニメーションを開始

		// 傘に開くよう命令を下す
		player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::Close());
		// 予約の初期化
		player_->ConsumeReservedAction();
	}
	void UmbrellaClose::Update([[maybe_unused]] float deltaTime) {
		const auto& umbrella = player_->GetUmbrella();
		const auto& input = player_->GetInput();

		if (input.sheathe == ButtonState::Pressed) {

			Game::Event::OnAttack();
			// 行動の予約を行う
			player_->ReserveActionState(player_->sheatheWeaponState_.get());
		}

		// 傘側のStateで、開き終わったら型が変わる
		// -> 変わったらこちらのStateも別のステートに変更する
		if (umbrella.top_->GetUmbrellaForm() == UmbrellaForm::Closed) {
			// 予約が存在するか確認する
			PlayerStates::Base* nextAction = player_->ConsumeReservedAction();

			if (nextAction != nullptr) {
				// 予約が存在すればそのまま送る
				player_->ChangeActionState(nextAction);
			}
			else {
				// 終わったので通常状態に戻す
			// ※ もしなにか他にあるならこれより前に書く
				player_->ChangeActionState(player_->normalDrawnState_.get());
			}
		}
	}
	void UmbrellaClose::Exit() {

	}
	////////////////////////////
	//
	//  Umbrella Reverse
	// 
	////////////////////////////
	void UmbrellaReverse::Enter() {
		// 傘を開くプレイヤーのアニメーションを開始

		// 傘に開くよう命令を下す
		player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::Reverse());
		// 予約の初期化
		player_->ConsumeReservedAction();

	}
	void UmbrellaReverse::Update([[maybe_unused]]float deltaTime) {
		const auto& umbrella = player_->GetUmbrella();
		//const auto& input = player_->GetInput();

		//if (input.isSheathe) {
		//	// 行動の予約を行う
		//	player_->ReserveActionState(player_->sheatheWeaponState_.get());
		//}

		// 傘側のStateで、開き終わったら型が変わる
		// -> 変わったらこちらのStateも別のステートに変更する
		if (umbrella.top_->GetUmbrellaForm() == UmbrellaForm::Reverse) {
			// 予約が存在するか確認する
			PlayerStates::Base* nextAction = player_->ConsumeReservedAction();

			if (nextAction != nullptr) {
				// 予約が存在すればそのまま送る
				player_->ChangeActionState(nextAction);
			}
			else {
				// 終わったので通常状態に戻す
			// ※ もしなにか他にあるならこれより前に書く
				player_->ChangeActionState(player_->normalDrawnState_.get());
			}
		}
	}
	void UmbrellaReverse::Exit() {

	}
	////////////////////////////
	//
	//  Repair Umbrella
	// 
	////////////////////////////
	void RepairUmbrella::Enter() {
		// パラメータの初期化
		repairTimer_ = 0.0f;
		finishTimer_ = 0.0f;
		isFinishing_ = false;

		// 傘を修理するアニメーションを開始

		// 予約の初期化
		player_->ConsumeReservedAction();

		player_->ChangeMovementState(player_->restrictedState_.get()); // 修理中は移動できないようにする
	}

	void RepairUmbrella::Update(float deltaTime) {
		// ===================
		// 【 手の位置の調整 】
		// ===================

		const auto& input = player_->GetInput();
		auto& status = player_->GetUmbrella().top_->GetStatusComponent();
		//   =====================================================
		// 【 回避（コロリン）によるキャンセル処理（いつでも可能）】
		//   =====================================================
		if (input.evasion == ButtonState::Pressed) {
			// ※ここでエフェクトや音を止める処理を入れる
			// 回避ステートへ強制移行して修理を中断！
			// player_->ChangeActionState(player_->evasionState_.get());
			return;
		}

		//   ===========================
		// 【 0.5秒ごとのループ回復処理 】
		//   ===========================
		repairTimer_ += deltaTime;

		if (repairTimer_ >= REPAIR_INTERVAL) {
			// 最大HPの20%を計算して回復
			float healAmount = HEAL_PERCENTAGE;
			status.Heal(healAmount);

			// ※ここで「カシャッ」という短い修理SEや、火花エフェクトを出す

			// 全回復したかチェック
			if (status.GetHp() >= status.GetMaxHp()) {
				// 全回復した！完了モーションへ移行
				isFinishing_ = true;

				// ※ここで「完成アニメーション（立ち上がる等）」の再生
				// 「シャキーン！」という完了SEを鳴らす
			}
			else {
				// まだ回復できるならタイマーをリセットしてループ
				repairTimer_ = 0.0f;

				// ※ここで修理アニメーションを最初から再生し直す（ループさせる）
			}
		}

		// --------------------------------------------------------
		// 完了モーション中の処理
		// --------------------------------------------------------
		if (isFinishing_) {
			finishTimer_ += deltaTime;
			if (finishTimer_ >= FINISH_TIME) {
				// 完了モーションが終わったら通常状態へ戻る
				player_->ChangeActionState(player_->normalSheathedState_.get());
			}
			return; // 完了モーション中は回復ループの処理をしない
		}


	}

	void RepairUmbrella::Exit() {
		player_->ChangeMovementState(player_->idleState_.get()); // 修理が終わったら移動できるようにする
	}
}