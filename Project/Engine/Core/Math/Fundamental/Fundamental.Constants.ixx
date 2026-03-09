//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Math : Fundamental.Constants;
//////	//////	//////	//////	//////	//////	//////

import <numbers>;

import Lumina.Core.Common;

//****	******	******	******	******	******	****//

namespace Lumina::Math::Constant {
	export constexpr float Sqrt_2{ std::numbers::sqrt2_v<float> };
	export constexpr float Inv_Sqrt_2{ 1.0f / Sqrt_2 };

	export constexpr float Sqrt_3{ std::numbers::sqrt3_v<float> };
	export constexpr float Inv_Sqrt_3{ std::numbers::inv_sqrt3_v<float> };

	export constexpr float Pi{ std::numbers::pi_v<float> };
	export constexpr float Inv_Pi{ std::numbers::inv_pi_v<float> };
}