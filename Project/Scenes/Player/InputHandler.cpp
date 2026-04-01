module Game.Player : InputHandler;

import : Main;

import Lumina.Main;
import Lumina.OS.Windows.RawInput;

void InputHandler::SetPlayer(Player* player) { player_ = player; }

void InputHandler::HandleInput() {
	if (!player_) return;
	
    using Lumina::OS::Windows::KEY;

    auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
    auto const& keyboard{ inputMngr.Keyboard() };
    auto const& pad = Lumina::Context::Instance().RawInputContext().Pad();

    // 初期化
	PlayerInputData input;
	input.moveDirection = { 0.0f,0.0f,0.0f };
	// 移動入力を取得
    // コントローラまだ実装してないから一旦コメントアウト
    float stickX = pad.GetLeftStickX();
    float stickY = pad.GetLeftStickY();
    if (std::abs(stickX) > 0.15f) { input.moveDirection.X = stickX; }
    if (std::abs(stickY) > 0.15f) input.moveDirection.Z = stickY;

    float rightStickX = pad.GetRightStickX();
    float rightStickY = pad.GetRightStickY();
    input.aimingDirectionX = (std::abs(rightStickX) > 0.15f) ? rightStickX : 0.0f;
    input.aimingDirectionY = (std::abs(rightStickY) > 0.15f) ? rightStickY : 0.0f;


    if (keyboard.IsPressed(KEY::W)) { input.moveDirection.Z += 1.0f; }
    if (keyboard.IsPressed(KEY::S)) { input.moveDirection.Z -= 1.0f; }
    if (keyboard.IsPressed(KEY::A)) { input.moveDirection.X -= 1.0f; }
    if (keyboard.IsPressed(KEY::D)) { input.moveDirection.X += 1.0f; }

    if (input.moveDirection.X != 0.0f) {
        player_->eyesDirection_.X = input.moveDirection.X;
    }

    // --- アクション入力の取得 ---
    
    // ==================
    // 【 アタック系 】
    // ==================
    input.isJump = false;
    if (keyboard.IsPressed(KEY::SPACE)) {
        input.isJump = true;
    }
    if (pad.IsHold(0x1000)) {
		input.isJump = true;
    }

    input.isAttack = false;
    if (keyboard.IsPressed(KEY::J)) {
        input.isAttack = true;
    }
    if (pad.IsHold(0x8000)) {
        input.isAttack = true;
    }

    input.isAttackHeld = false;
    if (keyboard.IsPressed(KEY::J)) {
        input.isAttackHeld = true;
    }
    if (pad.IsHold(0x8000)) {
        input.isAttackHeld = true;
    }

    input.isAttackReleased = false;
    if (keyboard.IsJustReleased(KEY::J)) {
        input.isAttackReleased = true;
    }
    if (pad.IsRelease(0x8000)) {
        input.isAttackReleased = true;
    }

    //input.isEvasion = InputManager::TrigerEvasion();

    // 納刀 -> 回復がスムーズに入力出来る
    input.isSheathe = false;
    if (keyboard.IsPressed(KEY::ENTER)) {
        input.isSheathe = true;
    }
    if (pad.IsHold(0x4000)) {
        input.isSheathe = true;
    }

    input.isReverse = false;
    if (keyboard.IsPressed(KEY::O)) {
        input.isReverse = true;
    }
    if (pad.IsHold(0x2000)) {
        input.isReverse = true;
    }

    input.isGuard = false;
    if (keyboard.IsPressed(KEY::I)) {
        input.isGuard = true;
    }
    if (pad.GetRightTrigger() > 10) {
        input.isGuard = true;
    }

    input.useMana = false;
    // マナ使用モードとして実装するかどうか
    if (keyboard.IsPressed(KEY::SHIFT_LEFT)) {
        input.useMana = true;
    }
    if (pad.IsHold(0x0100)) {
        input.useMana = true;
    }

    // ================
    // 【 照準・発射 】
    // ================
    input.isAiming = false; // 照準を合わせているかどうか
    input.isAimingHeld = false;
    input.isShoot = false;  // 射撃したかどうか
    if (pad.GetLeftTrigger() > 10) {
        //input.isAiming = true;
        input.isAimingHeld = true;
    }
    if (pad.GetRightTrigger() > 10) {
        input.isShoot = true;
    }

    // ================
    // 【 修復 】
    // ================
    input.isRepair = false;
    if(keyboard.IsPressed(KEY::R)) {
        input.isRepair = true;
	}
    if (pad.IsHold(0x4000)) {
        input.isRepair = true;
	}

    // Playerに入力情報を渡す！
    player_->SetInputData(input);
}