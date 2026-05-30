module;

#include"../.define"

//////	//////	//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Math : NumberSystem.Quaternion;
//////	//////	//////	//////	//////	//////	//////	//////	//////

import : Fundamental.Exponentiation;

import Lumina.Core.Common;

//****	******	******	******	******	******	******	******	****//

namespace Lumina::Math {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Definition												:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	export class alignas(16LLU) [[nodiscard]] Quaternion {

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
		//##++	Type Aliases											++##//
		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		using IN = Quaternion;
		using OUT = Quaternion&;
		#else
		using IN = Quaternion const&;
		using OUT = Quaternion&;
		#endif

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

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Arithmetic												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator+(Quaternion::IN lhs_, Quaternion::IN rhs_)
			noexcept -> Quaternion;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator*(Quaternion::IN lhs_, Quaternion::IN rhs_)
			noexcept -> Quaternion;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator*(Quaternion::IN lhs_, F32 rhs_)
			noexcept -> Quaternion;
		_LUMINA_INLINE_ friend auto _LUMINA_VECTORCALL_
			operator*(F32 lhs_, Quaternion::IN rhs_)
			noexcept -> Quaternion;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Dot Product, Cross Product								--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ static auto [[nodiscard]] _LUMINA_VECTORCALL_
			Dot(Quaternion::IN lhs_, Quaternion::IN rhs_)
			noexcept -> F32;
		_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_
			Dot(Quaternion::IN other_)
			const noexcept -> F32;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

		_LUMINA_INLINE_ static auto [[nodiscard]] _LUMINA_VECTORCALL_
			Cross(Quaternion::IN lhs_, Quaternion::IN rhs_)
			noexcept -> Quaternion;
		_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_
			Cross(Quaternion::IN other_)
			const noexcept -> Quaternion;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Norm, Unit, Conjugate, Reciprocal						--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ auto [[nodiscard]] Norm() const noexcept -> F32;
		_LUMINA_INLINE_ auto [[nodiscard]] Unit() const noexcept -> Quaternion;
		_LUMINA_INLINE_ auto [[nodiscard]] Conjugate() const noexcept -> Quaternion;
		_LUMINA_INLINE_ auto [[nodiscard]] Reciprocal() const noexcept -> Quaternion;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Constructors, Destructor								--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		_LUMINA_INLINE_ Quaternion() noexcept;
		_LUMINA_INLINE_ Quaternion(F32 x_, F32 y_, F32 z_, F32 w_) noexcept;
		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		_LUMINA_INLINE_ Quaternion(SIMD::F32x4 xyzw_) noexcept;
		#endif

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	protected:
		#if !defined(_LUMINA_INTRINSICS_UNUSED_)
		SIMD::F32x4 XYZW_;
		#else
		F32 X_, Y_, Z_, W_;
		#endif

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	public:
		Quaternion static const Identity;
	};

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Implementation											:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	#if !defined(_LUMINA_INTRINSICS_UNUSED_)
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
	
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Cast Operators											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ Quaternion::operator F32 const*()
		const noexcept { return reinterpret_cast<F32 const*>(this); }
	_LUMINA_INLINE_ Quaternion::operator SIMD::F32x4()
		const noexcept { return XYZW_; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Component Accessors										--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto Quaternion::X(F32 x_)
		& noexcept -> void { XYZW_ = SIMD::Insert<0x0U, 0, 0>(XYZW_, SIMD::Scalar::Set(x_)); }
	_LUMINA_INLINE_ auto Quaternion::Y(F32 y_)
		& noexcept -> void { XYZW_ = SIMD::Insert<0x0U, 1, 0>(XYZW_, SIMD::Scalar::Set(y_)); }
	_LUMINA_INLINE_ auto Quaternion::Z(F32 z_)
		& noexcept -> void { XYZW_ = SIMD::Insert<0x0U, 2, 0>(XYZW_, SIMD::Scalar::Set(z_)); }
	_LUMINA_INLINE_ auto Quaternion::W(F32 w_)
		& noexcept -> void { XYZW_ = SIMD::Insert<0x0U, 3, 0>(XYZW_, SIMD::Scalar::Set(w_)); }

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::X() const& noexcept -> F32 {
		return SIMD::Scalar::Convert<F32>(XYZW_);
	}
	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::Y() const& noexcept -> F32 {
		SIMD::F32x4 const yyyy{ SIMD::Shuffle<0x55U>(XYZW_, XYZW_) };
		return SIMD::Scalar::Convert<F32>(yyyy);
	}
	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::Z() const& noexcept -> F32 {
		SIMD::F32x4 const zzzz{ SIMD::Shuffle<0xAAU>(XYZW_, XYZW_) };
		return SIMD::Scalar::Convert<F32>(zzzz);
	}
	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::W() const& noexcept -> F32 {
		SIMD::F32x4 const wwww{ SIMD::Shuffle<0xFFU>(XYZW_, XYZW_) };
		return SIMD::Scalar::Convert<F32>(wwww);
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Arithmetic												--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator+(
		Quaternion::IN lhs_,
		Quaternion::IN rhs_
	) noexcept -> Quaternion {
		return SIMD::ADD(lhs_.XYZW_, rhs_.XYZW_);
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator*(
		Quaternion::IN lhs_,
		Quaternion::IN rhs_
	) noexcept -> Quaternion {
		SIMD::F32x4 ret{};

		//	x = x0w1 + y0z1 + z0y1 + w0x1
		//	y = x0z1 + y0w1 + z0x1 + w0y1
		//	z = x0y1 + y0x1 + z0w1 + w0z1
		//	w = x0x1 + y0y1 + z0z1 + w0w1
		{
			SIMD::F32x4 const lhs_XXXX{ SIMD::Shuffle<0, 0, 0, 0>(lhs_.XYZW_, lhs_.XYZW_) };
			SIMD::F32x4 const lhs_YYYY{ SIMD::Shuffle<1, 1, 1, 1>(lhs_.XYZW_, lhs_.XYZW_) };
			SIMD::F32x4 const lhs_ZZZZ{ SIMD::Shuffle<2, 2, 2, 2>(lhs_.XYZW_, lhs_.XYZW_) };
			SIMD::F32x4 const lhs_WWWW{ SIMD::Shuffle<3, 3, 3, 3>(lhs_.XYZW_, lhs_.XYZW_) };

			SIMD::F32x4 const rhs_WZYX{ SIMD::Shuffle<3, 2, 1, 0>(rhs_.XYZW_, rhs_.XYZW_) };
			SIMD::F32x4 const rhs_ZWXY{ SIMD::Shuffle<2, 3, 0, 1>(rhs_.XYZW_, rhs_.XYZW_) };
			SIMD::F32x4 const rhs_YXWZ{ SIMD::Shuffle<1, 0, 3, 2>(rhs_.XYZW_, rhs_.XYZW_) };

			ret = SIMD::ADD(
				SIMD::ADD(
					SIMD::MUL(lhs_XXXX, rhs_WZYX),
					SIMD::MUL(lhs_YYYY, rhs_ZWXY)
				),
				SIMD::ADD(
					SIMD::MUL(lhs_ZZZZ, rhs_YXWZ),
					SIMD::MUL(lhs_WWWW, rhs_.XYZW_)
				)
			);
		}

		//	x = x0w1 + y0z1 + (-z0y1) + w0x1
		//	y = (-x0z1) + y0w1 + z0x1 + w0y1
		//	z = x0y1 + (-y0x1) + z0w1 + w0z1
		//	w = x0x1 + y0y1 + z0z1 + (-w0w1)
		{
			SIMD::F32x4 const lhs_ZXYW{ SIMD::Shuffle<2, 0, 1, 3>(lhs_.XYZW_, lhs_.XYZW_) };
			SIMD::F32x4 const rhs_YZXW{ SIMD::Shuffle<1, 2, 0, 3>(rhs_.XYZW_, rhs_.XYZW_) };
			SIMD::F32x4 tmp{ SIMD::MUL(lhs_ZXYW, rhs_YZXW) };
			tmp = SIMD::ADD(tmp, tmp);

			ret = SIMD::SUB(ret, tmp);
		}

		//	x = x0w1 + y0z1 + (-z0y1) + w0x1
		//	y = (-x0z1) + y0w1 + z0x1 + w0y1
		//	z = x0y1 + (-y0x1) + z0w1 + w0z1
		//	w = (-x0x1) + (-y0y1) + (-z0z1) + w0w1
		{
			ret = SIMD::XOR(ret, SIMD::Mask::Sign<0, 0, 0, 1>);
		}

		return ret;
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator*(Quaternion::IN lhs_, F32 rhs_)
		noexcept -> Quaternion { return SIMD::MUL(lhs_.XYZW_, SIMD::SetAll(rhs_)); }
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_
		operator*(F32 lhs_, Quaternion::IN rhs_)
		noexcept -> Quaternion { return SIMD::MUL(SIMD::SetAll(lhs_), rhs_.XYZW_); }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Dot Product, Cross Product								--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_ Quaternion::Dot(
		Quaternion::IN lhs_,
		Quaternion::IN rhs_
	) noexcept -> F32 {
		SIMD::F32x4 const dotProd{ SIMD::Dot<0x1U, 0xFU>(lhs_.XYZW_, rhs_.XYZW_) };
		return SIMD::Scalar::Convert<F32>(dotProd);
	}
	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_ Quaternion::Dot(
		Quaternion::IN other_
	) const noexcept -> F32 {
		return Dot(*this, other_);
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Norm, Unit, Conjugate, Reciprocal						--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::Norm() const noexcept -> F32 {
		SIMD::F32x4 const sq_Norm{ SIMD::Dot<0x1U, 0xFU>(XYZW_, XYZW_) };
		SIMD::F32x4 const norm{ SIMD::SQRT(sq_Norm) };
		return SIMD::Scalar::Convert<F32>(norm);
	}

	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::Unit() const noexcept -> Quaternion {
		using SIMD::Flag::CMP;

		SIMD::F32x4 const sq_Norm{ SIMD::Dot<0xFU, 0xFU>(XYZW_, XYZW_) };
		SIMD::F32x4 const norm{ SIMD::SQRT(sq_Norm) };
		SIMD::F32x4 const zeroMask{ SIMD::Compare<CMP::NEQ_OQ>(SIMD::Zero(), norm) };

		SIMD::F32x4 unit{ SIMD::DIV(XYZW_, norm) };
		unit = SIMD::AND(unit, zeroMask);
		return unit;
	}

	_LUMINA_INLINE_ auto Quaternion::Conjugate() const noexcept -> Quaternion {
		return SIMD::XOR(XYZW_, SIMD::Mask::Sign<1, 1, 1, 0>);
	}

	_LUMINA_INLINE_ auto Quaternion::Reciprocal() const noexcept -> Quaternion {
		using SIMD::Flag::CMP;

		SIMD::F32x4 const conj{ SIMD::XOR(XYZW_, SIMD::Mask::Sign<1, 1, 1, 0>) };
		SIMD::F32x4 const sq_Norm{ SIMD::Dot<0xFU, 0xFU>(XYZW_, XYZW_) };
		SIMD::F32x4 const zeroMask{ SIMD::Compare<CMP::NEQ_OQ>(SIMD::Zero(), sq_Norm) };

		SIMD::F32x4 recip{ SIMD::DIV(conj, sq_Norm) };
		recip = SIMD::AND(recip, zeroMask);
		return recip;
	}

	_LUMINA_INLINE_ Quaternion::Quaternion()
		noexcept = default;
	_LUMINA_INLINE_ Quaternion::Quaternion(F32 x_, F32 y_, F32 z_, F32 w_)
		noexcept : XYZW_{ SIMD::Set(w_, z_, y_, x_) } {}
	_LUMINA_INLINE_ Quaternion::Quaternion(SIMD::F32x4 xyzw_)
		noexcept : XYZW_{ xyzw_} {}

	//--#-	!defined(_LUMINA_INTRINSICS_UNUSED_)
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
	#else
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Cast Operators											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ Quaternion::operator F32 const*()
		const noexcept { return { &X_ }; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Component Accessors										--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto Quaternion::X(F32 x_) & noexcept -> void { X_ = x_; }
	_LUMINA_INLINE_ auto Quaternion::Y(F32 y_) & noexcept -> void { Y_ = y_; }
	_LUMINA_INLINE_ auto Quaternion::Z(F32 z_) & noexcept -> void { Z_ = z_; }
	_LUMINA_INLINE_ auto Quaternion::W(F32 w_) & noexcept -> void { W_ = w_; }

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::X() const& noexcept -> F32 { return X_; }
	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::Y() const& noexcept -> F32 { return Y_; }
	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::Z() const& noexcept -> F32 { return Z_; }
	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::W() const& noexcept -> F32 { return W_; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Arithmetic												--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator+(
		Quaternion::IN lhs_,
		Quaternion::IN rhs_
	) noexcept -> Quaternion {
		return Quaternion{
			lhs_.X_ + rhs_.X_,
			lhs_.Y_ + rhs_.Y_,
			lhs_.Z_ + rhs_.Z_,
			lhs_.W_ + rhs_.W_
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator*(
		Quaternion::IN lhs_,
		Quaternion::IN rhs_
	) noexcept -> Quaternion {
		return Quaternion{
			(lhs_.W_ * rhs_.X_ + lhs_.X_ * rhs_.W_) + (lhs_.Y_ * rhs_.Z_ - lhs_.Z_ * rhs_.Y_),
			(lhs_.W_ * rhs_.Y_ + lhs_.Y_ * rhs_.W_) + (lhs_.Z_ * rhs_.X_ - lhs_.X_ * rhs_.Z_),
			(lhs_.W_ * rhs_.Z_ + lhs_.Z_ * rhs_.W_) + (lhs_.X_ * rhs_.Y_ - lhs_.Y_ * rhs_.X_),
			lhs_.W_ * rhs_.W_ - (lhs_.X_ * rhs_.X_ + lhs_.Y_ * rhs_.Y_ + lhs_.Z_ * rhs_.Z_)
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator*(
		Quaternion::IN lhs_,
		F32 rhs_
	) noexcept -> Quaternion {
		return Quaternion{
			lhs_.X_ * rhs_,
			lhs_.Y_ * rhs_,
			lhs_.Z_ * rhs_,
			lhs_.W_ * rhs_
		};
	}
	export _LUMINA_INLINE_ auto _LUMINA_VECTORCALL_ operator*(
		F32 lhs_,
		Quaternion::IN rhs_
	) noexcept -> Quaternion {
		return Quaternion{
			lhs_ * rhs_.X_,
			lhs_ * rhs_.Y_,
			lhs_ * rhs_.Z_,
			lhs_ * rhs_.W_
		};
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Dot Product & Cross Product								--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_ Quaternion::Dot(
		Quaternion::IN lhs_,
		Quaternion::IN rhs_
	) noexcept -> F32 {
		return {
			lhs_.X_ * rhs_.X_ +
			lhs_.Y_ * rhs_.Y_ +
			lhs_.Z_ * rhs_.Z_ +
			lhs_.W_ * rhs_.W_
		};
	}
	_LUMINA_INLINE_ auto [[nodiscard]] _LUMINA_VECTORCALL_ Quaternion::Dot(
		Quaternion::IN other_
	) const noexcept -> F32 {
		return Dot(*this, other_);
	}

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Norm, Unit, Conjugate, Reciprocal						--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::Norm() const noexcept -> F32 {
		return SQRT(X_ * X_ + Y_ * Y_ + Z_ * Z_ + W_ * W_);
	}

	_LUMINA_INLINE_ auto [[nodiscard]] Quaternion::Unit() const noexcept -> Quaternion {
		F32 const inv_Norm{ 1.0f / Norm() };
		return Quaternion{ X_ * inv_Norm, Y_ * inv_Norm, Z_ * inv_Norm, W_ * inv_Norm };
	}

	_LUMINA_INLINE_ auto Quaternion::Conjugate() const noexcept -> Quaternion {
		return Quaternion{ -X_, -Y_, -Z_, W_ };
	}

	_LUMINA_INLINE_ auto Quaternion::Reciprocal() const noexcept -> Quaternion {
		F32 const inv_Norm{ 1.0f / Norm() };
		return Quaternion{ (-X_) * inv_Norm, (-Y_) * inv_Norm, (-Z_) * inv_Norm, W_ * inv_Norm };
	}

	_LUMINA_INLINE_ Quaternion::Quaternion()
		noexcept = default;
	_LUMINA_INLINE_ Quaternion::Quaternion(F32 x_, F32 y_, F32 z_, F32 w_)
		noexcept : X_{ x_ }, Y_{ y_ }, Z_{ z_ }, W_{ w_ } {}

	//--#-	defined(_LUMINA_INTRINSICS_UNUSED_)
	//--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--#-	-#--//
	#endif

	//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	Quaternion const Quaternion::Identity{ 0.0f, 0.0f, 0.0f, 1.0f };
}