export module Game.Umbrella : State;

import : Common;

import Game.MotionManager;

namespace UmbrellaStates {
	export class Base {
	public:
		virtual ~Base() = default;
		virtual void Enter() = 0;
		virtual void Update(float deltaTime) = 0;
		virtual void Exit() = 0;
		void SetTop(Umbrella::Top* top) { top_ = top; }
	protected:
		Umbrella::Top* top_;
	};

	export class Close : public Base {
	public:
		void Enter()override;
		void Update(float deltaTime)override;
		void Exit()override;
	};

	export class Open : public Base {
	public:
		void Enter()override;
		void Update(float deltaTime)override;
		void Exit()override;
	};

	export class Reverse : public Base {
	public:
		void Enter()override;
		void Update(float deltaTime)override;
		void Exit()override;
	};

	// 手持ち状態（基本は柄にくっついている。プレイヤーの操作を受け付ける）
	export class Attached : public Base {
	public:
		void Enter()override;
		void Update(float deltaTime)override;
		void Exit()override;
	};

	// 攻撃ステート（手には持っているが、武器を振っている危険な状態）
	export class NormalAttack : public Base {
	public:
		void Enter() override;
		void Update(float deltaTime) override;
		void Exit() override;
	private:
		MotionController motion_; // 攻撃時の軌道（剣の振り）もHermiteで制御！
	};

	// 飛行状態（投げられて飛んでいる。独自の速度で座標移動する）
	export class Thrown : public Base {
	public:
		void Enter()override;
		void Update(float deltaTime)override;
		void Exit()override;
	private:
		MotionController motion_;
	};

	// 刺さっている状態（敵や壁にくっついている。ワープ可能）
	export class Stuck : public Base {
	public:
		void Enter()override;
		void Update(float deltaTime)override;
		void Exit()override;
	};
}
