export module Game.EnemyBase;

import <memory>;

import Lumina;
import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Sprite;

namespace Game {
	// 基本的な敵の基底クラス
	export class EnemyBase : public Lumina::NonCopyable<EnemyBase> {
	public:
		virtual ~EnemyBase() = default;

		// フレーム更新処理
		virtual void Update(Lumina::F32 deltaTime_) = 0;

		// 描画処理
		virtual void Render(Lumina::SpriteRenderer& renderer_) = 0;

		// ダメージ処理
		virtual void TakeDamage(Lumina::F32 amount_) {
			if (amount_ <= 0.0f || !IsAlive_) {
				return;
			}
			HP_ -= amount_;
			if (HP_ <= 0.0f) {
				HP_ = 0.0f;
				IsAlive_ = false;
				OnDead();
			}
		}

		// 位置アクセサ
		Lumina::Math::F32x2 const& Position() const noexcept { return Position_; }
		void Position(Lumina::Math::F32x2 const& p_) noexcept { Position_ = p_; }

		// 速度アクセサ
		Lumina::Math::F32x2 const& Velocity() const noexcept { return Velocity_; }
		void Velocity(Lumina::Math::F32x2 const& v_) noexcept { Velocity_ = v_; }

		// HP / 生存状態
		Lumina::F32 HP() const noexcept { return HP_; }
		void HP(Lumina::F32 hp_) noexcept { HP_ = hp_; }
		bool IsAlive() const noexcept { return IsAlive_; }

	protected:
		EnemyBase(
			Lumina::Math::F32x2 const& initialPos_ = { 0.0f, 0.0f },
			Lumina::F32 initialHP_ = 1.0f
		) noexcept
			: Position_{ initialPos_ }
			, Velocity_{ 0.0f, 0.0f }
			, HP_{ initialHP_ }
			, IsAlive_{ initialHP_ > 0.0f } {}

		// 死亡時コールバック（派生クラスで必要ならオーバーライド）
		virtual void OnDead() {}

	protected:
		Lumina::Math::F32x2 Position_{ 0.0f, 0.0f };
		Lumina::Math::F32x2 Velocity_{ 0.0f, 0.0f };
		Lumina::F32 HP_{ 1.0f };
		bool IsAlive_{ true };
	};
}
