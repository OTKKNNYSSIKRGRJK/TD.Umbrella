export module Game.CharacterTest;

import <vector>;

import Lumina;
import Lumina.Sprite;

namespace Game {
	export class CharacterTest {
	public:
		constexpr Lumina::Math::F32x3 const& Position() const noexcept { return Position_; }
		constexpr Lumina::Math::F32x3 const& Velocity() const noexcept { return Velocity_; }

	public:
		void Update();

	public:
		void Initialize();

	private:
		Lumina::Math::F32x3 Position_;
		Lumina::Math::F32x3 Velocity_;

		Lumina::Math::F32x3 ModelScale_;
		Lumina::Math::F32x3 ModelRotate_;
		Lumina::Math::F32x3 ModelTranslate_;

		std::vector<Lumina::Sprite> Sprites_;
	};
}