//////	//////	//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Common;
//////	//////	//////	//////	//////	//////	//////	//////	//////

import <type_traits>;

import <limits>;

import <utility>;

export import : TypeAlias;
export import : SIMD;
export import : Time;
export import : Mixins;

export import : DataStructure.List;
export import : DataStructure.Bitset;

export import nlohmann.json;

//****	******	******	******	******	******	******	******	****//

namespace Lumina {
	export template<typename T_First, typename T_Second>
	using Pair = std::pair<T_First, T_Second>;
}

namespace Lumina {
	export struct F32x2 { F32 X, Y; };
	export struct F32x3 { F32 X, Y, Z; };
	export struct F32x4 { F32 X, Y, Z, W; };
}

namespace Lumina::Bit {
	export template<typename T>
		requires (sizeof(T) == sizeof(U32))
	T As(U32 val_) { return { *reinterpret_cast<T*>(&val_) }; }
}

//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

namespace Lumina::Numeric {
	export template<typename T> requires (std::is_floating_point_v<T>)
		constexpr typename T Inf{ std::numeric_limits<T>::infinity() };

	export template<typename T> requires (std::is_floating_point_v<T>)
		constexpr typename T MinAbove0{ std::numeric_limits<T>::min() };
	export template<typename T>
		constexpr typename T Max{ std::numeric_limits<T>::max() };
}