//////	//////	//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Common : TypeAlias;
//////	//////	//////	//////	//////	//////	//////	//////	//////

import <cstdint>;
import <cstddef>;

//****	******	******	******	******	******	******	******	****//

namespace Lumina {
	export using B1 = bool;

	export using C8 = char;
	export using C16 = wchar_t;

	export using C8U = char8_t;
	export using C16U = char16_t;
	export using C32U = char32_t;

	export using SIZE = std::size_t;

	export using I8 = std::int8_t;
	export using I16 = std::int16_t;
	export using I32 = std::int32_t;
	export using I64 = std::int64_t;

	export using U8 = std::uint8_t;
	export using U16 = std::uint16_t;
	export using U32 = std::uint32_t;
	export using U64 = std::uint64_t;

	export using F32 = float;
	export using F64 = double;
}