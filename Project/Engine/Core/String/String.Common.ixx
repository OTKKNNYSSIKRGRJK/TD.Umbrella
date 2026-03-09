export module Lumina.Core.String : Common;

import <concepts>;

import <string>;

import <utility>;

import Lumina.Core.Common;

// TODO (if possible)
// Memory management
// Custom String class & StringView class from scratch
// CodePointString class for internal string processing

namespace Lumina {
	class StringBase;
	class StringViewBase;

	template<typename ElemType, typename ElemTraits>
	class [[nodiscard]] StringT;
	template<typename ElemType, typename ElemTraits>
	class [[nodiscard]] StringViewT;

	export template<typename ElemType, SIZE N>
	class [[nodiscard]] StringLiteral;
}

namespace Lumina {
	template<typename ElemType>
	using ElementTraitsT = std::char_traits<ElemType>;
}

namespace Lumina {
	export using String = StringT<C8, ElementTraitsT<C8>>;
	export using WString = StringT<C16, ElementTraitsT<C16>>;

	export using StringView = StringViewT<C8, ElementTraitsT<C8>>;
	export using WStringView = StringViewT<C16, ElementTraitsT<C16>>;
}

namespace Lumina::Concept {
	template<typename T>
	concept String = std::derived_from<T, StringBase>;
	template<typename T>
	concept StringView = std::derived_from<T, StringViewBase>;
}

namespace Lumina {
	template<
		Concept::String Type_DST,
		Concept::StringView Type_SRC
	>
	auto operator<<=(
		Type_DST& dst_,
		Type_SRC src_
	) -> void;
}

namespace Lumina {
	class StringBase {};
	class StringViewBase {};
}

namespace Lumina {
	template<typename ElemType, typename ElemTraits>
	class [[nodiscard]] StringT : public StringBase {
	public:
		using ElementType = ElemType;
		using ElementTraits = ElemTraits;

	public:
		constexpr auto Length()
			const noexcept -> U64 { return Data_.length(); }
		constexpr auto Resize(
			U64 size_,
			ElemType elem_ = ElemType{}
		) noexcept -> void { Data_.resize(size_, elem_); }
		constexpr auto Capacity()
			const noexcept -> U64 { return Data_.capacity(); }
		constexpr auto Clear()
			noexcept -> void { Data_.clear(); }
		constexpr auto IsEmpty()
			const noexcept -> B1 { return Data_.empty(); }
		constexpr auto Data()
			noexcept -> ElemType* { return Data_.data(); }
		constexpr auto Data()
			const noexcept -> ElemType const* { return Data_.data(); }

	public:
		constexpr StringT() noexcept {}
		template<typename ElemType_SRC>
		inline StringT(ElemType_SRC const* src_) noexcept {
			if constexpr (std::is_same_v<ElemType, ElemType_SRC>) {
				Data_ = src_;
			}
			else {
				(*this) <<= StringViewT<ElemType_SRC, ElementTraitsT<ElemType_SRC>>{ src_ };
			}
		}

	private:
		std::pmr::basic_string<ElemType, ElemTraits> Data_;
	};
}

namespace Lumina {
	template<typename ElemType, typename ElemTraits>
	class [[nodiscard]] StringViewT : public StringViewBase {
	public:
		using ElementType = ElemType;
		using ElementTraits = ElemTraits;

	public:
		constexpr auto Length()
			const noexcept -> U64 { return DataView_.length(); }
		constexpr auto IsEmpty()
			const noexcept -> B1 { return DataView_.empty(); }
		constexpr auto Data()
			const noexcept -> ElemType const* { return DataView_.data(); }

	public:
		StringViewT(ElemType const* str_Literal_)
			noexcept : DataView_{ str_Literal_ } {}
		StringViewT(StringT<ElemType, ElemTraits> const& str_)
			noexcept : DataView_{ str_.Data() } {}

	private:
		std::basic_string_view<ElemType, ElemTraits> DataView_;
	};
}

namespace Lumina {

	/// https://github.com/tahonermann/char8_t-remediation/blob/master/char8_t-remediation.h
	/// https://cpprefjp.github.io/lang/cpp20/class_types_in_non-type_template_parameters.html

	template<typename ElemType, SIZE N>
	class [[nodiscard]] StringLiteral {
	public:
		using ElementType = ElemType;

	public:
		#if defined(__cpp_impl_three_way_comparison)
		friend auto operator <=>(StringLiteral const&, StringLiteral const&) = default;
		#endif

	public:
		constexpr static auto [[nodiscard]] Length()
			noexcept -> SIZE { return N; }

	public:
		consteval StringLiteral(
			ElemType const (&src_)[N + 1]
		) noexcept : StringLiteral{ src_, std::make_index_sequence<N + 1>{} } {}

		template<SIZE...IDX_SEQ>
		consteval StringLiteral(
			ElemType const (&src_)[N + 1],
			std::index_sequence<IDX_SEQ...>
		) noexcept : Data{ src_[IDX_SEQ]... } {}

	public:
		ElemType Data[N + 1];
	};

	template<typename ElemType, SIZE N>
	StringLiteral(ElemType const (&src_)[N]) -> StringLiteral<ElemType, N - 1>;
}