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
    //input.isJump = InputManager::IsJump();
    //input.isAttack = InputManager::IsAttack();
    //input.isEvasion = InputManager::TrigerEvasion();
    input.isSheathe = false; // ※任意のボタンを設定（例: IsSheathe() など）
    input.isGuard = false;
    if (keyboard.IsPressed(KEY::ENTER)) {
        input.isSheathe = true;
    }
    if (keyboard.IsPressed(KEY::I)) {
        input.isGuard = true;
    }

    input.useMana = false;
    // マナ使用モードとして実装するかどうか
    if (keyboard.IsPressed(KEY::SHIFT_LEFT)) {
        input.useMana = true;
    }

    // Playerに入力情報を渡す！
    player_->SetInputData(input);
}