export module Game.Events.InGame;

import Lumina.EventSystem;

import Lumina.Core.Math;

namespace Game::Event::InGame {
	export struct OnPlayerMove : public Lumina::EventInterface {
		void* Player;
		Lumina::Math::F32x3 Velocity;
	};

	export struct OnPlayerJump : public Lumina::EventInterface {
		void* Player;
		Lumina::Math::F32x3 Velocity;
	};

	export struct OnPlayerAttack : public Lumina::EventInterface {
	};
	export struct OnPlayerReverseCharge : public Lumina::EventInterface {
		float Radius;
	};
	export struct OnPlayerReverseChargeAttack : public Lumina::EventInterface {
		float Power;
	};
	export struct OnPlayerThrowUmbrella : public Lumina::EventInterface {
	};
	export struct OnPlayerWarp : public Lumina::EventInterface {
	};
}