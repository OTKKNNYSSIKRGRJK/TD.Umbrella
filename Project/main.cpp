#include<Windows.h>
#include<memory>

import Lumina;

Lumina::I32 WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, Lumina::I32) {
	auto& context{ Lumina::Context::Instance() };
	context.Initialize();

	auto& sceneMngr{ Lumina::SceneManager::Instance() };
	sceneMngr.Load<"Title">();
	sceneMngr.Activate("Title");

	while (context.Run());

	context.Finalize();

	return 0;
}