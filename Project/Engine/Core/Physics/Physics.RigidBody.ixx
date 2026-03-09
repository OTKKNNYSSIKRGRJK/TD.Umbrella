export module Lumina.Core.Physics : RigidBody;

import Lumina.Core.Common;
import Lumina.Core.Math;

namespace Lumina::Unit {
	using KG = F32;
}

namespace Lumina {
	namespace {
		constexpr auto Max(F32 lhs_, F32 rhs_)
			noexcept -> F32 { return (lhs_ > rhs_) ? (lhs_) : (rhs_); }
	}
}

namespace Lumina::Physics {
	export class InertialMass {
	public:
		class Inverse;
		class Infinity;

	public:
		constexpr operator Unit::KG() const noexcept;

	public:
		constexpr static auto Infinity() noexcept -> InertialMass;

	public:
		constexpr InertialMass() noexcept;
		constexpr InertialMass(Unit::KG mass_) noexcept;

	protected:
		Unit::KG Mass_;
	};

	constexpr InertialMass::operator Unit::KG()
		const noexcept { return Mass_; }

	constexpr auto InertialMass::Infinity()
		noexcept -> InertialMass { return InertialMass{ Numeric::Inf<F32> }; }

	constexpr InertialMass::InertialMass()
		noexcept : Mass_{ 1.0f } {}

	constexpr InertialMass::InertialMass(Unit::KG mass_)
		noexcept : Mass_{ Max(mass_, Numeric::MinAbove0<F32>) } {}

	//////	//////	//////	//////	//////	//////

	class InertialMass::Inverse {
	public:
		constexpr operator Unit::KG() const noexcept;

	public:
		constexpr static auto Infinity() noexcept -> Inverse;

	public:
		constexpr Inverse() noexcept;
		constexpr Inverse(Unit::KG mass_) noexcept;

	protected:
		Unit::KG Inv_Mass_;
	};

	constexpr InertialMass::Inverse::operator Unit::KG()
		const noexcept { return Inv_Mass_; }

	constexpr InertialMass::Inverse::Inverse()
		noexcept : Inv_Mass_{ 1.0f } {}

	constexpr InertialMass::Inverse::Inverse(Unit::KG mass_)
		noexcept : Inv_Mass_{ 1.0f / Max(mass_, Numeric::MinAbove0<F32>) } {}

	constexpr auto InertialMass::Inverse::Infinity() noexcept -> Inverse {
		Inverse ret{};
		ret.Inv_Mass_ = 0.0f;
		return ret;
	}
}

namespace Lumina::Physics {
	export class Inertia {
	public:
		class Inverse;

	public:
		F32 X;
		F32 Y;
		F32 Z;
	};

	class Inertia::Inverse {
	public:
		friend inline auto operator*(Inverse const& inv_I_, Math::F32x3 const& vec_) -> Math::F32x3;
		friend inline auto operator*(Math::F32x3 const& vec_, Inverse const& inv_I_) -> Math::F32x3;

	public:
		F32 X{ 1.0f };
		F32 Y{ 1.0f };
		F32 Z{ 1.0f };
	};

	export inline auto operator*(Inertia::Inverse const& inv_I_, Math::F32x3 const& vec_) -> Math::F32x3 {
		return Math::F32x3{
			vec_.X * inv_I_.X,
			vec_.Y * inv_I_.Y,
			vec_.Z * inv_I_.Z
		};
	}
	export inline auto operator*(Math::F32x3 const& vec_, Inertia::Inverse const& inv_I_) -> Math::F32x3 {
		return Math::F32x3{
			vec_.X * inv_I_.X,
			vec_.Y * inv_I_.Y,
			vec_.Z * inv_I_.Z
		};
	}
}

namespace Lumina::Physics {
	export class RigidBody {
	public:
		void ApplyLinearImpulse(Math::F32x3 const& impulse_);
		void ApplyAngularImpulse(Math::F32x3 const& impulse_, Math::F32x3 const& appliedPos_);

	public:
		InertialMass::Inverse Inv_Mass;
		Math::F32x3 Position;
		Math::F32x3 LinearVelocity;

		Inertia::Inverse Inv_Inertia;
		Math::Versor Rotation;
		Math::F32x3 AngularVelocity;
	};

	void RigidBody::ApplyLinearImpulse(Math::F32x3 const& impulse_) {
		LinearVelocity += Inv_Mass * impulse_;
	}

	void RigidBody::ApplyAngularImpulse(Math::F32x3 const& impulse_, Math::F32x3 const& appliedPos_) {
		AngularVelocity += Inv_Inertia * Math::F32x3::Cross(appliedPos_ - Position, impulse_);
	}
}