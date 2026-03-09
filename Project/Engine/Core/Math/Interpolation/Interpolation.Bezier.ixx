export module Lumina.Core.Math : Interpolation.Bezier;

//import <cstdint>;
//
//namespace Lumina::Math::Easing {
//	export template<uint32_t Degree>
//	constexpr float In(float t_) {
//		float const recursiveResult1{ In<(Degree >> 1)>(t_) };
//		float const recursiveResult2{ In<(Degree & 1)>(t_) };
//		return recursiveResult1 * recursiveResult1 * recursiveResult2;
//	}
//
//	template<> constexpr float In<0U>(float) { return 1.0f; }
//	template<> constexpr float In<1U>(float t_) { return t_; }
//	template<> constexpr float In<2U>(float t_) { return t_ * t_; }
//	template<> constexpr float In<3U>(float t_) { return t_ * t_ * t_; }
//	template<> constexpr float In<4U>(float t_) { return t_ * t_ * t_ * t_; }
//
//	export template<uint32_t Degree>
//	constexpr float Out(float t_) {
//		return 1.0f - In<Degree>(1.0f - t_);
//	}
//}