module;

#include"../.define"

//////	//////	//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Math : LinearAlgebra.Vector;
//////	//////	//////	//////	//////	//////	//////	//////	//////

import : Fundamental.Exponentiation;
import : Fundamental.Trigonometry;

import Lumina.Core.Common;

//****	******	******	******	******	******	******	******	****//

//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
//..''	''..''	''..''	''..''	''..''	''..''	''..''	''..''	''..//
/// @class		Lumina::Math::F32x2
/// @brief		<span>2D 32-bit Float Vector</span>
//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//
/// @class		Lumina::Math::F32x3
/// @brief		<span>3D 32-bit Float Vector</span>
//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//
/// @class		Lumina::Math::F32x4
/// @brief		<span>4D 32-bit Float Vector</span>
/// @details
/// ### References
/// 1. https://github.com/microsoft/DirectXMath
/// 2. https://github.com/pelletier/vector3/blob/master/vector3.h
//''..	..''..	..''..	..''..	..''..	..''..	..''..	..''..	..''//
//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

namespace Lumina::Math {
	export class [[nodiscard]] F32x2;
	export class [[nodiscard]] F32x3;
	export class [[nodiscard]] F32x4;
}

namespace Lumina::Math {
	namespace {
		template<U32 Dimension>
		concept IsValidVectorDimension = ((2U <= Dimension) && (Dimension <= 4U));
	}
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	F32x2													<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

namespace Lumina::Math {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Definition												:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	class [[nodiscard]] F32x2 {

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Component Accessors										--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr auto [[nodiscard]] operator[](U32 idx_)
			noexcept -> F32& { return { *((&X) + idx_) }; }
		constexpr auto [[nodiscard]] operator[](U32 idx_)
			const noexcept -> F32 { return { *((&X) + idx_) }; }

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		constexpr auto [[nodiscard]] operator()()
			noexcept -> F32* { return { &X }; }
		constexpr auto [[nodiscard]] operator()()
			const noexcept -> F32 const* { return { &X }; }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr friend auto operator+(F32x2 const& lhs_, F32x2 const& rhs_) noexcept -> F32x2;
		constexpr friend auto operator-(F32x2 const& lhs_, F32x2 const& rhs_) noexcept -> F32x2;
		constexpr friend auto operator*(F32 lhs_, F32x2 const& rhs_) noexcept -> F32x2;
		constexpr friend auto operator*(F32x2 const& lhs_, F32 rhs_) noexcept -> F32x2;
		constexpr friend auto operator/(F32x2 const& lhs_, F32 rhs_) noexcept -> F32x2;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		constexpr auto operator+=(F32x2 const& rhs_) noexcept -> F32x2& {
			X += rhs_.X;
			Y += rhs_.Y;
			return { *this };
		}
		constexpr auto operator-=(F32x2 const& rhs_) noexcept -> F32x2& {
			X -= rhs_.X;
			Y -= rhs_.Y;
			return { *this };
		}
		constexpr auto operator*=(F32 rhs_) noexcept -> F32x2& {
			X *= rhs_;
			Y *= rhs_;
			return { *this };
		}
		constexpr auto operator/=(F32 rhs_) noexcept -> F32x2& {
			X /= rhs_;
			Y /= rhs_;
			return { *this };
		}

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Dot Product, Cross Product								--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr static auto [[nodiscard]]
			Dot(F32x2 const& lhs_, F32x2 const& rhs_)
			noexcept -> F32 { return { lhs_.X * rhs_.X + lhs_.Y * rhs_.Y }; }
		constexpr auto [[nodiscard]]
			Dot(F32x2 const& other_)
			const noexcept -> F32 { return Dot(*this, other_); }

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//
		
	public:
		constexpr static auto [[nodiscard]]
			Cross(F32x2 const& lhs_, F32x2 const& rhs_)
			noexcept -> F32 { return { lhs_.X * rhs_.Y - lhs_.Y * rhs_.X }; }
		constexpr auto [[nodiscard]]
			Cross(F32x2 const& other_)
			const noexcept -> F32 { return Cross(*this, other_); }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Norm, Unit												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		inline auto [[nodiscard]] Norm() const noexcept -> F32 {
			return SQRT(X * X + Y * Y);
		}
		inline auto [[nodiscard]] Unit() const noexcept -> F32x2 {
			F32 const inv_Norm{ 1.0f / Norm() };
			return F32x2{ X * inv_Norm, Y * inv_Norm };
		}

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Constructors											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr F32x2(F32 x_ = 0.0f, F32 y_ = 0.0f) noexcept :
			X{ x_ },
			Y{ y_ } {}
		constexpr F32x2(F32 const xy_[2]) noexcept :
			X{ xy_[0] },
			Y{ xy_[1] } {}

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	public:
		F32 X, Y;
	};

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Implementation											:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	export constexpr auto operator+(
		F32x2 const& lhs_,
		F32x2 const& rhs_
	) noexcept -> F32x2 {
		return F32x2{ lhs_.X + rhs_.X, lhs_.Y + rhs_.Y };
	}
	export constexpr auto operator-(
		F32x2 const& lhs_,
		F32x2 const& rhs_
	) noexcept -> F32x2 {
		return F32x2{ lhs_.X - rhs_.X, lhs_.Y - rhs_.Y };
	}
	export constexpr auto operator*(
		F32 lhs_,
		F32x2 const& rhs_
	) noexcept -> F32x2 {
		return F32x2{ lhs_ * rhs_.X, lhs_ * rhs_.Y };
	}
	export constexpr auto operator*(
		F32x2 const& lhs_,
		F32 rhs_
	) noexcept -> F32x2 {
		return F32x2{ lhs_.X * rhs_, lhs_.Y * rhs_ };
	}
	export constexpr auto operator/(
		F32x2 const& lhs_,
		F32 rhs_
	) noexcept -> F32x2 {
		F32 const inv_RHS{ 1.0f / rhs_ };
		return F32x2{ lhs_.X * inv_RHS, lhs_.Y * inv_RHS };
	}
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	F32x3													<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

namespace Lumina::Math {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Definition												:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	class [[nodiscard]] F32x3 {

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Component Accessors										--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr auto [[nodiscard]] operator[](U32 idx_)
			noexcept -> F32& { return { *((&X) + idx_) }; }
		constexpr auto [[nodiscard]] operator[](U32 idx_)
			const noexcept -> F32 { return { *((&X) + idx_) }; }

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		constexpr auto [[nodiscard]] operator()()
			noexcept -> F32* { return { &X }; }
		constexpr auto [[nodiscard]] operator()()
			const noexcept -> F32 const* { return { &X }; }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr friend auto operator+(F32x3 const& lhs_, F32x3 const& rhs_) noexcept -> F32x3;
		constexpr friend auto operator-(F32x3 const& lhs_, F32x3 const& rhs_) noexcept -> F32x3;
		constexpr friend auto operator*(F32 lhs_, F32x3 const& rhs_) noexcept -> F32x3;
		constexpr friend auto operator*(F32x3 const& lhs_, F32 rhs_) noexcept -> F32x3;
		constexpr friend auto operator/(F32x3 const& lhs_, F32 rhs_) noexcept -> F32x3;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		constexpr auto operator+=(F32x3 const& rhs_) noexcept -> F32x3& {
			X += rhs_.X;
			Y += rhs_.Y;
			Z += rhs_.Z;
			return { *this };
		}
		constexpr auto operator-=(F32x3 const& rhs_) noexcept -> F32x3& {
			X -= rhs_.X;
			Y -= rhs_.Y;
			Z -= rhs_.Z;
			return { *this };
		}
		constexpr auto operator*=(F32 rhs_) noexcept -> F32x3& {
			X *= rhs_;
			Y *= rhs_;
			Z *= rhs_;
			return { *this };
		}
		constexpr auto operator/=(F32 rhs_) noexcept -> F32x3& {
			X /= rhs_;
			Y /= rhs_;
			Z /= rhs_;
			return { *this };
		}

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Dot Product, Cross Product								--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr static auto [[nodiscard]]
			Dot(F32x3 const& lhs_, F32x3 const& rhs_)
			noexcept -> F32 { return { lhs_.X * rhs_.X + lhs_.Y * rhs_.Y + lhs_.Z * rhs_.Z }; }
		constexpr auto [[nodiscard]]
			Dot(F32x3 const& other_)
			const noexcept -> F32 { return Dot(*this, other_); }

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//
		
	public:
		constexpr static auto [[nodiscard]]
			Cross(F32x3 const& lhs_, F32x3 const& rhs_) noexcept -> F32x3 {
			return F32x3{
				lhs_.Y * rhs_.Z - lhs_.Z * rhs_.Y,
				lhs_.Z * rhs_.X - lhs_.X * rhs_.Z,
				lhs_.X * rhs_.Y - lhs_.Y * rhs_.X
			};
		}
		constexpr auto [[nodiscard]]
			Cross(F32x3 const& other_) const noexcept-> F32x3 {
			return Cross(*this, other_);
		}

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Norm, Unit												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		inline auto [[nodiscard]] Norm() const noexcept -> F32 {
			return SQRT(X * X + Y * Y + Z * Z);
		}
		inline auto [[nodiscard]] Unit() const noexcept -> F32x3 {
			F32 const inv_Norm{ 1.0f / Norm() };
			return F32x3{ X * inv_Norm, Y * inv_Norm, Z * inv_Norm };
		}

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Constructors											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr F32x3(F32 x_ = 0.0f, F32 y_ = 0.0f, F32 z_ = 0.0f) noexcept :
			X{ x_ },
			Y{ y_ },
			Z{ z_ } {}
		constexpr F32x3(F32 const xyz_[3]) noexcept :
			X{ xyz_[0] },
			Y{ xyz_[1] },
			Z{ xyz_[2] } {}
		constexpr F32x3(F32x2 const& xy_, F32 z_ = 0.0f) noexcept :
			X{ xy_[0] },
			Y{ xy_[1] },
			Z{ z_ } {}

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	public:
		F32 X, Y, Z;
	};

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Implementation											:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	export constexpr auto operator+(
		F32x3 const& lhs_,
		F32x3 const& rhs_
	) noexcept -> F32x3 {
		return F32x3{
			lhs_.X + rhs_.X,
			lhs_.Y + rhs_.Y,
			lhs_.Z + rhs_.Z
		};
	}
	export constexpr auto operator-(
		F32x3 const& lhs_,
		F32x3 const& rhs_
	) noexcept -> F32x3 {
		return F32x3{
			lhs_.X - rhs_.X,
			lhs_.Y - rhs_.Y,
			lhs_.Z - rhs_.Z
		};
	}
	export constexpr auto operator*(
		F32 lhs_,
		F32x3 const& rhs_
	) noexcept -> F32x3 {
		return F32x3{
			lhs_ * rhs_.X,
			lhs_ * rhs_.Y,
			lhs_ * rhs_.Z
		};
	}
	export constexpr auto operator*(
		F32x3 const& lhs_,
		F32 rhs_
	) noexcept -> F32x3 {
		return F32x3{
			lhs_.X * rhs_,
			lhs_.Y * rhs_,
			lhs_.Z * rhs_
		};
	}
	export constexpr auto operator/(
		F32x3 const& lhs_,
		F32 rhs_
	) noexcept -> F32x3 {
		return F32x3{
			lhs_.X / rhs_,
			lhs_.Y / rhs_,
			lhs_.Z / rhs_
		};
	}
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	F32x4													<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

namespace Lumina::Math {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Definition												:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	class alignas(16LLU) [[nodiscard]] F32x4 {

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
		//##++	Type Aliases											++##//
		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	public:
		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		using OUT = F32x4&;
		using IN = F32x4 const;
		#else
		using OUT = F32x4&;
		using IN = F32x4 const&;
		#endif

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Cast Operators											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ explicit operator F32 const*() const noexcept;
		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		_LUMINA_INLINE_ explicit operator SIMD::F32x4() const noexcept;
		#endif

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Component Accessors										--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ auto X(F32 x_) & noexcept -> void;
		_LUMINA_INLINE_ auto Y(F32 y_) & noexcept -> void;
		_LUMINA_INLINE_ auto Z(F32 z_) & noexcept -> void;
		_LUMINA_INLINE_ auto W(F32 w_) & noexcept -> void;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		_LUMINA_INLINE_ auto [[nodiscard]] X() const& noexcept -> F32;
		_LUMINA_INLINE_ auto [[nodiscard]] Y() const& noexcept -> F32;
		_LUMINA_INLINE_ auto [[nodiscard]] Z() const& noexcept -> F32;
		_LUMINA_INLINE_ auto [[nodiscard]] W() const& noexcept -> F32;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		_LUMINA_INLINE_ auto [[nodiscard]]
			Set(U32 idx_, F32 val_)
			& noexcept -> void;
		_LUMINA_INLINE_ auto [[nodiscard]]
			Get(U32 idx_)
			const& noexcept -> F32;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator+(F32x4::IN lhs_, F32x4::IN rhs_) noexcept -> F32x4;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator-(F32x4::IN lhs_, F32x4::IN rhs_) noexcept -> F32x4;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator*(F32x4::IN lhs_, F32 rhs_) noexcept -> F32x4;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator*(F32 lhs_, F32x4::IN rhs_) noexcept -> F32x4;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator*(F32x4::IN lhs_, F32x4::IN rhs_) noexcept -> F32x4;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator/(F32x4::IN lhs_, F32 rhs_) noexcept -> F32x4;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator/(F32 lhs_, F32x4::IN rhs_) noexcept -> F32x4;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator/(F32x4::IN lhs_, F32x4::IN rhs_) noexcept -> F32x4;
		
		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator+=(F32x4::IN rhs_) noexcept -> F32x4::OUT;
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator-=(F32x4::IN rhs_) noexcept -> F32x4::OUT;
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator*=(F32 rhs_) noexcept -> F32x4::OUT;
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator/=(F32 rhs_) noexcept -> F32x4::OUT;
		
		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		_LUMINA_INLINE_ static auto _LUMINA_VECTORCALL_ MultiplyAdd(
			F32x4::IN op_MUL_LHS_,
			F32x4::IN op_MUL_RHS_,
			F32x4::IN op_ACC_
		) noexcept -> F32x4;
		_LUMINA_INLINE_ static auto _LUMINA_VECTORCALL_ MultiplySubtract(
			F32x4::IN op_MUL_LHS_,
			F32x4::IN op_MUL_RHS_,
			F32x4::IN op_ACC_
		) noexcept -> F32x4;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		template<B1 IsMasked_X, B1 IsMasked_Y, B1 IsMasked_Z, B1 IsMasked_W>
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			FlipSign() const noexcept -> F32x4;

		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			operator-() const noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Dot Product, Cross Product								--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		template<U32 Dimension>
			requires (IsValidVectorDimension<Dimension>)
		_LUMINA_INLINE_ static auto [[nodiscard]] _LUMINA_VECTORCALL_
			Dot(F32x4::IN lhs_, F32x4::IN rhs_) noexcept -> F32;
		template<U32 Dimension>
			requires (IsValidVectorDimension<Dimension>)
		_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_
			Dot(F32x4::IN other_) const noexcept -> F32;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	public:
		template<U32 Dimension>
			requires (IsValidVectorDimension<Dimension>)
		_LUMINA_INLINE_ static auto [[nodiscard]] _LUMINA_VECTORCALL_
			Cross(F32x4::IN lhs_, F32x4::IN rhs_) noexcept -> F32x4;
		template<U32 Dimension>
			requires (IsValidVectorDimension<Dimension>)
		_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_
			Cross(F32x4::IN other_) const noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Norm, Unit												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		template<U32 Dimension>
			requires (IsValidVectorDimension<Dimension>)
		_LUMINA_INLINE_ auto [[nodiscard]] Norm() const noexcept -> F32;

		template<U32 Dimension>
			requires (IsValidVectorDimension<Dimension>)
		_LUMINA_INLINE_ auto [[nodiscard]] Unit() const noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Trigonometry											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ auto SIN() const noexcept -> F32x4;
		_LUMINA_INLINE_ auto COS() const noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Swizzle, Shuffle										--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		template<U32 Mask>
			requires (Mask < 0x100U)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Swizzle() const noexcept -> F32x4;
		template<U32 Index_X, U32 Index_Y, U32 Index_Z, U32 Index_W>
			requires (
				(Index_X < 4U) &&
				(Index_Y < 4U) &&
				(Index_Z < 4U) &&
				(Index_W < 4U)
			)
		_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
			Swizzle() const noexcept -> F32x4;

		template<U32 Mask>
			requires (Mask < 0x100U)
		_LUMINA_INLINE_ static auto _LUMINA_VECTORCALL_
			Shuffle(F32x4::IN lhs_, F32x4::IN rhs_) noexcept -> F32x4;
		template<U32 Index_X, U32 Index_Y, U32 Index_Z, U32 Index_W>
			requires (
				(Index_X < 4U) &&
				(Index_Y < 4U) &&
				(Index_Z < 4U) &&
				(Index_W < 4U)
			)
		_LUMINA_INLINE_ static auto _LUMINA_VECTORCALL_
			Shuffle(F32x4::IN lhs_, F32x4::IN rhs_) noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

		_LUMINA_INLINE_ static auto _LUMINA_VECTORCALL_
			Zero() noexcept -> F32x4;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Constructors											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ F32x4() noexcept;
		_LUMINA_INLINE_ F32x4(F32 src_) noexcept;
		_LUMINA_INLINE_ F32x4(F32 x_, F32 y_, F32 z_ = 0.0f, F32 w_ = 0.0f) noexcept;
		_LUMINA_INLINE_ F32x4(F32 const xyzw_[4]) noexcept;
		_LUMINA_INLINE_ F32x4(F32x2 const& xy_, F32 z_ = 0.0f, F32 w_ = 0.0f) noexcept;
		_LUMINA_INLINE_ F32x4(F32x3 const& xyz_, F32 w_ = 0.0f) noexcept;
		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		_LUMINA_INLINE_ F32x4(SIMD::F32x4 xyzw_) noexcept;
		#endif

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	protected:
		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		SIMD::F32x4 XYZW_;
		#else
		F32 X_, Y_, Z_, W_;
		#endif

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

		#if !defined(_LUMINA_INTRINSICS_UNUSED_)

	protected:
		using ComponentSetter = auto(F32x4::*)(F32) & noexcept -> void;
		constexpr static ComponentSetter ComponentSetters_[4]{ &X, &Y, &Z, &W };

		using ComponentGetter = auto(F32x4::*)() const& noexcept -> F32;
		constexpr static ComponentGetter ComponentGetters_[4]{ &X, &Y, &Z, &W };

		#endif
	};

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Implementation											:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	#if !defined(_LUMINA_INTRINSICS_UNUSED_)
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
	
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Cast Operators											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ F32x4::operator F32 const*()
		const noexcept { return reinterpret_cast<F32 const*>(this); }
	_LUMINA_INLINE_ F32x4::operator SIMD::F32x4()
		const noexcept { return XYZW_; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Component Accessors										--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto F32x4::X(F32 x_)
		& noexcept -> void { XYZW_ = SIMD::Insert<0x0U, 0, 0>(XYZW_, SIMD::Scalar::Set(x_)); }
	_LUMINA_INLINE_ auto F32x4::Y(F32 y_)
		& noexcept -> void { XYZW_ = SIMD::Insert<0x0U, 1, 0>(XYZW_, SIMD::Scalar::Set(y_)); }
	_LUMINA_INLINE_ auto F32x4::Z(F32 z_)
		& noexcept -> void { XYZW_ = SIMD::Insert<0x0U, 2, 0>(XYZW_, SIMD::Scalar::Set(z_)); }
	_LUMINA_INLINE_ auto F32x4::W(F32 w_)
		& noexcept -> void { XYZW_ = SIMD::Insert<0x0U, 3, 0>(XYZW_, SIMD::Scalar::Set(w_)); }

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::X() const& noexcept -> F32 {
		return SIMD::Scalar::Convert<F32>(XYZW_);
	}
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Y() const& noexcept -> F32 {
		SIMD::F32x4 const yyyy{ SIMD::Shuffle<0x55U>(XYZW_, XYZW_) };
		return SIMD::Scalar::Convert<F32>(yyyy);
	}
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Z() const& noexcept -> F32 {
		SIMD::F32x4 const zzzz{ SIMD::Shuffle<0xAAU>(XYZW_, XYZW_) };
		return SIMD::Scalar::Convert<F32>(zzzz);
	}
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::W() const& noexcept -> F32 {
		SIMD::F32x4 const wwww{ SIMD::Shuffle<0xFFU>(XYZW_, XYZW_) };
		return SIMD::Scalar::Convert<F32>(wwww);
	}

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	_LUMINA_INLINE_ auto [[nodiscard]]
		F32x4::Set(U32 idx_, F32 val_)
		& noexcept -> void { (this->*(ComponentSetters_[idx_]))(val_); }
	_LUMINA_INLINE_ auto [[nodiscard]]
		F32x4::Get(U32 idx_)
		const& noexcept -> F32 { return (this->*(ComponentGetters_[idx_]))(); }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Arithmetic												--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator+(F32x4::IN lhs_, F32x4::IN rhs_)
		noexcept -> F32x4 { return SIMD::ADD(lhs_.XYZW_, rhs_.XYZW_); }
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator-(F32x4::IN lhs_, F32x4::IN rhs_)
		noexcept -> F32x4 { return SIMD::SUB(lhs_.XYZW_, rhs_.XYZW_); }
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator*(F32x4::IN lhs_, F32 rhs_)
		noexcept -> F32x4 { return SIMD::MUL(lhs_.XYZW_, SIMD::SetAll(rhs_)); }
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator*(F32 lhs_, F32x4::IN rhs_)
		noexcept -> F32x4 { return SIMD::MUL(SIMD::SetAll(lhs_), rhs_.XYZW_); }
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator*(F32x4::IN lhs_, F32x4::IN rhs_)
		noexcept -> F32x4 { return SIMD::MUL(lhs_.XYZW_, rhs_.XYZW_); }
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator/(F32x4::IN lhs_, F32 rhs_)
		noexcept -> F32x4 { return SIMD::DIV(lhs_.XYZW_, SIMD::SetAll(rhs_)); }
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator/(F32 lhs_, F32x4::IN rhs_)
		noexcept -> F32x4 { return SIMD::DIV(SIMD::SetAll(lhs_), rhs_.XYZW_); }
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator/(F32x4::IN lhs_, F32x4::IN rhs_)
		noexcept -> F32x4 { return SIMD::DIV(lhs_.XYZW_, rhs_.XYZW_); }

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::operator+=(
		F32x4::IN rhs_
	) noexcept -> F32x4::OUT {
		XYZW_ = SIMD::ADD(XYZW_, rhs_.XYZW_);
		return { *this };
	}
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::operator-=(
		F32x4::IN rhs_
	) noexcept -> F32x4::OUT {
		XYZW_ = SIMD::SUB(XYZW_, rhs_.XYZW_);
		return { *this };
	}
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::operator*=(
		F32 rhs_
	) noexcept -> F32x4::OUT {
		SIMD::F32x4 const operand_RHS{ SIMD::SetAll(rhs_) };
		XYZW_ = SIMD::MUL(XYZW_, operand_RHS);
		return { *this };
	}
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::operator/=(
		F32 rhs_
	) noexcept -> F32x4::OUT {
		SIMD::F32x4 const operand_RHS{ SIMD::SetAll(rhs_) };
		XYZW_ = SIMD::DIV(XYZW_, operand_RHS);
		return { *this };
	}

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::MultiplyAdd(
		F32x4::IN op_MUL_LHS_,
		F32x4::IN op_MUL_RHS_,
		F32x4::IN op_ACC_
	) noexcept -> F32x4 {
		return SIMD::FMADD(
			op_MUL_LHS_.XYZW_,
			op_MUL_RHS_.XYZW_,
			op_ACC_.XYZW_
		);
	}
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::MultiplySubtract(
		F32x4::IN op_MUL_LHS_,
		F32x4::IN op_MUL_RHS_,
		F32x4::IN op_ACC_
	) noexcept -> F32x4 {
		return SIMD::FMSUB(
			op_MUL_LHS_.XYZW_,
			op_MUL_RHS_.XYZW_,
			op_ACC_.XYZW_
		);
	}

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//
	
	template<B1 IsMasked_X, B1 IsMasked_Y, B1 IsMasked_Z, B1 IsMasked_W>
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::FlipSign() const noexcept -> F32x4 {
		return SIMD::XOR(
			XYZW_,
			SIMD::Mask::Sign<
				IsMasked_X,
				IsMasked_Y,
				IsMasked_Z,
				IsMasked_W
			>
		);
	}

	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::operator-() const noexcept -> F32x4 {
		return FlipSign<1, 1, 1, 1>();
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Dot Product, Cross Product								--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	template<U32 Dimension>
		requires (IsValidVectorDimension<Dimension>)
	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_ F32x4::Dot(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32 {
		constexpr U32 mask_OUT{ 0x1U };
		constexpr U32 mask_IN{ (1U << Dimension) - 1U };

		SIMD::F32x4 const dotProd{ SIMD::Dot<mask_OUT, mask_IN>(lhs_.XYZW_, rhs_.XYZW_)};
		return SIMD::Scalar::Convert<F32>(dotProd);
	}

	template<U32 Dimension>
		requires (IsValidVectorDimension<Dimension>)
	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_
		F32x4::Dot(F32x4::IN other_)
		const noexcept -> F32 { return Dot<Dimension>(*this, other_); }

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_ F32x4::Cross<3U>(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		SIMD::F32x4 const lhs_YZXW{ SIMD::Shuffle<1, 2, 0, 3>(lhs_.XYZW_, lhs_.XYZW_) };
		SIMD::F32x4 const rhs_ZXYW{ SIMD::Shuffle<2, 0, 1, 3>(rhs_.XYZW_, rhs_.XYZW_) };

		SIMD::F32x4 const lhs_ZXYW{ SIMD::Shuffle<2, 0, 1, 3>(lhs_.XYZW_, lhs_.XYZW_) };
		SIMD::F32x4 const rhs_YZXW{ SIMD::Shuffle<1, 2, 0, 3>(rhs_.XYZW_, rhs_.XYZW_) };

		SIMD::F32x4 const crossProd{
			SIMD::FNMADD(
				lhs_ZXYW,
				rhs_YZXW,
				SIMD::MUL(
					lhs_YZXW,
					rhs_ZXYW
				)
			)
		};
		return crossProd;
	}

	template<U32 Dimension>
		requires (IsValidVectorDimension<Dimension>)
	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_
		F32x4::Cross(F32x4::IN other_)
		const noexcept -> F32x4 { return Cross<Dimension>(*this, other_); }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Norm, Unit												--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	template<U32 Dimension>
		requires (IsValidVectorDimension<Dimension>)
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Norm() const noexcept -> F32 {
		constexpr U32 mask_OUT{ 0x1U };
		constexpr U32 mask_IN{ (1U << Dimension) - 1U };

		SIMD::F32x4 const sq_Norm{ SIMD::Dot<mask_OUT, mask_IN>(XYZW_, XYZW_) };
		SIMD::F32x4 const norm{ SIMD::SQRT(sq_Norm) };
		return SIMD::Scalar::Convert<F32>(norm);
	}

	template<U32 Dimension>
		requires (IsValidVectorDimension<Dimension>)
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Unit() const noexcept -> F32x4 {
		using SIMD::Flag::CMP;

		constexpr U32 mask{ (1U << Dimension) - 1U };
		SIMD::F32x4 const sq_Norm{ SIMD::Dot<mask, mask>(XYZW_, XYZW_) };
		SIMD::F32x4 const norm{ SIMD::SQRT(sq_Norm) };
		SIMD::F32x4 const zeroMask{ SIMD::Compare<CMP::NEQ_OQ>(SIMD::Zero(), norm) };

		SIMD::F32x4 unit{ SIMD::DIV(XYZW_, norm) };
		unit = SIMD::AND(unit, zeroMask);
		return unit;
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Trigonometry											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto F32x4::SIN() const noexcept -> F32x4 { return SIMD::SIN(XYZW_); }
	_LUMINA_INLINE_ auto F32x4::COS() const noexcept -> F32x4 { return SIMD::COS(XYZW_); }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Swizzle, Shuffle										--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	template<U32 Mask>
		requires (Mask < 0x100U)
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Swizzle() const noexcept -> F32x4 {
		return SIMD::Shuffle<Mask>(XYZW_, XYZW_);
	}
	template<U32 Index_X, U32 Index_Y, U32 Index_Z, U32 Index_W>
		requires (
			(Index_X < 4U) &&
			(Index_Y < 4U) &&
			(Index_Z < 4U) &&
			(Index_W < 4U)
		)
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Swizzle() const noexcept -> F32x4 {
		return SIMD::Shuffle<Index_X, Index_Y, Index_Z, Index_W>(XYZW_, XYZW_);
	}

	template<U32 Mask>
		requires (Mask < 0x100U)
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Shuffle(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return SIMD::Shuffle<Mask>(lhs_.XYZW_, rhs_.XYZW_);
	}
	template<U32 Index_X, U32 Index_Y, U32 Index_Z, U32 Index_W>
		requires (
			(Index_X < 4U) &&
			(Index_Y < 4U) &&
			(Index_Z < 4U) &&
			(Index_W < 4U)
		)
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Shuffle(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return SIMD::Shuffle<Index_X, Index_Y, Index_Z, Index_W>(lhs_.XYZW_, rhs_.XYZW_);
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Zero()
		noexcept -> F32x4 { return SIMD::Zero(); }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Constructors											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ F32x4::F32x4() noexcept :
		XYZW_{} {}
	_LUMINA_INLINE_ F32x4::F32x4(F32 src_) noexcept :
		XYZW_{ SIMD::SetAll(src_) } {}
	_LUMINA_INLINE_ F32x4::F32x4(F32 x_, F32 y_, F32 z_, F32 w_) noexcept :
		XYZW_{ SIMD::Set(w_, z_, y_, x_) } {}
	_LUMINA_INLINE_ F32x4::F32x4(F32 const xyzw_[4]) noexcept :
		XYZW_{ SIMD::LoadUnaligned(xyzw_) } {}
	_LUMINA_INLINE_ F32x4::F32x4(F32x2 const& xy_, F32 z_, F32 w_) noexcept :
		XYZW_{ SIMD::Set(w_, z_, xy_[1], xy_[0]) } {}
	_LUMINA_INLINE_ F32x4::F32x4(F32x3 const& xyz_, F32 w_) noexcept :
		XYZW_{ SIMD::Set(w_, xyz_[2], xyz_[1], xyz_[0]) } {}
	_LUMINA_INLINE_ F32x4::F32x4(SIMD::F32x4 xyzw_) noexcept :
		XYZW_{ xyzw_ } {}

	//--#-	!defined(_LUMINA_INTRINSICS_UNUSED_)
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
	#else
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Component Accessors										--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto F32x4::X(F32 x_) & noexcept -> void { X_ = x_; }
	_LUMINA_INLINE_ auto F32x4::Y(F32 y_) & noexcept -> void { Y_ = y_; }
	_LUMINA_INLINE_ auto F32x4::Z(F32 z_) & noexcept -> void { Z_ = z_; }
	_LUMINA_INLINE_ auto F32x4::W(F32 w_) & noexcept -> void { W_ = w_; }

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::X() const& noexcept -> F32 { return X_; }
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Y() const& noexcept -> F32 { return Y_; }
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Z() const& noexcept -> F32 { return Z_; }
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::W() const& noexcept -> F32 { return W_; }

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	_LUMINA_INLINE_ auto [[nodiscard]]
		F32x4::Set(U32 idx_, F32 val_)
		& noexcept -> void { *(&X_ + idx_) = val_; }
	_LUMINA_INLINE_ auto [[nodiscard]]
		F32x4::Get(U32 idx_)
		const& noexcept -> F32 { return { *(&X_ + idx_) }; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Arithmetic												--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator+(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_.X_ + rhs_.X_,
			lhs_.Y_ + rhs_.Y_,
			lhs_.Z_ + rhs_.Z_,
			lhs_.W_ + rhs_.W_
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator-(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_.X_ - rhs_.X_,
			lhs_.Y_ - rhs_.Y_,
			lhs_.Z_ - rhs_.Z_,
			lhs_.W_ - rhs_.W_
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator*(
		F32x4::IN lhs_,
		F32 rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_.X_ * rhs_,
			lhs_.Y_ * rhs_,
			lhs_.Z_ * rhs_,
			lhs_.W_ * rhs_
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator*(
		F32 lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_ * rhs_.X_,
			lhs_ * rhs_.Y_,
			lhs_ * rhs_.Z_,
			lhs_ * rhs_.W_
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator*(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_.X_ * rhs_.X_,
			lhs_.Y_ * rhs_.Y_,
			lhs_.Z_ * rhs_.Z_,
			lhs_.W_ * rhs_.W_
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator/(
		F32x4::IN lhs_,
		F32 rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_.X_ / rhs_,
			lhs_.Y_ / rhs_,
			lhs_.Z_ / rhs_,
			lhs_.W_ / rhs_
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator/(
		F32 lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_ / rhs_.X_,
			lhs_ / rhs_.Y_,
			lhs_ / rhs_.Z_,
			lhs_ / rhs_.W_
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator/(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_.X_ / rhs_.X_,
			lhs_.Y_ / rhs_.Y_,
			lhs_.Z_ / rhs_.Z_,
			lhs_.W_ / rhs_.W_
		};
	}

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//
	
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::operator+=(
		F32x4::IN const& rhs_
	) noexcept -> F32x4::OUT {
		X_ += rhs_.X_;
		Y_ += rhs_.Y_;
		Z_ += rhs_.Z_;
		W_ += rhs_.W_;
		return { *this };
	}
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::operator-=(
		F32x4::IN const& rhs_
	) noexcept -> F32x4::OUT {
		X_ -= rhs_.X_;
		Y_ -= rhs_.Y_;
		Z_ -= rhs_.Z_;
		W_ -= rhs_.W_;
		return { *this };
	}
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::operator*=(
		F32 rhs_
	) noexcept -> F32x4::OUT {
		X_ *= rhs_;
		Y_ *= rhs_;
		Z_ *= rhs_;
		W_ *= rhs_;
		return { *this };
	}
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::operator/=(
		F32 rhs_
	) noexcept -> F32x4::OUT {
		X_ /= rhs_;
		Y_ /= rhs_;
		Z_ /= rhs_;
		W_ /= rhs_;
		return { *this };
	}

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		F32x4::operator-()
		const noexcept -> F32x4 { return F32x4{ -X_, -Y_, -Z_, -W_ }; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Dot Product, Cross Product								--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_ F32x4::Dot<2U>(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32 {
		return {
			lhs_.X_ * rhs_.X_ +
			lhs_.Y_ * rhs_.Y_
		};
	}
	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_ F32x4::Dot<3U>(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32 {
		return {
			lhs_.X_ * rhs_.X_ +
			lhs_.Y_ * rhs_.Y_ +
			lhs_.Z_ * rhs_.Z_
		};
	}
	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_ F32x4::Dot<4U>(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32 {
		return {
			lhs_.X_ * rhs_.X_ +
			lhs_.Y_ * rhs_.Y_ +
			lhs_.Z_ * rhs_.Z_ +
			lhs_.W_ * rhs_.W_
		};
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Norm, Unit												--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	
	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Norm<2U>()
		const noexcept -> F32 { return SQRT(X_ * X_ + Y_ * Y_); }
	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Norm<3U>()
		const noexcept -> F32 { return SQRT(X_ * X_ + Y_ * Y_ + Z_ * Z_); }
	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Norm<4U>()
		const noexcept -> F32 { return SQRT(X_ * X_ + Y_ * Y_ + Z_ * Z_ + W_ * W_); }

	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Unit<2U>() const noexcept -> F32x4 {
		F32 const inv_Norm{ 1.0f / Norm<2U>() };
		return F32x4{ X_ * inv_Norm, Y_ * inv_Norm };
	}
	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Unit<3U>() const noexcept -> F32x4 {
		F32 const inv_Norm{ 1.0f / Norm<3U>() };
		return F32x4{ X_ * inv_Norm, Y_ * inv_Norm, Z_ * inv_Norm };
	}
	template<>
	_LUMINA_INLINE_ auto [[nodiscard]] F32x4::Unit<4U>() const noexcept -> F32x4 {
		F32 const inv_Norm{ 1.0f / Norm<4U>() };
		return F32x4{ X_ * inv_Norm, Y_ * inv_Norm, Z_ * inv_Norm, W_ * inv_Norm };
	}
	
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Trigonometry											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto F32x4::SIN() const noexcept -> F32x4 {
		return F32x4{
			Math::SIN(X_),
			Math::SIN(Y_),
			Math::SIN(Z_),
			Math::SIN(W_)
		};
	}
	_LUMINA_INLINE_ auto F32x4::COS() const noexcept -> F32x4 {
		return F32x4{
			Math::COS(X_),
			Math::COS(Y_),
			Math::COS(Z_),
			Math::COS(W_)
		};
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Swizzle, Shuffle										--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	template<U32 Mask>
		requires (Mask < 0x100U)
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Swizzle() const noexcept -> F32x4 {
		return F32x4{
			Get(Mask & 0x3U),
			Get((Mask >> 2U) & 0x3U),
			Get((Mask >> 4U) & 0x3U),
			Get((Mask >> 6U) & 0x3U),
		};
	}
	template<U32 Index_X, U32 Index_Y, U32 Index_Z, U32 Index_W>
		requires (
			(Index_X < 4U) &&
			(Index_Y < 4U) &&
			(Index_Z < 4U) &&
			(Index_W < 4U)
		)
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Swizzle() const noexcept -> F32x4 {
		return F32x4{
			Get(Index_X),
			Get(Index_Y),
			Get(Index_Z),
			Get(Index_W),
		};
	}

	template<U32 Mask>
		requires (Mask < 0x100U)
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Shuffle(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_.Get(Mask & 0x3U),
			lhs_.Get((Mask >> 2U) & 0x3U),
			rhs_.Get((Mask >> 4U) & 0x3U),
			rhs_.Get((Mask >> 6U) & 0x3U),
		};
	}
	template<U32 Index_X, U32 Index_Y, U32 Index_Z, U32 Index_W>
		requires (
			(Index_X < 4U) &&
			(Index_Y < 4U) &&
			(Index_Z < 4U) &&
			(Index_W < 4U)
		)
	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Shuffle(
		F32x4::IN lhs_,
		F32x4::IN rhs_
	) noexcept -> F32x4 {
		return F32x4{
			lhs_.Get(Index_X),
			lhs_.Get(Index_Y),
			rhs_.Get(Index_Z),
			rhs_.Get(Index_W),
		};
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ F32x4::Zero()
		noexcept -> F32x4 { return F32x4{ 0.0f, 0.0f, 0.0f, 0.0f }; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Constructors											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ F32x4::F32x4() noexcept :
		X_{}, Y_{}, Z_{}, W_{} {}
	_LUMINA_INLINE_ F32x4::F32x4(F32 src_) noexcept :
		X_{ src_ }, Y_{ src_ }, Z_{ src_ }, W_{ src_ } {}
	_LUMINA_INLINE_ F32x4::F32x4(F32 x_, F32 y_, F32 z_, F32 w_) noexcept :
		X_{ x_ }, Y_{ y_ }, Z_{ z_ }, W_{ w_ } {}
	_LUMINA_INLINE_ F32x4::F32x4(F32 const xyzw_[4]) noexcept :
		X_{ xyzw_[0] }, Y_{ xyzw_[1] }, Z_{ xyzw_[2] }, W_{ xyzw_[3] } {}
	_LUMINA_INLINE_ F32x4::F32x4(F32x2 const& xy_, F32 z_, F32 w_) noexcept :
		X_{ xy_[0] }, Y_{ xy_[1] }, Z_{ z_ }, W_{ w_ } {}
	_LUMINA_INLINE_ F32x4::F32x4(F32x3 const& xyz_, F32 w_) noexcept :
		X_{ xyz_[0] }, Y_{ xyz_[1] }, Z_{ xyz_[2] }, W_{ w_ } {}

	//--#-	defined(_LUMINA_INTRINSICS_UNUSED_)
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
	#endif
}