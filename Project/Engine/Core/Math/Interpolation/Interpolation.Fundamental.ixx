//////	//////	//////	//////	//////	//////	//////	//////	//////
export module Lumina.Core.Math : Interpolation.Fundamental;
//////	//////	//////	//////	//////	//////	//////	//////	//////

import <cmath>;
import <algorithm>;
import <concepts>;

import Lumina.Core.Common;

import : Fundamental.Trigonometry;
import : NumberSystem.Quaternion;

namespace Lumina::Math {
	export template<typename _T>
	class LERP {
	public:
		inline auto operator()(F32 t_) const& noexcept
			-> _T { return { Alpha_ * (1.0f - t_) + Omega_ * t_ }; }

	public:
		constexpr LERP(_T const& alpha_, _T const& omega_) noexcept :
			Alpha_{ alpha_ }, Omega_{ omega_ } {}

	private:
		_T Alpha_;
		_T Omega_;
	};

	export class SLERP {
	public:
		inline auto operator()(F32 t_) const& noexcept -> Quaternion {
			if (INV_SIN_Theta_ != 0.0f) {
				return
					Alpha_ * std::sin(Theta_ * (1.0f - t_)) * INV_SIN_Theta_ +
					Omega_ * std::sin(Theta_ * t_) * INV_SIN_Theta_;
			}
			else {
				return Alpha_ * (1.0f - t_) + Omega_ * t_;
			}
		}

	public:
		constexpr SLERP() noexcept = default;
		inline SLERP(Quaternion const& q0_, Quaternion const& q1_) noexcept {
			Alpha_ = q0_.Unit();
			Omega_ = q1_.Unit();
			F32 const cos_Theta{ std::clamp(Quaternion::Dot(Alpha_, Omega_), -1.0f, 1.0f) };
			if (cos_Theta < 0.0f) {
				Alpha_ = Alpha_ * (-1.0f);
			}
			Theta_ = std::acos(cos_Theta);
			INV_SIN_Theta_ = 1.0f / SIN(Theta_);
		}

	private:
		Quaternion Alpha_{};
		Quaternion Omega_{};

		F32 Theta_{};
		F32 INV_SIN_Theta_{};
	};
}