#include"Watercolor.hlsli"
#include"MathUtils.hlsli"

#define _NUM_THREADS_X_ 8
#define _NUM_THREADS_Y_ 8



void AdjustColorByPigmentDensity(in uint2 coord_, in float2 uv_) {
	const float pigmentDensity = Watercolor::Input::Simulation::Pigment.SampleLevel(BilinearClamp, uv_, 0.0f);
	const float4 color = Watercolor::Input::Geometry::Albedo.SampleLevel(BilinearClamp, uv_, 0.0f);
	Watercolor::Output::Composite[coord_] = pow(color, pigmentDensity);
}

void CalculateColorBleeding(in uint2 coord_, in float2 uv_) {
	const float4 color = Watercolor::Output::Composite[coord_];
	const float4 colorBlurred = Watercolor::Input::BlurV.SampleLevel(BilinearClamp, uv_, 0.0f);
	const float4 factors0 = Watercolor::Input::Geometry::Factors0.SampleLevel(BilinearClamp, uv_, 0.0f);
	const float factor_Bleeding = factors0.r;
	
	Watercolor::Output::Composite[coord_] =
		color * (1.0f - factor_Bleeding) +
		colorBlurred * factor_Bleeding;
}

void CalculateEdgeDarkening(in uint2 coord_, in float2 uv_) {
	const float4 color = Watercolor::Output::Composite[coord_];
	const float edgeDensity = Watercolor::Input::EdgeDensity.SampleLevel(BilinearClamp, uv_, 0.0f);;
	Watercolor::Output::Composite[coord_] = pow(color, 1.0f + edgeDensity);
}

void MultiplySubstrateColor(in uint2 coord_, in float2 uv_) {
	const float4 substrateColor = Watercolor::Input::Substrate::Albedo.SampleLevel(BilinearClamp, uv_, 0.0f);
	Watercolor::Output::Composite[coord_] *= substrateColor;
}

[numthreads(_NUM_THREADS_X_, _NUM_THREADS_Y_, 1)]
void main(uint3 tid_ : SV_DispatchThreadID) {
	const uint2 coord = tid_.xy;
	const float2 uv = Watercolor::TexelSize * coord;
	
	AdjustColorByPigmentDensity(coord, uv);
	CalculateColorBleeding(coord, uv);
	CalculateEdgeDarkening(coord, uv);
	MultiplySubstrateColor(coord, uv);
}