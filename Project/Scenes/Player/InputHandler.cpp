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
	auto playerInput = player_->GetInput();

	input.moveDirection = { 0.0f,0.0f,0.0f };
	///////////////////////////
    ///
    /// 移動入力
    ///
    ///////////////////////////
    float stickX = pad.GetLeftStickX();
    float stickY = pad.GetLeftStickY();
    if (std::abs(stickX) > 0.15f) { input.moveDirection.X = stickX; }
    if (std::abs(stickY) > 0.15f) input.moveDirection.Y = stickY;

    if (keyboard.IsPressed(KEY::W)) { input.moveDirection.Z += 1.0f; }
    if (keyboard.IsPressed(KEY::S)) { input.moveDirection.Z -= 1.0f; }
    if (keyboard.IsPressed(KEY::A)) { input.moveDirection.X -= 1.0f; }
    if (keyboard.IsPressed(KEY::D)) { input.moveDirection.X += 1.0f; }

    if (input.moveDirection.X != 0.0f) {
		player_->eyesDirection_.X = input.moveDirection.X > 0.0f ? 1.0f : -1.0f;
    }
    ///////////////////////////
    ///
    /// 右スティック入力
    ///
    ///////////////////////////
    float rightStickX = pad.GetRightStickX();
    float rightStickY = pad.GetRightStickY();
    input.aimingDirectionX = (std::abs(rightStickX) > 0.15f) ? rightStickX : 0.0f;
    input.aimingDirectionY = (std::abs(rightStickY) > 0.15f) ? rightStickY : 0.0f;

    if (keyboard.IsPressed(KEY::W)) { input.aimingDirectionY += 1.0f; }
    if (keyboard.IsPressed(KEY::S)) { input.aimingDirectionY -= 1.0f; }
    if (keyboard.IsPressed(KEY::A)) { input.aimingDirectionX -= 1.0f; }
    if (keyboard.IsPressed(KEY::D)) { input.aimingDirectionX += 1.0f; }

    // ==========================
    // 【 アクション入力の取得 】
    // ==========================

    input.jump = UpdateButtonState(
        keyboard.IsPressed(KEY::SPACE) || pad.IsHold(0x1000),
        playerInput.jump
	);

    input.attack = UpdateButtonState(
        keyboard.IsPressed(KEY::J) || pad.IsHold(0x8000),
        playerInput.attack
    );
    
    input.evasion = UpdateButtonState(
        keyboard.IsPressed(KEY::O) || pad.IsHold(0x2000),
        playerInput.attack
    );

    input.sheathe = UpdateButtonState(
        keyboard.IsPressed(KEY::ENTER) || pad.IsHold(0x4000),
        playerInput.sheathe
	);

    input.reverse = UpdateButtonState(
        keyboard.IsPressed(KEY::O) || pad.IsHold(0x2000),
        playerInput.reverse
    );

    input.guard = UpdateButtonState(
        keyboard.IsPressed(KEY::I) || pad.GetRightTrigger() > 10,
        playerInput.guard
    );

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
    input.aim = UpdateButtonState(
        keyboard.IsPressed(KEY::K) || pad.GetLeftTrigger() > 100,
        playerInput.aim
    );

    input.shoot = UpdateButtonState(
        keyboard.IsPressed(KEY::L) || pad.GetRightTrigger() > 100,
        playerInput.shoot
    );

    // ================
    // 【 修復 】
    // ================
    input.repair = UpdateButtonState(
        keyboard.IsPressed(KEY::R) || pad.IsHold(0x4000),
        playerInput.repair
	);

    // ================
    // 【 デバッグ用 】
    // ================
#if defined(_DEBUG)
    input.debugRevive = keyboard.IsJustPressed(KEY::BACKSPACE) || pad.IsHold(0x0010);
#endif

    // Playerに入力情報を渡す！
    player_->SetInputData(input);
}