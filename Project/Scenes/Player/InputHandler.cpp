module Game.Player : InputHandler;

import : Main;

import Lumina.Main;
import Lumina.OS.Windows.RawInput;

void InputHandler::SetPlayer(Player* player) { player_ = player; }

void InputHandler::HandleInput() {
	if (!player_) return;
	
    auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
    auto const& keyboard{ inputMngr.Keyboard() };

    // 初期化
	PlayerInputData input;
	input.moveDirection = { 0.0f,0.0f,0.0f };
	// 移動入力を取得
    // コントローラまだ実装してないから一旦コメントアウト
    /*float stickX = InputManager::GetGamePad(0).GetLeftStickX();
    float stickY = InputManager::GetGamePad(0).GetLeftStickY();
    if (std::abs(stickX) > 0.15f) { input.moveDirection.X = stickX; }
    if (std::abs(stickY) > 0.15f) input.moveDirection.Z = stickY;*/
    /*float rightStickX = InputManager::GetGamePad(0).GetRightStickX();
    float rightStickY = InputManager::GetGamePad(0).GetRightStickY();
    input.aimingDirectionX = (std::abs(rightStickX) > 0.15f) ? rightStickX : 0.0f;
    input.aimingDirectionY = (std::abs(rightStickY) > 0.15f) ? rightStickY : 0.0f;*/


    using Lumina::OS::Windows::KEY;
    if (keyboard.IsPressed(KEY::W)) { input.moveDirection.Z += 1.0f; }
    if (keyboard.IsPressed(KEY::S)) { input.moveDirection.Z -= 1.0f; }
    if (keyboard.IsPressed(KEY::A)) { input.moveDirection.X -= 1.0f; }
    if (keyboard.IsPressed(KEY::D)) { input.moveDirection.X += 1.0f; }

    if (input.moveDirection.X != 0.0f) {
        player_->eyesDirection_.X = input.moveDirection.X;
    }

    // --- アクション入力の取得 ---
    // IsJump, IsAttack, TrigerEvasionはコントローラ関連かな？わからん
    // ==================
    // 【 アタック系 】
    // ==================
    input.isJump = false;
    if (keyboard.IsPressed(KEY::SPACE)) {
        input.isJump = true;
    }

    input.isAttack = false;
    if (keyboard.IsPressed(KEY::J)) {
        input.isAttack = true;
    }

    input.isAttackHeld = false;
    /*if (InputManager::GetKey().HoldKey(DIK_J)) {
        input.isAttackHeld = true;
    }*/
    /*if (InputManager::GetGamePad(0).IsHold(XINPUT_GAMEPAD_Y)) {
        input.isAttackHeld = true;
    }*/

    input.isAttackReleased = false;
   /* if (InputManager::GetKey().ReleaseKey(DIK_J)) {
        input.isAttackReleased = true;
    }*/
    /*if (InputManager::GetGamePad(0).IsRelease(XINPUT_GAMEPAD_Y)) {
        input.isAttackReleased = true;
    }*/

    //input.isEvasion = InputManager::TrigerEvasion();
    input.isShoot = false;
   /* if (InputManager::GetGamePad(0).IsPressed(XINPUT_GAMEPAD_X)) {
        input.isShoot = true;
    }*/

    
    input.isReverse = false;
    /*if (InputManager::GetKey().PressedKey(DIK_O)) {
        input.isReverse = true;
    }*/

    // 納刀 -> 回復がスムーズに入力出来る
    input.isSheathe = false;
    if (keyboard.IsPressed(KEY::ENTER)) {
        input.isSheathe = true;
    }

    input.isGuard = false;
    if (keyboard.IsPressed(KEY::I)) {
        input.isGuard = true;
    }

    input.useMana = false;
    // マナ使用モードとして実装するかどうか
    if (keyboard.IsPressed(KEY::SHIFT_LEFT)) {
        input.useMana = true;
    }

    // ================
    // 【 照準・発射 】
    // ================
    input.isAiming = false; // 照準を合わせているかどうか
    input.isAimingHeld = false;
    input.isShoot = false;  // 射撃したかどうか
    /*if (InputManager::GetGamePad(0).IsPressed(XINPUT_GAMEPAD_LEFT_SHOULDER)) {
        input.isAiming = true;
    }
    if (InputManager::GetGamePad(0).IsHold(XINPUT_GAMEPAD_LEFT_SHOULDER)) {
        input.isAimingHeld = true;
    }
    if (InputManager::GetGamePad(0).IsPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
        input.isShoot = true;
    }*/

    // ================
    // 【 修復 】
    // ================
    input.isRepair = false;
   /* if (InputManager::GetKey().PressedKey(DIK_R)) {
        input.isRepair = true;
    }
    if (InputManager::GetGamePad(0).IsPressed(XINPUT_GAMEPAD_X)) {
        input.isShoot = true;
    }*/

    // Playerに入力情報を渡す！
    player_->SetInputData(input);
}