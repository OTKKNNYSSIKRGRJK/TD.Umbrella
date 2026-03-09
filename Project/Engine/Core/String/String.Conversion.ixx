module;

#include<Windows.h>

export module Lumina.Core.String : Conversion;

import <type_traits>;

import <string>;
import <cuchar>;

import : Common;

import Lumina.Core.Common;

namespace Lumina {

	/// https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
	/// https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte

	namespace {
		enum class CodePage {
			UTF8 = CP_UTF8,
		};

		template<typename ElemType_OUT, typename ElemType_IN>
		inline auto Cast(
			ElemType_IN* data_IN_
		) noexcept -> ElemType_OUT* {
			if constexpr (std::is_same_v<ElemType_OUT, ElemType_IN>) {
				return data_IN_;
			}
			else {
				return static_cast<ElemType_OUT*>(static_cast<void*>(data_IN_));
			}
		}

		template<typename ElemType_OUT, typename ElemType_IN>
		inline auto Cast(
			ElemType_IN const* data_IN_
		) noexcept -> ElemType_OUT const* {
			if constexpr (std::is_same_v<ElemType_OUT, ElemType_IN>) {
				return data_IN_;
			}
			else {
				return static_cast<ElemType_OUT const*>(static_cast<void const*>(data_IN_));
			}
		}

		template<
			Concept::String Type_DST,
			Concept::StringView Type_SRC
		>
		inline auto Convert(
			Type_DST& dst_,
			Type_SRC const& src_,
			I32 size_ = 0,
			CodePage codePage_ = CodePage::UTF8,
			U32 flag_Conversion_ = 0U,
			C8 const* ptr_DefaultChar_ = nullptr
		) {
			using ElementType_DST = typename Type_DST::ElementType;
			using ElementType_SRC = typename Type_SRC::ElementType;

			if (size_ != 0) { dst_.Resize(size_, ElementType_DST{}); }

			I32 const len_SRC{ static_cast<I32>(src_.Length()) };

			if constexpr (sizeof(ElementType_DST) == sizeof(C8)) {
				if constexpr (sizeof(ElementType_SRC) == sizeof(C8)) {
					if (size_ == 0) { return len_SRC; }
					else {
						dst_ = Cast<C8, ElementType_SRC>(src_.Data());
						return size_;
					}
				}
				else if constexpr (sizeof(ElementType_SRC) == sizeof(C16)) {
					C8* data_DST{
						(size_ == 0) ?
						(nullptr) :
						(Cast<C8, ElementType_DST>(dst_.Data()))
					};
					C16 const* data_SRC{
						Cast<C16, ElementType_SRC>(src_.Data())
					};

					return ::WideCharToMultiByte(
						//	Code page
						static_cast<U32>(codePage_),
						//	Flags indicating the conversion type;
						//	must be set to either 0 or MB_ERR_INVALID_CHARS for UTF-8,
						//	otherwise the function failing with ERROR_INVALID_FLAGS
						(codePage_ == CodePage::UTF8) ? (0U) : (flag_Conversion_),
						//	Input pointer to the string to convert
						data_SRC,
						//	Size, in characters, of the string to convert
						len_SRC,
						//	Output pointer to the buffer that receives the converted string;
						//	useless when the next parameter is set to 0
						data_DST,
						//	Size, in bytes, of the buffer to receive the converted string;
						//	the function returns the required buffer size in bytes
						//	(including terminating null character) when set to 0
						size_,
						//	Input pointer to the character to use
						//	if a character cannot be represented in the specified code page;
						//	must be set to NULL or nullptr for UTF-8
						//	otherwise the function failing with ERROR_INVALID_PARAMETER
						(codePage_ == CodePage::UTF8) ? (nullptr) : (ptr_DefaultChar_),
						//	Output pointer to a flag that indicates
						//	if the function has used a default character in the conversion;
						//	must be set to NULL or nullptr for UTF-8
						//	otherwise the function failing with ERROR_INVALID_PARAMETER
						nullptr
					);
				}
			}
			else if constexpr (sizeof(ElementType_DST) == sizeof(C16)) {
				if constexpr (sizeof(ElementType_SRC) == sizeof(C8)) {
					C16* data_DST{ nullptr };
					if (size_ != 0) {
						data_DST = Cast<C16, ElementType_DST>(dst_.Data());
					}
					C8 const* data_SRC{ Cast<C8, ElementType_SRC>(src_.Data()) };

					return ::MultiByteToWideChar(
						//	Code page
						static_cast<U32>(codePage_),
						//	Flags indicating the conversion type;
						//	must be set to either 0 or MB_ERR_INVALID_CHARS for UTF-8
						//	otherwise the function failing with ERROR_INVALID_FLAGS
						(codePage_ == CodePage::UTF8) ? (0U) : (flag_Conversion_),
						// Pointer to the string to convert
						data_SRC,
						//	Size, in bytes, of the string to convert
						len_SRC,
						//	Pointer to the buffer that receives the converted string;
						//	useless when the next parameter is set to 0
						data_DST,
						//	Size, in characters, of the buffer to receive the converted string;
						//	the function returns the required buffer size in characters
						//	(including terminating null character) when set to 0
						size_
					);
				}
				else if constexpr (sizeof(ElementType_SRC) == sizeof(C16)) {
					if (size_ == 0) { return len_SRC; }
					else {
						dst_ = Cast<C16, ElementType_SRC>(src_.Data());
						return size_;
					}
				}
			}
		}

		template<
			Concept::String Type_DST,
			Concept::StringView Type_SRC
		>
		inline auto CalculateNecessarySize(
			Type_DST& dst_,
			Type_SRC const& src_,
			CodePage codePage_ = CodePage::UTF8,
			U32 flag_Conversion_ = 0U,
			C8 const* ptr_DefaultChar_ = nullptr
		) -> I32 {
			return Convert(
				dst_,
				src_,
				0,
				codePage_,
				flag_Conversion_,
				ptr_DefaultChar_
			);
		}
	}
}

namespace Lumina {
	template<
		Concept::String Type_DST,
		Concept::StringView Type_SRC
	>
	auto operator<<=(
		Type_DST& dst_,
		Type_SRC src_
	) -> void {
		dst_.Clear();
		if (src_.IsEmpty()) { return; }

		I32 const size{ CalculateNecessarySize(dst_, src_, CodePage::UTF8) };
		if (size == 0) { return; }

		Convert(dst_, src_, size, CodePage::UTF8);
	}
}