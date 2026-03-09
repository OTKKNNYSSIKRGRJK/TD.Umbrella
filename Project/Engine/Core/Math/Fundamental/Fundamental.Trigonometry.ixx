//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Math : Fundamental.Trigonometry;
//////	//////	//////	//////	//////	//////	//////

import <cmath>;

import : Fundamental.Constants;

import Lumina.Core.Common;

namespace Lumina::Math {
	export inline auto SIN(F32 rad_) noexcept -> F32 { return std::sin(rad_); }
	export inline auto COS(F32 rad_) noexcept -> F32 { return std::cos(rad_); }
	export inline auto TAN(F32 rad_) noexcept -> F32 { return std::tan(rad_); }
}

namespace Lumina::Math{
	namespace {
		constexpr F32 Inv_180{ 1.0f / 180.0f };
		constexpr F32 Factor_DegToRad{ Constant::Pi * Inv_180 };
		constexpr F32 Factor_RadToDeg{ Constant::Inv_Pi * 180.0f };
	}

	export constexpr auto DegToRad(F32 deg_) noexcept -> F32 {
		return deg_ * Factor_DegToRad;
	}
	export constexpr auto RadToDeg(F32 rad_) noexcept -> F32 {
		return rad_ * Factor_RadToDeg;
	}
}