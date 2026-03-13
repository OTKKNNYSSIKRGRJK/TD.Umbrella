module Game.CharacterTest;

import Lumina;

namespace Game {
	void CharacterTest::Initialize() {
		auto& sprite{ Sprites_.emplace_back() };
		sprite.Scale = { 500.0f, 500.0f };
		sprite.TextureID = 0;
		sprite.UVs[1] = { 1.0f, 1.0f };

		auto& sprite2{ Sprites_.emplace_back() };
		sprite2.Scale = { 250.0f, 250.0f };
		sprite2.Translate = { 400.0f, 100.0f };
		sprite2.TextureID = 1;
	}

	void CharacterTest::Update() {
		auto& context{ Lumina::Context::Instance() };
		auto& spriteMngr{ context.SpriteManager() };
		auto const& cmdList{ context.MainCommandList() };

		spriteMngr.Begin(cmdList);
		spriteMngr.BatchBegin();
		for (auto const& sprite : Sprites_) {
			spriteMngr.Batch(sprite);
		}
		spriteMngr.BatchEnd();
	}
}