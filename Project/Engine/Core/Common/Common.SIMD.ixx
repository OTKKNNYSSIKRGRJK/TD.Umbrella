module;

#if defined(NDEBUG) || defined(_LUMINA_DEBUG_INTRINSICS_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

#if defined(__AVX2__) && !defined(_LUMINA_INTRINSICS_AVX2_)
#define _LUMINA_INTRINSICS_AVX2_
#endif

//####	######	######	######	######	######	######	######	####//

//--#-	defined(NDEBUG) || defined(_LUMINA_DEBUG_INTRINSICS_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
#else
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

#define _LUMINA_INTRINSICS_UNUSED_

//--#-	!(defined(NDEBUG) || defined(_LUMINA_DEBUG_INTRINSICS_))
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
#endif

//####	######	######	######	######	######	######	######	####//

#if !defined(_LUMINA_INTRINSICS_UNUSED_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

#if defined (_LUMINA_INTRINSICS_AVX2_)
#include <intrin.h>
#endif

//####	######	######	######	######	######	######	######	####//

#if defined (_LUMINA_INTRINSICS_AVX2_)
#define _LUMINA_VECTORCALL_ __vectorcall

//--#-	defined (_LUMINA_INTRINSICS_AVX2_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
#else
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

#define _LUMINA_VECTORCALL_

//--#-	!defined (_LUMINA_INTRINSICS_AVX2_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
#endif

//####	######	######	######	######	######	######	######	####//

#if defined (_LUMINA_INTRINSICS_NEON_)
#include <intrin.h>
#endif

//--#-	!defined(_LUMINA_INTRINSICS_UNUSED_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
#else
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

#define _LUMINA_VECTORCALL_

//--#-	defined(_LUMINA_INTRINSICS_UNUSED_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
#endif

//####	######	######	######	######	######	######	######	####//

#define _LUMINA_INLINE_ __forceinline

//////	//////	//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Common : SIMD;
//////	//////	//////	//////	//////	//////	//////	//////	//////

import : Type;

//****	******	******	******	******	******	******	******	****//

#if !defined (_LUMINA_INTRINSICS_UNUSED_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	Type Aliases											<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

namespace Lumina::SIMD {
	#if defined (_LUMINA_INTRINSICS_AVX2_)
	export using I32x4 = __m128i;
	export using F32x4 = __m128;
	export using F32x8 = __m256;
	#endif
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	Flags													<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
//..''	''..''	''..''	''..''	''..''	''..''	''..''	''..''	''..//
/// @enum		Lumina::SIMD::Flag::CMP
/// @brief		<span>Comparison Type</span>
/// @details
/// ### Description
/// Specifies the comparison operator of the `Lumina::SIMD::Packed::Compare` function.
/// ### References
/// 1. https://stackoverflow.com/questions/16988199/how-to-choose-avx-compare-predicate-variants
/// 2. https://stackoverflow.com/questions/8627331/what-does-ordered-unordered-comparison-mean
/// 3. https://qiita.com/fukushima1981/items/5001079900b328696859
//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//
/// @enum		Lumina::SIMD::Flag::FMAC
/// @brief		<span>Fused Multiply-Accumulate Switch</span>
/// @details
/// ### Description
/// Specifies whether fused multiply-accumulate operations are in use.
/// ### References
/// 1. https://en.wikipedia.org/wiki/Multiply-accumulate_operation
//''..	..''..	..''..	..''..	..''..	..''..	..''..	..''..	..''//
//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

namespace Lumina::SIMD::Flag {

	/// @brief		Comparison Type
	export enum class CMP : U32 {
		#if defined (_LUMINA_INTRINSICS_AVX2_)
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

		/// LHS == RHS
		/// <span style="color:#EF7F9F;">AND</span>
		/// <b>NEITHER</b> contains NaN
		EQ_OQ = _CMP_EQ_OQ,

		/// LHS == RHS
		/// <span style="color:#5FAFEF;">OR</span>
		/// <b>EITHER</b> contains NaN
		EQ_UQ = _CMP_EQ_UQ,

		/// LHS != RHS
		/// <span style="color:#EF7F9F;">AND</span>
		/// <b>NEITHER</b> contains NaN
		NEQ_OQ = _CMP_NEQ_OQ,

		/// LHS != RHS
		/// <span style="color:#5FAFEF;">OR</span>
		/// <b>EITHER</b> contains NaN
		NEQ_UQ = _CMP_NEQ_UQ,

		/// LHS > RHS
		/// <span style="color:#EF7F9F;">AND</span>
		/// <b>NEITHER</b> contains NaN
		GT_OQ = _CMP_GT_OQ,

		/// LHS > RHS
		/// <span style="color:#5FAFEF;">OR</span>
		/// <b>EITHER</b> contains NaN
		GT_UQ = _CMP_NLE_UQ,

		/// LHS <= RHS
		/// <span style="color:#EF7F9F;">AND</span>
		/// <b>NEITHER</b> contains NaN
		NGT_OQ = _CMP_LE_OQ,

		/// LHS <= RHS
		/// <span style="color:#5FAFEF;">OR</span>
		/// <b>EITHER</b> contains NaN
		NGT_UQ = _CMP_NGT_UQ,

		/// LHS < RHS
		/// <span style="color:#EF7F9F;">AND</span>
		/// <b>NEITHER</b> contains NaN
		LT_OQ = _CMP_LT_OQ,

		/// LHS < RHS
		/// <span style="color:#5FAFEF;">OR</span>
		/// <b>EITHER</b> contains NaN
		LT_UQ = _CMP_NGE_UQ,

		/// LHS >= RHS
		/// <span style="color:#EF7F9F;">AND</span>
		/// <b>NEITHER</b> contains NaN
		NLT_OQ = _CMP_GE_OQ,

		/// LHS >= RHS
		/// <span style="color:#5FAFEF;">OR</span>
		/// <b>EITHER</b> contains NaN
		NLT_UQ = _CMP_NLT_UQ,

		//--#-	defined (_LUMINA_INTRINSICS_AVX2_)
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
		#endif
	};

	export enum class FMAC : U32 {
		FALSE = 0U,
		TRUE = 1U,
	};
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	Intrinsic Function Wrappers								<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
//::::	Declaration												:::://
//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

namespace Lumina::SIMD {

	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
	//##++	Packed													++##//
	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	inline namespace Packed {

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			ADD(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SUB(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			MUL(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			DIV(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//
		
		/// @brief		operand_MUL_LHS_ * operand_MUL_RHS_ + operand_ACC_
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			FMADD(
				F32x4 operand_MUL_LHS_,
				F32x4 operand_MUL_RHS_,
				F32x4 operand_ACC_
			) noexcept -> F32x4;
		/// @brief		-(operand_MUL_LHS_ * operand_MUL_RHS_) + operand_ACC_
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			FNMADD(
				F32x4 operand_MUL_LHS_,
				F32x4 operand_MUL_RHS_,
				F32x4 operand_ACC_
			) noexcept -> F32x4;
		/// @brief		operand_MUL_LHS_ * operand_MUL_RHS_ - operand_ACC_
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			FMSUB(
				F32x4 operand_MUL_LHS_,
				F32x4 operand_MUL_RHS_,
				F32x4 operand_ACC_
			) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Exponentiation											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SQRT(F32x4 src_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Dot Product												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export template<U32 Mask_OUT, U32 Mask_IN>
			requires (
				(Mask_OUT < 0x10U) &&
				(Mask_IN < 0x10U)
			)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Dot(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Logical Operations										--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			AND(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			OR(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			XOR(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Comparison												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export template<Flag::CMP Flag_CMP>
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Compare(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Trigonometry											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SIN(F32x4 src_) noexcept -> F32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			COS(F32x4 src_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export template<U32 Mask>
			requires (Mask < 0x100U)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Shuffle(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;
		export template<U32 Index_X, U32 Index_Y, U32 Index_Z, U32 Index_W>
			requires (
				(Index_X < 4U) &&
				(Index_Y < 4U) &&
				(Index_Z < 4U) &&
				(Index_W < 4U)
			)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Shuffle(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

		export template<U32 Mask_OUT, U32 Index_IN_DST, U32 Index_IN_SRC>
			requires (
				(Mask_OUT < 0x10U) &&
				(Index_IN_DST < 4U) &&
				(Index_IN_SRC < 4U)
			)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Insert(F32x4 dst_, F32x4 src_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Set(I32 w_, I32 z_, I32 y_, I32 x_) noexcept -> I32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Set(F32 w_, F32 z_, F32 y_, F32 x_) noexcept -> F32x4;

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SetAll(I32 src_) noexcept -> I32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SetAll(F32 src_) noexcept -> F32x4;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Zero() noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		
		export template<typename OUT, typename IN>
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Cast(IN src_) noexcept -> OUT;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			LoadAligned(F32 const src_[4]) noexcept -> F32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			LoadUnaligned(F32 const src_[4]) noexcept -> F32x4;
	}

	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
	//##++	Scalar													++##//
	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	namespace Scalar {

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			ADD(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SUB(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;
		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			MUL(F32x4 lhs_, F32x4 rhs_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Set(F32 x_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Move(F32x4 dst_, F32x4 src_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		export template<typename T>
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Convert(F32x4 src_) noexcept -> T;
	}
}

//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
//::::	Definition												:::://
//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

#if defined (_LUMINA_INTRINSICS_AVX2_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

namespace Lumina::SIMD {

	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
	//##++	Packed													++##//
	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	namespace Packed {
		
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			ADD(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_add_ps(lhs_, rhs_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SUB(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_sub_ps(lhs_, rhs_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			MUL(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_mul_ps(lhs_, rhs_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			DIV(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_div_ps(lhs_, rhs_); }

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ FMADD(
			F32x4 operand_MUL_LHS_,
			F32x4 operand_MUL_RHS_,
			F32x4 operand_ACC_
		) noexcept -> F32x4 {
			return ::_mm_fmadd_ps(
				operand_MUL_LHS_,
				operand_MUL_RHS_,
				operand_ACC_
			);
		}
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ FNMADD(
			F32x4 operand_MUL_LHS_,
			F32x4 operand_MUL_RHS_,
			F32x4 operand_ACC_
		) noexcept -> F32x4 {
			return ::_mm_fnmadd_ps(
				operand_MUL_LHS_,
				operand_MUL_RHS_,
				operand_ACC_
			);
		}
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ FMSUB(
			F32x4 operand_MUL_LHS_,
			F32x4 operand_MUL_RHS_,
			F32x4 operand_ACC_
		) noexcept -> F32x4 {
			return ::_mm_fmsub_ps(
				operand_MUL_LHS_,
				operand_MUL_RHS_,
				operand_ACC_
			);
		}

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Exponentiation											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SQRT(F32x4 src_)
			noexcept -> F32x4 { return ::_mm_sqrt_ps(src_); }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Dot Product												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		template<U32 Mask_OUT, U32 Mask_IN>
			requires (
				(Mask_OUT < 0x10U) &&
				(Mask_IN < 0x10U)
			)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ Dot(
			F32x4 lhs_,
			F32x4 rhs_
		) noexcept -> F32x4 {
			constexpr U32 mask{
				(Mask_OUT) |
				(Mask_IN << 4U)
			};
			return ::_mm_dp_ps(lhs_, rhs_, mask);
		}
	
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Logical Operations										--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			AND(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_and_ps(lhs_, rhs_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			OR(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_or_ps(lhs_, rhs_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			XOR(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_xor_ps(lhs_, rhs_); }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Comparison												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		template<Flag::CMP Flag_CMP>
			_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Compare(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_cmp_ps(lhs_, rhs_, static_cast<I32>(Flag_CMP)); }


		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Trigonometry											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SIN(F32x4 src_)
			noexcept -> F32x4 { return ::_mm_sin_ps(src_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			COS(F32x4 src_)
			noexcept -> F32x4 { return ::_mm_cos_ps(src_); }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		template<U32 Mask>
			requires (Mask < 0x100U)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ Shuffle(
			F32x4 lhs_,
			F32x4 rhs_
		) noexcept -> F32x4 {
			return ::_mm_shuffle_ps(lhs_, rhs_, Mask);
		}
		template<U32 Index_X, U32 Index_Y, U32 Index_Z, U32 Index_W>
			requires (
				(Index_X < 4U) &&
				(Index_Y < 4U) &&
				(Index_Z < 4U) &&
				(Index_W < 4U)
			)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ Shuffle(
			F32x4 lhs_,
			F32x4 rhs_
		) noexcept -> F32x4 {
			constexpr U32 mask{
				(Index_X) |
				(Index_Y << 2U) |
				(Index_Z << 4U) |
				(Index_W << 6U)
			};
			return ::_mm_shuffle_ps(lhs_, rhs_, mask);
		}

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//
	
		template<U32 Mask_OUT, U32 Index_IN_DST, U32 Index_IN_SRC>
			requires (
				(Mask_OUT < 0x10U) &&
				(Index_IN_DST < 4U) &&
				(Index_IN_SRC < 4U)
			)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ Insert(
			F32x4 dst_,
			F32x4 src_
		) noexcept -> F32x4 {
			constexpr U32 mask{
				(Mask_OUT) |
				(Index_IN_DST << 4U) |
				(Index_IN_SRC << 6U)
			};
			return ::_mm_insert_ps(dst_, src_, mask);
		}
		template<>
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ Insert<0x0U, 0, 0>(
			F32x4 dst_,
			F32x4 src_
		) noexcept -> F32x4 {
			return ::_mm_move_ss(dst_, src_);
		}

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Set(I32 w_, I32 z_, I32 y_, I32 x_)
			noexcept -> I32x4 { return ::_mm_set_epi32(w_, z_, y_, x_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Set(F32 w_, F32 z_, F32 y_, F32 x_)
			noexcept -> F32x4 { return ::_mm_set_ps(w_, z_, y_, x_); }
		
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SetAll(I32 src_)
			noexcept -> I32x4 { return ::_mm_set1_epi32(src_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SetAll(F32 src_)
			noexcept -> F32x4 { return ::_mm_set_ps1(src_); }

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Zero()
			noexcept -> F32x4 { return ::_mm_setzero_ps(); }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		template<>
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Cast<F32x4, I32x4>(I32x4 src_)
			noexcept -> F32x4 { return ::_mm_castsi128_ps(src_); }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			LoadAligned(F32 const src_[4]) noexcept -> F32x4 { return ::_mm_load_ps(src_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			LoadUnaligned(F32 const src_[4]) noexcept -> F32x4 { return ::_mm_loadu_ps(src_); }
	}

	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
	//##++	Scalar													++##//
	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	namespace Scalar {

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			ADD(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_add_ss(lhs_, rhs_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			SUB(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_sub_ss(lhs_, rhs_); }
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			MUL(F32x4 lhs_, F32x4 rhs_)
			noexcept -> F32x4 { return ::_mm_mul_ss(lhs_, rhs_); }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Set(F32 x_)
			noexcept -> F32x4 { return ::_mm_set_ss(x_); }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Move(F32x4 dst_, F32x4 src_)
			noexcept -> F32x4 { return ::_mm_move_ss(dst_, src_); }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	
		template<>
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Convert<F32>(F32x4 src_)
			noexcept -> F32 {;
			return ::_mm_cvtss_f32(src_); }
	}
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	Constant Masks											<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

namespace Lumina::SIMD::Mask {
	namespace {
		constexpr I32 K_I32_0x80000000{ static_cast<I32>(0x80000000) };
		constexpr I32 K_I32_0x0{ static_cast<I32>(0x0) };
	}

	export template<B1 IsMasked_X, B1 IsMasked_Y, B1 IsMasked_Z, B1 IsMasked_W>
	inline F32x4 const Sign{
		Cast<F32x4, I32x4>(
			Set(
				(IsMasked_W) ? (K_I32_0x80000000) : (K_I32_0x0),
				(IsMasked_Z) ? (K_I32_0x80000000) : (K_I32_0x0),
				(IsMasked_Y) ? (K_I32_0x80000000) : (K_I32_0x0),
				(IsMasked_X) ? (K_I32_0x80000000) : (K_I32_0x0)
			)
		)
	};
}

//--#-	defined(_LUMINA_INTRINSICS_AVX2_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
#endif

//--#-	!defined(_LUMINA_INTRINSICS_UNUSED_)
//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
#endif