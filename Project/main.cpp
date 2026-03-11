#include<Windows.h>
#include<memory>

import Lumina;

Lumina::I32 WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, Lumina::I32) {
	auto context{ std::make_unique<Lumina::Context>() };
	context->Initialize();

	while (context->Run());

	context->Finalize();

	return 0;
}