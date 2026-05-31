#include<Windows.h>
#include<memory>

import Lumina;
import Game.Scene.Title;
import Game.Scene.InGame;
import Game.BGMManager;

Lumina::I32 WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, Lumina::I32) {
	auto& context{ Lumina::Context::Instance() };
	context.Initialize();

	auto& sceneMngr{ Lumina::SceneManager::Instance() };
	sceneMngr.Load<"Title">();
	sceneMngr.Load<"InGame">();
	sceneMngr.Activate("Title");

	while (context.Run()) {
		Game::BGMManager::GetInstance()->Update(1.0f / 60.0f);
	}

	context.Finalize();

	return 0;
}