export module Game.Umbrella : State;

import : Common;

import Game.MotionManager;

import Lumina.Core.Math;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
}

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
	// これらの状態はアニメーション用
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

	export class Broken : public Base {
	public:
		void Enter()override;
		void Update(float deltaTime)override;
		void Exit()override;
	};
	// ここまでがアニメーション用
	
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

	// 🚀 飛んでいる状態（親から離れて移動中）
	export class Flying : public Base {
	public:
		Flying(const Vector3& initialVelocity) : velocity_(initialVelocity) {}
		void Enter() override;
		void Update(float deltaTime) override;
		void Exit() override;
	private:
		Vector3 velocity_; // 飛んでいく速度
	};

	// 🛑 静止している状態（足場・ワープ先になる）
	export class Stationary : public Base {
	public:
		void Enter() override;
		void Update(float deltaTime) override;
		void Exit() override;
	};
}
