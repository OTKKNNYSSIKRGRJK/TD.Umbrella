module;

#include"../.define"

//////	//////	//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Math : LinearAlgebra.Matrix;
//////	//////	//////	//////	//////	//////	//////	//////	//////

import : Fundamental.Constants;

import : LinearAlgebra.Vector;

import Lumina.Core.Common;

//****	******	******	******	******	******	******	******	****//

//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
//..''	''..''	''..''	''..''	''..''	''..''	''..''	''..''	''..//
/// @enum		Lumina::Math::MatrixLayout
/// @brief		<span>Layout of Matrix</span>
/// @details
/// ### Description
/// Specifies whether the matrix is row-major or column-major.
//''..	..''..	..''..	..''..	..''..	..''..	..''..	..''..	..''//
//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

namespace Lumina::Math {
	export enum MatrixLayout {
		ROW_MAJOR,
		COLUMN_MAJOR,
	};
}

//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
//..''	''..''	''..''	''..''	''..''	''..''	''..''	''..''	''..//
/// @class		Lumina::Math::F32x2x2
/// @brief		<span>2x2 Matrix of 32-bit Float Entries</span>
/// @details
/// ### Description
/// 128-bit Aligned if using intrinsics, unaligned otherwise.
//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//
/// @class		Lumina::Math::F32x4x4
/// @brief		<span>4x4 Matrix of 32-bit Float Entries</span>
/// @details
/// ### Description
/// 128-bit Aligned if using intrinsics, unaligned otherwise.
//''..	..''..	..''..	..''..	..''..	..''..	..''..	..''..	..''//
//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

namespace Lumina::Math {
	export template<MatrixLayout Layout = ROW_MAJOR>
	class [[nodiscard]] F32x2x2;
	export template<MatrixLayout Layout = ROW_MAJOR>
	class [[nodiscard]] F32x4x4;
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	F32x2x2<ROW_MAJOR>										<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

namespace Lumina::Math {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Definition												:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	template<>
	class alignas(16LLU) [[nodiscard]] F32x2x2<ROW_MAJOR> {

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
		//##++	Type Aliases											++##//
		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

		using OUT = F32x4x4<ROW_MAJOR>&;
		using IN = F32x4x4<ROW_MAJOR> const&;

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
		
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		/// @brief		dst_ = src_LHS_ + src_RHS_
		_LUMINA_INLINE_ static auto Add(
			F32x2x2<ROW_MAJOR>::OUT dst_,
			F32x2x2<ROW_MAJOR>::IN src_LHS_,
			F32x2x2<ROW_MAJOR>::IN src_RHS_
		) noexcept -> void;
		/// @brief		dst_ = src_LHS_ - src_RHS_
		_LUMINA_INLINE_ static auto Subtract(
			F32x2x2<ROW_MAJOR>::OUT dst_,
			F32x2x2<ROW_MAJOR>::IN src_LHS_,
			F32x2x2<ROW_MAJOR>::IN src_RHS_
		) noexcept -> void;
		/// @brief		dst_ = src_LHS_ * src_RHS_
		template<U32 Flag_FMAC = 0U>
		_LUMINA_INLINE_ static auto Multiply(
			F32x2x2<ROW_MAJOR>::OUT dst_,
			F32x2x2<ROW_MAJOR>::IN src_LHS_,
			F32x2x2<ROW_MAJOR>::IN src_RHS_
		) noexcept -> void;

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	protected:
		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		SIMD::F32x4 Entries_;
		#else
		F32 Entries_[2][2];
		#endif
	};

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Implementation											:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	#if !defined(_LUMINA_INTRINSICS_UNUSED_)
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

	//--#-	!defined(_LUMINA_INTRINSICS_UNUSED_)
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
	#else
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

	//--#-	defined(_LUMINA_INTRINSICS_UNUSED_)
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
	#endif
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	F32x4x4<ROW_MAJOR>										<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

namespace Lumina::Math {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Definition												:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	template<>
	class alignas(16LLU) [[nodiscard]] F32x4x4<ROW_MAJOR> {

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
		//##++	Type Aliases											++##//
		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	public:
		using OUT = F32x4x4<ROW_MAJOR>&;
		using IN = F32x4x4<ROW_MAJOR> const&;

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Cast Operators											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ operator F32x4 const*() const noexcept;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Row Accessors											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr auto [[nodiscard]] operator[](U32 idx_Row_)
			noexcept -> F32x4&;
		constexpr auto [[nodiscard]] operator[](U32 idx_Row_)
			const noexcept -> F32x4 const&;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		/// @brief		dst_ = src_LHS_ + src_RHS_
		_LUMINA_INLINE_ static auto Add(
			F32x4x4<ROW_MAJOR>::OUT dst_,
			F32x4x4<ROW_MAJOR>::IN src_LHS_,
			F32x4x4<ROW_MAJOR>::IN src_RHS_
		) noexcept -> void;
		/// @brief		dst_ = src_LHS_ - src_RHS_
		_LUMINA_INLINE_ static auto Subtract(
			F32x4x4<ROW_MAJOR>::OUT dst_,
			F32x4x4<ROW_MAJOR>::IN src_LHS_,
			F32x4x4<ROW_MAJOR>::IN src_RHS_
		) noexcept -> void;
		/// @brief		dst_ = src_LHS_ * src_RHS_
		template<U32 Flag_FMAC = 0U>
		_LUMINA_INLINE_ static auto Multiply(
			F32x4x4<ROW_MAJOR>::OUT dst_,
			F32x4x4<ROW_MAJOR>::IN src_LHS_,
			F32x4x4<ROW_MAJOR>::IN src_RHS_
		) noexcept -> void;
		/// @brief		dst_ = src_LHS_ * src_RHS_
		template<U32 Flag_FMAC = 0U>
		_LUMINA_INLINE_ static auto Multiply(
			F32x4::OUT dst_,
			F32x4::IN src_LHS_,
			F32x4x4<ROW_MAJOR>::IN src_RHS_
		) noexcept -> void;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		_LUMINA_INLINE_ friend auto operator*(
			F32x4x4<ROW_MAJOR>::IN lhs_,
			F32x4x4<ROW_MAJOR>::IN rhs_
		) noexcept -> F32x4x4<ROW_MAJOR>;
		_LUMINA_INLINE_ friend auto operator*(
			F32x4::IN lhs_,
			F32x4x4<ROW_MAJOR>::IN rhs_
		) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ auto Trace() const noexcept -> F32;

		_LUMINA_INLINE_ auto Determinant() const noexcept -> F32;

		_LUMINA_INLINE_ auto Inverse() const noexcept -> F32x4x4<ROW_MAJOR>;

		_LUMINA_INLINE_ static auto Transpose(
			F32x4x4<ROW_MAJOR>::OUT dst_,
			F32x4x4<ROW_MAJOR>::IN src_
		) noexcept -> void;
		_LUMINA_INLINE_ auto Transpose() const noexcept -> F32x4x4<ROW_MAJOR>;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Constructors, Destructor								--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ F32x4x4() noexcept;
		_LUMINA_INLINE_ F32x4x4(
			F32 m_00_, F32 m_01_, F32 m_02_, F32 m_03_,
			F32 m_10_, F32 m_11_, F32 m_12_, F32 m_13_,
			F32 m_20_, F32 m_21_, F32 m_22_, F32 m_23_,
			F32 m_30_, F32 m_31_, F32 m_32_, F32 m_33_
		) noexcept;
		_LUMINA_INLINE_ F32x4x4(
			F32x4::IN row_0_,
			F32x4::IN row_1_,
			F32x4::IN row_2_,
			F32x4::IN row_3_
		) noexcept;
		_LUMINA_INLINE_ F32x4x4(
			F32x4 const rows_[4]
		) noexcept;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

		constexpr ~F32x4x4() noexcept;

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	protected:
		F32x4 Rows_[4];

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	public:
		F32x4x4<ROW_MAJOR> static const Identity;
	};

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Implementation											:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Cast Operators											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ F32x4x4<ROW_MAJOR>::operator F32x4 const*()
		const noexcept { return Rows_; }
	
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Row Accessors											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	constexpr auto [[nodiscard]] F32x4x4<ROW_MAJOR>::operator[](U32 idx_Row_)
		noexcept -> F32x4& { return Rows_[idx_Row_]; }
	constexpr auto [[nodiscard]] F32x4x4<ROW_MAJOR>::operator[](U32 idx_Row_)
		const noexcept -> F32x4 const& { return Rows_[idx_Row_]; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Arithmetic												--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto F32x4x4<ROW_MAJOR>::Add(
		F32x4x4<ROW_MAJOR>::OUT dst_,
		F32x4x4<ROW_MAJOR>::IN src_LHS_,
		F32x4x4<ROW_MAJOR>::IN src_RHS_
	) noexcept -> void {
		dst_.Rows_[0] = src_LHS_.Rows_[0] + src_RHS_.Rows_[0];
		dst_.Rows_[1] = src_LHS_.Rows_[1] + src_RHS_.Rows_[1];
		dst_.Rows_[2] = src_LHS_.Rows_[2] + src_RHS_.Rows_[2];
		dst_.Rows_[3] = src_LHS_.Rows_[3] + src_RHS_.Rows_[3];
	}

	_LUMINA_INLINE_ auto F32x4x4<ROW_MAJOR>::Subtract(
		F32x4x4<ROW_MAJOR>::OUT dst_,
		F32x4x4<ROW_MAJOR>::IN src_LHS_,
		F32x4x4<ROW_MAJOR>::IN src_RHS_
	) noexcept -> void {
		dst_.Rows_[0] = src_LHS_.Rows_[0] - src_RHS_.Rows_[0];
		dst_.Rows_[1] = src_LHS_.Rows_[1] - src_RHS_.Rows_[1];
		dst_.Rows_[2] = src_LHS_.Rows_[2] - src_RHS_.Rows_[2];
		dst_.Rows_[3] = src_LHS_.Rows_[3] - src_RHS_.Rows_[3];
	}

	/// @ref		https://stackoverflow.com/questions/18499971/efficient-4x4-matrix-multiplication-c-vs-assembly "Reference"
	template<U32 Flag_FMAC>
	_LUMINA_INLINE_ auto F32x4x4<ROW_MAJOR>::Multiply(
		F32x4x4<ROW_MAJOR>::OUT dst_,
		F32x4x4<ROW_MAJOR>::IN src_LHS_,
		F32x4x4<ROW_MAJOR>::IN src_RHS_
	) noexcept -> void {
		for (U32 i{ 0U }; i < 4U; i++) {
			Multiply<Flag_FMAC>(
				dst_.Rows_[i],
				src_LHS_.Rows_[i],
				src_RHS_
			);
		}
	}

	template<U32 Flag_FMAC>
	_LUMINA_INLINE_ auto F32x4x4<ROW_MAJOR>::Multiply(
		F32x4::OUT dst_,
		F32x4::IN src_LHS_,
		F32x4x4<ROW_MAJOR>::IN src_RHS_
	) noexcept -> void {
		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

		F32x4 const entry_I0_SRC_LHS{ src_LHS_.Swizzle<0, 0, 0, 0>() };
		F32x4 const entry_I1_SRC_LHS{ src_LHS_.Swizzle<1, 1, 1, 1>() };
		F32x4 const entry_I2_SRC_LHS{ src_LHS_.Swizzle<2, 2, 2, 2>() };
		F32x4 const entry_I3_SRC_LHS{ src_LHS_.Swizzle<3, 3, 3, 3>() };

		F32x4 const row_0_SRC_RHS{ src_RHS_.Rows_[0] };
		F32x4 const row_1_SRC_RHS{ src_RHS_.Rows_[1] };
		F32x4 const row_2_SRC_RHS{ src_RHS_.Rows_[2] };
		F32x4 const row_3_SRC_RHS{ src_RHS_.Rows_[3] };

		if constexpr (Flag_FMAC == static_cast<U32>(SIMD::Flag::FMAC::FALSE)) {
			dst_ =
				(
					(entry_I0_SRC_LHS * row_0_SRC_RHS) +
					(entry_I1_SRC_LHS * row_1_SRC_RHS)
				) +
				(
					(entry_I2_SRC_LHS * row_2_SRC_RHS) +
					(entry_I3_SRC_LHS * row_3_SRC_RHS)
				);
		}
		else {
			dst_ = entry_I0_SRC_LHS * row_0_SRC_RHS;
			dst_ = F32x4::MultiplyAdd(entry_I1_SRC_LHS, row_1_SRC_RHS, dst_);
			dst_ = F32x4::MultiplyAdd(entry_I2_SRC_LHS, row_2_SRC_RHS, dst_);
			dst_ = F32x4::MultiplyAdd(entry_I3_SRC_LHS, row_3_SRC_RHS, dst_);
		}

		//--#-	!defined(_LUMINA_INTRINSICS_UNUSED_)
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
		#else
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

		for (U32 i{ 0U }; i < 4U; ++i) {
			for (U32 j{ 0U }; j < 4U; ++j) {
				dst_.Set(
					j,
					src_LHS_.Get(0U) * src_RHS_.Rows_[0U].Get(j) +
					src_LHS_.Get(1U) * src_RHS_.Rows_[1U].Get(j) +
					src_LHS_.Get(2U) * src_RHS_.Rows_[2U].Get(j) +
					src_LHS_.Get(3U) * src_RHS_.Rows_[3U].Get(j)
				);
			}
		}

		//--#-	defined(_LUMINA_INTRINSICS_UNUSED_)
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
		#endif
	}

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	export _LUMINA_INLINE_ auto operator*(
		F32x4x4<ROW_MAJOR>::IN lhs_,
		F32x4x4<ROW_MAJOR>::IN rhs_
	) noexcept -> F32x4x4<ROW_MAJOR> {
		F32x4x4<ROW_MAJOR> ret{};
		F32x4x4<ROW_MAJOR>::Multiply(ret, lhs_, rhs_);
		return ret;
	}
	
	export _LUMINA_INLINE_ auto operator*(
		F32x4::IN lhs_,
		F32x4x4<ROW_MAJOR>::IN rhs_
	) noexcept -> F32x4 {
		F32x4 ret{};
		F32x4x4<ROW_MAJOR>::Multiply(ret, lhs_, rhs_);
		return ret;
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	//_LUMINA_INLINE_ auto F32x4x4<ROW_MAJOR>::Determinant() const noexcept -> F32 {
	//}

	//_LUMINA_INLINE_ auto F32x4x4<ROW_MAJOR>::Inverse() const noexcept -> F32x4x4<ROW_MAJOR> {
	//}

	_LUMINA_INLINE_ auto F32x4x4<ROW_MAJOR>::Transpose(
		F32x4x4<ROW_MAJOR>::OUT dst_,
		F32x4x4<ROW_MAJOR>::IN src_
	) noexcept -> void {
		F32x4 const src_00_01_10_11{ F32x4::Shuffle<0, 1, 0, 1>(src_.Rows_[0], src_.Rows_[1]) };
		F32x4 const src_02_03_12_13{ F32x4::Shuffle<2, 3, 2, 3>(src_.Rows_[0], src_.Rows_[1]) };
		F32x4 const src_20_21_30_31{ F32x4::Shuffle<0, 1, 0, 1>(src_.Rows_[2], src_.Rows_[3]) };
		F32x4 const src_22_23_32_33{ F32x4::Shuffle<2, 3, 2, 3>(src_.Rows_[2], src_.Rows_[3]) };

		dst_ = {
			//	SRC[0][0], SRC[1][0], SRC[2][0], SRC[3][0]
			F32x4::Shuffle<0, 2, 0, 2>(src_00_01_10_11, src_20_21_30_31),
			//	SRC[0][1], SRC[1][1], SRC[2][1], SRC[3][1]
			F32x4::Shuffle<1, 3, 1, 3>(src_00_01_10_11, src_20_21_30_31),
			//	SRC[0][2], SRC[1][2], SRC[2][2], SRC[3][2]
			F32x4::Shuffle<0, 2, 0, 2>(src_02_03_12_13, src_22_23_32_33),
			//	SRC[0][3], SRC[1][3], SRC[2][3], SRC[3][3]
			F32x4::Shuffle<1, 3, 1, 3>(src_02_03_12_13, src_22_23_32_33),
		};
	}
	_LUMINA_INLINE_ auto F32x4x4<ROW_MAJOR>::Transpose() const noexcept -> F32x4x4<ROW_MAJOR> {
		F32x4x4<ROW_MAJOR> ret{};
		Transpose(ret, *this);
		return ret;
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Constructors, Destructor								--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ F32x4x4<ROW_MAJOR>::F32x4x4() noexcept :
		Rows_{} {}
	_LUMINA_INLINE_ F32x4x4<ROW_MAJOR>::F32x4x4(
		F32 m_00_, F32 m_01_, F32 m_02_, F32 m_03_,
		F32 m_10_, F32 m_11_, F32 m_12_, F32 m_13_,
		F32 m_20_, F32 m_21_, F32 m_22_, F32 m_23_,
		F32 m_30_, F32 m_31_, F32 m_32_, F32 m_33_
	) noexcept :
		Rows_{
			{ m_00_, m_01_, m_02_, m_03_ },
			{ m_10_, m_11_, m_12_, m_13_ },
			{ m_20_, m_21_, m_22_, m_23_ },
			{ m_30_, m_31_, m_32_, m_33_ },
		} {}
	_LUMINA_INLINE_ F32x4x4<ROW_MAJOR>::F32x4x4(
		F32x4::IN row_0_,
		F32x4::IN row_1_,
		F32x4::IN row_2_,
		F32x4::IN row_3_
	) noexcept :
		Rows_{ row_0_, row_1_, row_2_, row_3_, } {}
	_LUMINA_INLINE_ F32x4x4<ROW_MAJOR>::F32x4x4(
		F32x4 const rows_[4]
	) noexcept :
		Rows_{ rows_[0], rows_[1], rows_[2], rows_[3], } {}


	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	constexpr F32x4x4<ROW_MAJOR>::~F32x4x4() noexcept {}

	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	F32x4x4<ROW_MAJOR> const F32x4x4<ROW_MAJOR>::Identity{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	F32x4x4<COLUMN_MAJOR>									<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

namespace Lumina::Math {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Definition												:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	template<>
	class alignas(16LLU) [[nodiscard]] F32x4x4<COLUMN_MAJOR> {

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
		//##++	Type Aliases											++##//
		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	public:
		using OUT = F32x4x4<COLUMN_MAJOR>&;
		using IN = F32x4x4<COLUMN_MAJOR> const&;

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
		
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Cast Operators											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ operator F32x4 const*() const noexcept;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Column Accessors										--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr auto [[nodiscard]] operator[](U32 idx_Col_) noexcept -> F32x4&;
		constexpr auto [[nodiscard]] operator[](U32 idx_Col_) const noexcept -> F32x4 const&;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		/// @brief		dst_ = src_LHS_ + src_RHS_
		_LUMINA_INLINE_ static auto Add(
			F32x4x4<COLUMN_MAJOR>::OUT dst_,
			F32x4x4<COLUMN_MAJOR>::IN src_LHS_,
			F32x4x4<COLUMN_MAJOR>::IN src_RHS_
		) noexcept -> void;
		/// @brief		dst_ = src_LHS_ - src_RHS_
		_LUMINA_INLINE_ static auto Subtract(
			F32x4x4<COLUMN_MAJOR>::OUT dst_,
			F32x4x4<COLUMN_MAJOR>::IN src_LHS_,
			F32x4x4<COLUMN_MAJOR>::IN src_RHS_
		) noexcept -> void;
		/// @brief		dst_ = src_LHS_ * src_RHS_
		template<U32 Flag_FMAC = 0U>
		_LUMINA_INLINE_ static auto Multiply(
			F32x4x4<COLUMN_MAJOR>::OUT dst_,
			F32x4x4<COLUMN_MAJOR>::IN src_LHS_,
			F32x4x4<COLUMN_MAJOR>::IN src_RHS_
		) noexcept -> void;
		/// @brief		dst_ = src_LHS_ * src_RHS_
		_LUMINA_INLINE_ static auto Multiply(
			F32x4::OUT dst_,
			F32x4x4<COLUMN_MAJOR>::IN src_LHS_,
			F32x4::IN src_RHS_
		) noexcept -> void;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		_LUMINA_INLINE_ friend auto operator*(
			F32x4x4<COLUMN_MAJOR>::IN lhs_,
			F32x4x4<COLUMN_MAJOR>::IN rhs_
		) noexcept -> F32x4x4<COLUMN_MAJOR>;
		_LUMINA_INLINE_ friend auto operator*(
			F32x4x4<COLUMN_MAJOR>::IN lhs_,
			F32x4::IN rhs_
		) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Constructors, Destructor								--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ F32x4x4() noexcept;
		_LUMINA_INLINE_ F32x4x4(
			F32 m_00_, F32 m_10_, F32 m_20_, F32 m_30_,
			F32 m_01_, F32 m_11_, F32 m_21_, F32 m_31_,
			F32 m_02_, F32 m_12_, F32 m_22_, F32 m_32_,
			F32 m_03_, F32 m_13_, F32 m_23_, F32 m_33_
		) noexcept;
		_LUMINA_INLINE_ F32x4x4(
			F32x4::IN col_0_,
			F32x4::IN col_1_,
			F32x4::IN col_2_,
			F32x4::IN col_3_
		) noexcept;
		_LUMINA_INLINE_ F32x4x4(
			F32x4 const cols_[4]
		) noexcept;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

		constexpr ~F32x4x4() noexcept;

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	protected:
		F32x4 Columns_[4];

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	public:
		F32x4x4<COLUMN_MAJOR> static const Identity;
	};

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Implementation											:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Cast Operators											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ F32x4x4<COLUMN_MAJOR>::operator F32x4 const*()
		const noexcept { return Columns_; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Column Accessors										--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	constexpr auto [[nodiscard]] F32x4x4<COLUMN_MAJOR>::operator[](U32 idx_Col_)
		noexcept -> F32x4& { return Columns_[idx_Col_]; }
	constexpr auto [[nodiscard]] F32x4x4<COLUMN_MAJOR>::operator[](U32 idx_Col_)
		const noexcept -> F32x4 const& { return Columns_[idx_Col_]; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Arithmetic												--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto F32x4x4<COLUMN_MAJOR>::Add(
		F32x4x4<COLUMN_MAJOR>::OUT dst_,
		F32x4x4<COLUMN_MAJOR>::IN src_LHS_,
		F32x4x4<COLUMN_MAJOR>::IN src_RHS_
	) noexcept -> void {
		dst_.Columns_[0] = src_LHS_.Columns_[0] + src_RHS_.Columns_[0];
		dst_.Columns_[1] = src_LHS_.Columns_[1] + src_RHS_.Columns_[1];
		dst_.Columns_[2] = src_LHS_.Columns_[2] + src_RHS_.Columns_[2];
		dst_.Columns_[3] = src_LHS_.Columns_[3] + src_RHS_.Columns_[3];
	}

	_LUMINA_INLINE_ auto F32x4x4<COLUMN_MAJOR>::Subtract(
		F32x4x4<COLUMN_MAJOR>::OUT dst_,
		F32x4x4<COLUMN_MAJOR>::IN src_LHS_,
		F32x4x4<COLUMN_MAJOR>::IN src_RHS_
	) noexcept -> void {
		dst_.Columns_[0] = src_LHS_.Columns_[0] - src_RHS_.Columns_[0];
		dst_.Columns_[1] = src_LHS_.Columns_[1] - src_RHS_.Columns_[1];
		dst_.Columns_[2] = src_LHS_.Columns_[2] - src_RHS_.Columns_[2];
		dst_.Columns_[3] = src_LHS_.Columns_[3] - src_RHS_.Columns_[3];
	}
	
	template<U32 Flag_FMAC>
	_LUMINA_INLINE_ auto F32x4x4<COLUMN_MAJOR>::Multiply(
		F32x4x4<COLUMN_MAJOR>::OUT dst_,
		F32x4x4<COLUMN_MAJOR>::IN src_LHS_,
		F32x4x4<COLUMN_MAJOR>::IN src_RHS_
	) noexcept -> void {
		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

		for (U32 i{ 0U }; i < 4U; i++) {
			F32x4 const entry_I0_SRC_LHS{ src_LHS_.Columns_[i].Swizzle<0, 0, 0, 0>() };
			F32x4 const entry_I1_SRC_LHS{ src_LHS_.Columns_[i].Swizzle<1, 1, 1, 1>() };
			F32x4 const entry_I2_SRC_LHS{ src_LHS_.Columns_[i].Swizzle<2, 2, 2, 2>() };
			F32x4 const entry_I3_SRC_LHS{ src_LHS_.Columns_[i].Swizzle<3, 3, 3, 3>() };

			F32x4 const col_0_SRC_RHS{ src_RHS_.Columns_[0] };
			F32x4 const col_1_SRC_RHS{ src_RHS_.Columns_[1] };
			F32x4 const col_2_SRC_RHS{ src_RHS_.Columns_[2] };
			F32x4 const col_3_SRC_RHS{ src_RHS_.Columns_[3] };

			if constexpr (Flag_FMAC == static_cast<U32>(SIMD::Flag::FMAC::FALSE)) {
				dst_.Columns_[i] =
					(
						(entry_I0_SRC_LHS * col_0_SRC_RHS) +
						(entry_I1_SRC_LHS * col_1_SRC_RHS)
					) +
					(
						(entry_I2_SRC_LHS * col_2_SRC_RHS) +
						(entry_I3_SRC_LHS * col_3_SRC_RHS)
					);
			}
			else {
				dst_.Columns_[i] = entry_I0_SRC_LHS * col_0_SRC_RHS;
				dst_.Columns_[i] = F32x4::MultiplyAdd(entry_I1_SRC_LHS, col_1_SRC_RHS, dst_.Columns_[i]);
				dst_.Columns_[i] = F32x4::MultiplyAdd(entry_I2_SRC_LHS, col_2_SRC_RHS, dst_.Columns_[i]);
				dst_.Columns_[i] = F32x4::MultiplyAdd(entry_I3_SRC_LHS, col_3_SRC_RHS, dst_.Columns_[i]);
			}
		}

		//--#-	!defined(_LUMINA_INTRINSICS_UNUSED_)
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
		#else
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

		for (U32 i{ 0U }; i < 4U; ++i) {
			for (U32 j{ 0U }; j < 4U; ++j) {
				dst_.Columns_[i].Set(
					j,
					src_LHS_.Columns_[i].Get(0U) * src_RHS_.Columns_[0U].Get(j) +
					src_LHS_.Columns_[i].Get(1U) * src_RHS_.Columns_[1U].Get(j) +
					src_LHS_.Columns_[i].Get(2U) * src_RHS_.Columns_[2U].Get(j) +
					src_LHS_.Columns_[i].Get(3U) * src_RHS_.Columns_[3U].Get(j)
				);
			}
		}

		//--#-	defined(_LUMINA_INTRINSICS_UNUSED_)
		//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
		#endif
	}

	_LUMINA_INLINE_ auto F32x4x4<COLUMN_MAJOR>::Multiply(
		F32x4::OUT dst_,
		F32x4x4<COLUMN_MAJOR>::IN src_LHS_,
		F32x4::IN src_RHS_
	) noexcept -> void {
		dst_ = {
			F32x4::Dot<4>(src_LHS_.Columns_[0], src_RHS_),
			F32x4::Dot<4>(src_LHS_.Columns_[1], src_RHS_),
			F32x4::Dot<4>(src_LHS_.Columns_[2], src_RHS_),
			F32x4::Dot<4>(src_LHS_.Columns_[3], src_RHS_)
		};
	}

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	export _LUMINA_INLINE_ auto operator*(
		F32x4x4<COLUMN_MAJOR>::IN lhs_,
		F32x4x4<COLUMN_MAJOR>::IN rhs_
	) noexcept -> F32x4x4<COLUMN_MAJOR> {
		F32x4x4<COLUMN_MAJOR> ret{};
		F32x4x4<COLUMN_MAJOR>::Multiply(ret, lhs_, rhs_);
		return ret;
	}

	export _LUMINA_INLINE_ auto operator*(
		F32x4x4<COLUMN_MAJOR>::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		F32x4 ret{};
		F32x4x4<COLUMN_MAJOR>::Multiply(ret, lhs_, rhs_);
		return ret;
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Constructors, Destructor								--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ F32x4x4<COLUMN_MAJOR>::F32x4x4() noexcept :
		Columns_{} {}
	_LUMINA_INLINE_ F32x4x4<COLUMN_MAJOR>::F32x4x4(
		F32 m_00_, F32 m_10_, F32 m_20_, F32 m_30_,
		F32 m_01_, F32 m_11_, F32 m_21_, F32 m_31_,
		F32 m_02_, F32 m_12_, F32 m_22_, F32 m_32_,
		F32 m_03_, F32 m_13_, F32 m_23_, F32 m_33_
	) noexcept :
		Columns_{
			{ m_00_, m_10_, m_20_, m_30_ },
			{ m_01_, m_11_, m_21_, m_31_ },
			{ m_02_, m_12_, m_22_, m_32_ },
			{ m_03_, m_13_, m_23_, m_33_ },
		} {}
	_LUMINA_INLINE_ F32x4x4<COLUMN_MAJOR>::F32x4x4(
		F32x4::IN col_0_,
		F32x4::IN col_1_,
		F32x4::IN col_2_,
		F32x4::IN col_3_
	) noexcept :
		Columns_{ col_0_, col_1_, col_2_, col_3_, } {}
	_LUMINA_INLINE_ F32x4x4<COLUMN_MAJOR>::F32x4x4(
		F32x4 const cols_[4]
	) noexcept :
		Columns_{ cols_[0], cols_[1], cols_[2], cols_[3], } {}

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	constexpr F32x4x4<COLUMN_MAJOR>::~F32x4x4() noexcept {}

	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	F32x4x4<COLUMN_MAJOR> const F32x4x4<COLUMN_MAJOR>::Identity{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
}