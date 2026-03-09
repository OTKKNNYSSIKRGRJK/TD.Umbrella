//////	//////	//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Math : Geometry.Transformation;
//////	//////	//////	//////	//////	//////	//////	//////	//////

import : Fundamental.Exponentiation;
import : Fundamental.Trigonometry;
import : NumberSystem.Quaternion;
import : LinearAlgebra.Vector;
import : LinearAlgebra.Matrix;

import Lumina.Core.Common;

//****	******	******	******	******	******	******	******	****//

//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
//..''	''..''	''..''	''..''	''..''	''..''	''..''	''..''	''..//
/// @class		Lumina::Math::UnitF32x3
/// @brief		<span>3D 32-bit Float Unit Vector</span>
//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//
/// @class		Lumina::Math::AxisAngle
/// @brief		<span>Axis & Angle of the Axial Rotation</span>
//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//
/// @class		Lumina::Math::Versor
//''..	..''..	..''..	..''..	..''..	..''..	..''..	..''..	..''//
//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

namespace Lumina::Math {
	export class UnitF32x3;

	export class AxisAngle;
	export class Versor;
	export class SE3;

	export class Affinity;
}

//^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^v~	~v^^//
//	>>	UnitF32x3												<<	//
//vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv^~	~^vv//

namespace Lumina::Math {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Definition												:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	class [[nodiscard]] UnitF32x3 {

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Cast Operators											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr operator F32x3 const&() const noexcept;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Constructors, Destructor								--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr UnitF32x3() noexcept;
		inline UnitF32x3(float x_, float y_, float z_) noexcept;
		inline UnitF32x3(F32x3 const& vec_) noexcept;

		//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

		constexpr ~UnitF32x3() noexcept;

		//++##	++##++	##++##	++##++	##++##	++##++	##++//

	private:
		F32x3 Wrapped_;
	};

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Implementation											:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Cast Operators											--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	constexpr UnitF32x3::operator F32x3 const&()
		const noexcept { return Wrapped_; }

	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
	//==--	Constructors, Destructor								--==//
	//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	constexpr UnitF32x3::UnitF32x3() noexcept :
		Wrapped_{ 1.0f, 0.0f, 0.0f } {}
	inline UnitF32x3::UnitF32x3(float x_, float y_, float z_) noexcept :
		UnitF32x3{ F32x3{ x_, y_, z_ } } {}
	inline UnitF32x3::UnitF32x3(F32x3 const& vec_) noexcept :
		Wrapped_{ vec_ } {
		F32 const len2{ F32x3::Dot(Wrapped_, Wrapped_) };
		if (len2 > 0.0f) {
			if (len2 != 1.0f) {
				F32 const inv_Norm{ 1.0f / SQRT(len2) };
				Wrapped_ *= inv_Norm;
			}
		}
		else {
			Wrapped_ = { 1.0f, 0.0f, 0.0f };
		}
	}

	//''""	''""''	""''""	''""''	""''""	''""''	""''""	''""''	""''//

	constexpr UnitF32x3::~UnitF32x3() noexcept = default;
}

namespace Lumina::Math {
	class AxisAngle {
	public:
		UnitF32x3 Axis;
		F32 Angle;
	};
}

namespace Lumina::Math {
	class [[nodiscard]] Versor {

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Cast Operators											--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr operator Quaternion const&() const noexcept { return Wrapped_; }

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Constructors, Destructor								--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	public:
		constexpr Versor() noexcept;
		inline Versor(Quaternion const&) noexcept;
		inline Versor(AxisAngle const& axisAngle_) noexcept;

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

	private:
		Quaternion Wrapped_;
	};
}

namespace Lumina::Math {
	export class SE3 {
	public:
		constexpr operator F32x4x4<> const&() const noexcept { return Wrapped_; }

		//----	------	------	------	------	----//
	
	public:
		SE3 Inv() noexcept;

		//----	------	------	------	------	----//

	public:
		SE3(AxisAngle const& r_) noexcept;
		SE3(AxisAngle const& r_, F32x3 const& t_) noexcept;
		SE3(Versor const& r_) noexcept;
		SE3(Versor const& r_, F32x3 const& t_) noexcept;

	private:
		F32x4x4<> Wrapped_;
	};
}

namespace Lumina::Math {
	export class Affinity : private F32x4x4<> {
	public:
		inline operator F32x4x4<>() const noexcept { return { *this }; }

	public:
		static auto Reflection() -> Affinity;
		static auto Scale() -> Affinity;
		static auto Shear() -> Affinity;
		static auto Rotate() -> Affinity;
		static auto Translate() -> Affinity;
	};
}

namespace Lumina::Math {
	constexpr Versor::Versor() noexcept : Wrapped_{} {}

	inline Versor::Versor(Quaternion const& quat_) noexcept : Wrapped_{ quat_.Unit() } {}

	inline Versor::Versor(AxisAngle const& axisAngle_) noexcept {
		F32 const cos_HalfAngle{ COS(axisAngle_.Angle * 0.5f) };
		F32 const sin_HalfAngle{ SIN(axisAngle_.Angle * 0.5f) };
		F32x3 const& axis{ static_cast<F32x3 const&>(axisAngle_.Axis) };
		Wrapped_.X(axis.X * sin_HalfAngle);
		Wrapped_.Y(axis.Y * sin_HalfAngle);
		Wrapped_.Z(axis.Z * sin_HalfAngle);
		Wrapped_.W(cos_HalfAngle);
	}
}

namespace Lumina::Math {
	namespace {
		void QuaternionToMatrix(F32x4x4<>& m_, Quaternion const& q_) noexcept {
			F32x4 const xyzw{ static_cast<F32 const*>(q_) };					//	(x, y, z, w)
			F32x4 const xyzw_Negated{ -xyzw };									//	(-x, -y, -z, -w)
			F32x4 tmp_0{}, tmp_1{}, tmp_2{};

			//----	Row 0	------	------	------	------	----//
			{
				tmp_0 = F32x4::Shuffle<0, 3, 1, 2>(xyzw, xyzw_Negated);			//	(x, w, -y, -z)
				tmp_0 = F32x4::Shuffle<0, 2, 3, 1>(tmp_0, tmp_0);				//	(x, -y, -z, w)

				tmp_1 = F32x4::Shuffle<1, 0, 3, 2>(xyzw, xyzw);					//	(y, x, w, z)

				tmp_2 = F32x4::Shuffle<2, 0, 3, 1>(xyzw, xyzw_Negated);			//	(z, x, -w, -y)			
				tmp_2 = F32x4::Shuffle<0, 2, 1, 3>(tmp_2, tmp_2);				//	(z, -w, x, -y)

				//....	......	......	......	......	......	....//

				m_[0] = {
					F32x4::Dot<4>(xyzw, tmp_0),									//	ww + xx - yy - zz
					F32x4::Dot<4>(xyzw, tmp_1),									//	2(xy + zw)
					F32x4::Dot<4>(xyzw, tmp_2),									//	2(xz - yw)
					0.0f
				};
			}

			//----	Row 1	------	------	------	------	----//
			{
				tmp_0 = F32x4::Shuffle<1, 0, 3, 2>(xyzw, xyzw_Negated);			//	(y, x, -w, -z)

				tmp_1 = F32x4::Shuffle<1, 3, 0, 2>(xyzw, xyzw_Negated);			//	(y, w, -x, -z)
				tmp_1 = F32x4::Shuffle<2, 0, 3, 1>(tmp_1, tmp_1);				//	(-x, y, -z, w)

				tmp_2 = F32x4::Shuffle<3, 2, 1, 0>(xyzw, xyzw);					//	(w, z, y, x)

				//....	......	......	......	......	......	....//

				m_[1] = {
					F32x4::Dot<4>(xyzw, tmp_0),									//	2(xy - zw)
					F32x4::Dot<4>(xyzw, tmp_1),									//	ww - xx + yy - zz
					F32x4::Dot<4>(xyzw, tmp_2),									//	2(yz + xw)
					0.0f
				};
			}

			//----	Row 2	------	------	------	------	----//
			{
				tmp_0 = F32x4::Shuffle<2, 3, 0, 1>(xyzw, xyzw);					//	(z, w, x, y)

				tmp_1 = F32x4::Shuffle<1, 2, 0, 3>(xyzw, xyzw_Negated);			//	(y, z, -x, -w)
				tmp_1 = F32x4::Shuffle<3, 1, 0, 2>(tmp_1, tmp_1);				//	(-w, z, y, -x)

				tmp_2 = F32x4::Shuffle<0, 1, 2, 3>(xyzw_Negated, xyzw);			//	(-x, -y, z, w)

				//....	......	......	......	......	......	....//

				m_[2] = {
					F32x4::Dot<4>(xyzw, tmp_0),									//	2(xz + yw)
					F32x4::Dot<4>(xyzw, tmp_1),									//	2(yz - xw)
					F32x4::Dot<4>(xyzw, tmp_2),									//	ww - xx - yy + zz
					0.0f
				};
			}
		}
	}

	/*SE3 SE3::Inv() noexcept {
		SE3 ret{ *this };
		ret.Rows_[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
		ret = ret.Transpose();
		F32x4 t{ F32x4{ Entries_[3] } * ret };
		ret[3][0] = -t.X;
		ret[3][1] = -t.Y;
		ret[3][2] = -t.Z;

		return ret;
	}*/

	SE3::SE3(AxisAngle const& r_) noexcept {
		QuaternionToMatrix(Wrapped_, Versor{ r_ });
	}

	SE3::SE3(Versor const& r_) noexcept {
		QuaternionToMatrix(Wrapped_, r_);
	}

	SE3::SE3(Versor const& r_, F32x3 const& t_) noexcept {
		QuaternionToMatrix(Wrapped_, r_);
		Wrapped_[3] = { t_, 1.0f };
	}
}