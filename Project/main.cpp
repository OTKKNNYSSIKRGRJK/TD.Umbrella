#include<Windows.h>
#include<memory>

import Lumina;
import Game.Scene.Title;
import Game.Scene.InGame;
import Game.Scene.Result;

Lumina::I32 WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, Lumina::I32) {
	auto& context{ Lumina::Context::Instance() };
	context.Initialize();

	auto& sceneMngr{ Lumina::SceneManager::Instance() };
	sceneMngr.Load<"Title">();
	sceneMngr.Load<"InGame">();
	sceneMngr.Load<"Result">();
	sceneMngr.Activate("Title");

	while (context.Run());

	context.Finalize();

	return 0;
}