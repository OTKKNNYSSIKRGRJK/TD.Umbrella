export module Game.Events.InGame;

import Lumina.EventSystem;

import Lumina.Core.Math;

namespace Game::Event::InGame {
	// イベント構造体設定 -> 登録したら　Game.Scene.InGame.Initializeへ
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
	export struct OnPlayerWarp : public Lumina::EventInterface {
	};

	export struct OnPlayerAttackCombo1 : public Lumina::EventInterface {
	};
	export struct OnPlayerAttackCombo2 : public Lumina::EventInterface {
	};
	export struct OnPlayerAttackCombo3 : public Lumina::EventInterface {
	};
	export struct OnPlayerAttackRot : public Lumina::EventInterface {
	};
	export struct OnPlayerAttackJump : public Lumina::EventInterface {
	};
	export struct OnPlayerFlying : public Lumina::EventInterface {
	};
	export struct OnPlayerCharge : public Lumina::EventInterface {
	};
	export struct OnPlayerChargeAttack : public Lumina::EventInterface {
	};
	export struct OnPlayerThrowUmbrella : public Lumina::EventInterface {
	};
	export struct OnPlayerGainXp : public Lumina::EventInterface {
	};
	export struct OnPlayerLevelUp : public Lumina::EventInterface {
	};
}