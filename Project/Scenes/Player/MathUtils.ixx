export module Game.MathUtils;

import Lumina.Core.Math;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
}

namespace Game::MathUtils {
	export Matrix4x4 Scale(Vector3 const& scale_) {
		return {
			scale_.X, 0.0f, 0.0f, 0.0f,
			0.0f, scale_.Y, 0.0f, 0.0f,
			0.0f, 0.0f, scale_.Z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};
	}

	export Matrix4x4 RotateEulerXYZ(Vector3 const& eulerAngle_) {
		float const
			cosAlpha{ std::cosf(eulerAngle_.X) },
			sinAlpha{ std::sinf(eulerAngle_.X) },
			cosBeta{ std::cosf(eulerAngle_.Y) },
			sinBeta{ std::sinf(eulerAngle_.Y) },
			cosGamma{ std::cosf(eulerAngle_.Z) },
			sinGamma{ std::sinf(eulerAngle_.Z) };

		return {
			cosBeta * cosGamma,
			cosBeta * sinGamma,
			-sinBeta,
			0.0f,
			sinAlpha * sinBeta * cosGamma - cosAlpha * sinGamma,
			sinAlpha * sinBeta * sinGamma + cosAlpha * cosGamma,
			sinAlpha * cosBeta,
			0.0f,
			cosAlpha * sinBeta * cosGamma + sinAlpha * sinGamma,
			cosAlpha * sinBeta * sinGamma - sinAlpha * cosGamma,
			cosAlpha * cosBeta,
			0.0f,

			0.0f, 0.0f, 0.0f, 1.0f,
		};
	}

	export Matrix4x4 Translate(Vector3 const& traslate_) {
		return {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			traslate_.X, traslate_.Y, traslate_.Z, 1.0f,
		};
	}
	
	// S * R (EulerXYZ) * T
	export Matrix4x4 SRT(
		Vector3 const& scale_,
		Vector3 const& eulerAngle_,
		Vector3 const& translate_
	) {
		float const
			cosAlpha{ std::cos(eulerAngle_.X) },
			sinAlpha{ std::sin(eulerAngle_.X) },
			cosBeta{ std::cos(eulerAngle_.Y) },
			sinBeta{ std::sin(eulerAngle_.Y) },
			cosGamma{ std::cos(eulerAngle_.Z) },
			sinGamma{ std::sin(eulerAngle_.Z) };

		Matrix4x4 ret{
			cosBeta * cosGamma,
			cosBeta * sinGamma,
			-sinBeta,
			0.0f,
			sinAlpha * sinBeta * cosGamma - cosAlpha * sinGamma,
			sinAlpha * sinBeta * sinGamma + cosAlpha * cosGamma,
			sinAlpha * cosBeta,
			0.0f,
			cosAlpha * sinBeta * cosGamma + sinAlpha * sinGamma,
			cosAlpha * sinBeta * sinGamma - sinAlpha * cosGamma,
			cosAlpha * cosBeta,
			0.0f,
			translate_.X,
			translate_.Y,
			translate_.Z,
			1.0f,
		};

		ret[0] *= scale_.X;
		ret[1] *= scale_.Y;
		ret[2] *= scale_.Z;

		return ret;
	}

	export template<typename T>
	auto LERP(T const& a_, T const& b_, float t_)
		noexcept -> T { return a_ * (1.0f - t_) + b_ * t_; }

	export Matrix4x4 RotateAround(
		Vector3 const& axis_,
		float theta_
	) {
		float const cosTheta{ std::cos(theta_) };
		float const sinTheta{ std::sin(theta_) };
		return {
			axis_.X * axis_.X * (1.0f - cosTheta) + cosTheta,
			axis_.X * axis_.Y * (1.0f - cosTheta) + axis_.Z * sinTheta,
			axis_.X * axis_.Z * (1.0f - cosTheta) - axis_.Y * sinTheta,
			0.0f,

			axis_.Y * axis_.X * (1.0f - cosTheta) - axis_.Z * sinTheta,
			axis_.Y * axis_.Y * (1.0f - cosTheta) + cosTheta,
			axis_.Y * axis_.Z * (1.0f - cosTheta) + axis_.X * sinTheta,
			0.0f,

			axis_.Z * axis_.X * (1.0f - cosTheta) + axis_.Y * sinTheta,
			axis_.Z * axis_.Y * (1.0f - cosTheta) - axis_.X * sinTheta,
			axis_.Z * axis_.Z * (1.0f - cosTheta) + cosTheta,
			0.0f,

			0.0f, 0.0f, 0.0f, 1.0f,
		};
	}

	export Matrix4x4 Orthographic(
		float l_, float r_,
		float t_, float b_,
		float zn_, float zf_
	) {
		float const inv_W{ 1.0f / (r_ - l_) };
		float const inv_H{ 1.0f / (t_ - b_) };
		float const inv_D{ 1.0f / (zf_ - zn_) };

		return Matrix4x4{
			2.0f * inv_W, 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f * inv_H, 0.0f, 0.0f,
			0.0f, 0.0f, inv_D, 0.0f,
			-(l_ + r_) * inv_W, -(t_ + b_) * inv_H, -zn_ * inv_D, 1.0f,
		};
	}
}