#include"Watercolor.hlsli"
#include"MathUtils.hlsli"

#define _NUM_THREADS_X_ 8
#define _NUM_THREADS_Y_ 8



namespace Kernel {
	namespace Gaussian {
		static const float Separable[7] = {
			1.0f / 64.0f,
			6.0f / 64.0f,
			15.0f / 64.0f,
			20.0f / 64.0f,
			15.0f / 64.0f,
			6.0f / 64.0f,
			1.0f / 64.0f,
		};
	}
}



[numthreads(_NUM_THREADS_X_, _NUM_THREADS_Y_, 1)]
void Convolve_Horizontal(uint3 tid_ : SV_DispatchThreadID) {
	const uint2 coord = tid_.xy;
	const float2 uv_Center = Watercolor::TexelSize * coord;
	
	float4 ret = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	for (int i = -3; i <= 3; ++i) {
		const float2 uv = uv_Center + float2(i, 0) * Watercolor::UVStep.x * 3.0f;
		// * TODO : advection
		const float2 norm = Watercolor::Input::Substrate::Normal.SampleLevel(BilinearClamp, uv, 0.0f).xy;
		const float2 uv_Prime = uv + (norm - float2(0.5f, 0.5f)) * 5.0f;
		ret +=
			Watercolor::Input::Geometry::Albedo.SampleLevel(BilinearClamp, uv, 0.0f) *
			Kernel::Gaussian::Separable[i + 3];
	}
	
	Watercolor::Output::BlurH[coord] = ret;
}

[numthreads(_NUM_THREADS_X_, _NUM_THREADS_Y_, 1)]
void Convolve_Vertical(uint3 tid_ : SV_DispatchThreadID) {
	const uint2 coord = tid_.xy;
	const float2 uv_Center = Watercolor::TexelSize * coord;
	
	float4 ret = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	for (int i = -3; i <= 3; ++i) {
		const float2 uv = uv_Center + float2(0, i) * Watercolor::UVStep.y * 3.0f;
		// * TODO : advection
		const float2 norm = Watercolor::Input::Substrate::Normal.SampleLevel(BilinearClamp, uv, 0.0f).xy;
		const float2 uv_Prime = uv + (norm - float2(0.5f, 0.5f)) * 5.0f;
		ret +=
			Watercolor::Input::BlurH.SampleLevel(BilinearClamp, uv, 0.0f) *
			Kernel::Gaussian::Separable[i + 3];
	}
	
	Watercolor::Output::BlurV[coord] = ret;
}


