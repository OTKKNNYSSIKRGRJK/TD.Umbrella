//////	//////	//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Math : Fundamental.Exponentiation;
//////	//////	//////	//////	//////	//////	//////	//////	//////

import <cmath>;

import Lumina.Core.Common;

namespace Lumina::Math {
	export inline auto SQRT(F32 x_) noexcept -> F32 { return std::sqrt(x_); }
}