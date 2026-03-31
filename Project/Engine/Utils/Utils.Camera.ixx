export module Lumina.Utils.Camera;

import Lumina.Core.Common;
import Lumina.Core.Math;

namespace Lumina::Utils {
	export class Camera {
	private:
		class _Projection_;
		class _View_;

	public:
		constexpr auto View()
			const noexcept -> Math::F32x4x4<> const& { return View_; }
		constexpr auto Projection()
			const noexcept -> Math::F32x4x4<> const& { return Projection_; }

		constexpr auto WorldPosition()
			const noexcept -> Math::F32x3 const& { return WorldPosition_; }

		inline auto ViewInverse()
			const noexcept -> Math::F32x4x4<>;

	public:
		auto LookAt(
			Math::F32x3 const& eye_,
			Math::F32x3 const& center_,
			Math::F32x3 const& up_
		) noexcept -> void;
		auto RT(
			Math::F32x3 const& eulerAngles_,
			Math::F32x3 const& eye_
		) noexcept -> void;

		auto Perspective(
			F32 fovY_,
			F32 aspectRatio_,
			F32 nearClip_,
			F32 farClip_
		) noexcept -> void;
		auto Orthographic(
			Math::F32x3 const& leftTopNear_,
			Math::F32x3 const& rightBottomFar_
		) noexcept -> void;

		/*
		class _Perspective_
		*/
		//auto FOVY(F32 fovY_) noexcept -> void;
		//auto FOVY() const noexcept -> F32;
		//auto AspectRatio(F32 aspectRatio_) noexcept -> void;
		//auto AspectRatio() const noexcept -> F32;
		//operator F32x4x4<>() const noexcept;

	public:
		Camera() :
			View_{ Math::F32x4x4<>::Identity },
			Projection_{ Math::F32x4x4<>::Identity } {}
		~Camera() {}

	private:
		Math::F32x4x4<> View_;
		Math::F32x4x4<> Projection_;

		Math::F32x3 WorldPosition_;

		enum class PROJECTION_MODE {
			PERSPECTIVE,
			ORTHOGRAPHIC,
		};
	};


	inline auto Camera::ViewInverse() const noexcept -> Math::F32x4x4<> {
		Math::F32x4x4<> ret{};

		ret[0] = View_[0];
		ret[1] = View_[1];
		ret[2] = View_[2];
		ret[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
		ret = ret.Transpose();

		ret[3] = Math::F32x4{ View_[3] * ret }.FlipSign<1, 1, 1, 0>();

		return ret;
	}

	auto Camera::LookAt(
		Math::F32x3 const& eye_,
		Math::F32x3 const& center_,
		Math::F32x3 const& up_
	) noexcept -> void {
		Math::F32x3 const forward{ Math::F32x3{ center_ - eye_ }.Unit() };
		Math::F32x3 const right{ Math::F32x3::Cross(up_, forward).Unit() };
		Math::F32x3 const up{ Math::F32x3::Cross(forward, right) };
		Math::F32x3 const translate{
			-Lumina::Math::F32x3::Dot(eye_, right),
			-Lumina::Math::F32x3::Dot(eye_, up),
			-Lumina::Math::F32x3::Dot(eye_, forward)
		};

		View_ = {
			right.X, up.X, forward.X, 0.0f,
			right.Y, up.Y, forward.Y, 0.0f,
			right.Z, up.Z, forward.Z, 0.0f,
			translate.X, translate.Y, translate.Z, 1.0f,
		};

		WorldPosition_ = eye_;
	}

	auto Camera::RT(
		Math::F32x3 const& eulerAngles_,
		Math::F32x3 const& eye_
	) noexcept -> void {
		F32 const
			cos_X{ Math::COS(eulerAngles_.X) },
			sin_X{ Math::SIN(eulerAngles_.X) },
			cos_Y{ Math::COS(eulerAngles_.Y) },
			sin_Y{ Math::SIN(eulerAngles_.Y) },
			cos_Z{ Math::COS(eulerAngles_.Z) },
			sin_Z{ Math::SIN(eulerAngles_.Z) };

		Math::F32x4x4<> const rotation{
			cos_Y * cos_Z,
			cos_Y * sin_Z,
			-sin_Y,
			0.0f,

			sin_X * sin_Y * cos_Z - cos_X * sin_Z,
			sin_X * sin_Y * sin_Z + cos_X * cos_Z,
			sin_X * cos_Y,
			0.0f,

			cos_X * sin_Y * cos_Z + sin_X * sin_Z,
			cos_X * sin_Y * sin_Z - sin_X * cos_Z,
			cos_X * cos_Y,
			0.0f,

			0.0f, 0.0f, 0.0f, 1.0f,
		};

		//	Rotation matrices are orthogonal,
		//	so the transpose of a rotation matrix is equal to its inverse.
		View_ = rotation.Transpose();

		View_[3] = Math::F32x4{ Math::F32x4{ eye_, 1.0f } * View_ }.FlipSign<1, 1, 1, 0>();

		WorldPosition_ = eye_;
	}

	auto Camera::Perspective(
		F32 fovY_,
		F32 aspectRatio_,
		F32 nearClip_,
		F32 farClip_
	) noexcept -> void {
		F32 const cotTheta{ 1.0f / Math::TAN(fovY_ * 0.5f) };
		F32 const inv_FrustumHeight{ 1.0f / (farClip_ - nearClip_) };

		Projection_ = {
			(1.0f / aspectRatio_) * cotTheta, 0.0f, 0.0f, 0.0f,
			0.0f, cotTheta, 0.0f, 0.0f,
			0.0f, 0.0f, farClip_ * inv_FrustumHeight, 1.0f,
			0.0f, 0.0f, -nearClip_ * farClip_ * inv_FrustumHeight, 0.0f,
		};
	}

	auto Camera::Orthographic(
		Math::F32x3 const& leftTopNear_,
		Math::F32x3 const& rightBottomFar_
	) noexcept -> void {
		Math::F32x3 const sum{ rightBottomFar_ + leftTopNear_ };
		Math::F32x3 const diff{ rightBottomFar_ - leftTopNear_ };
		F32 const inv_W{ 1.0f / diff.X };
		F32 const inv_H{ 1.0f / diff.Y };
		F32 const inv_D{ 1.0f / diff.Z };

		Projection_ = {
			2.0f * inv_W, 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f * inv_H, 0.0f, 0.0f,
			0.0f, 0.0f, inv_D, 0.0f,
			-sum.X * inv_W, -sum.Y * inv_H, -sum.Z * inv_D, 1.0f,
		};
	}
}