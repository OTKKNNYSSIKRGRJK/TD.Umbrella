export module Game.Player : InputHandler;

import : Common;

export class InputHandler {
public:
	InputHandler() = default;
public:
	void SetPlayer(Player* player);
	void HandleInput();
private:
	Player* player_;
};