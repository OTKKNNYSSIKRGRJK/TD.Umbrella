#include"Watercolor.hlsli"
#include"MathUtils.hlsli"

#define _NUM_THREADS_X_ 8
#define _NUM_THREADS_Y_ 8



void AdjustColorByPigmentDensity(in uint2 coord_, in float2 uv_) {
	const float4 color = Watercolor::Output::Composite[coord_];
	const float pigmentDensity = Watercolor::Input::Simulation::Pigment.SampleLevel(BilinearClamp, uv_, 0.0f);
	Watercolor::Output::Composite[coord_].a = lerp(0.75f, 1.0f, pigmentDensity);
}

void CalculateColorBleeding(in uint2 coord_, in float2 uv_) {
	const float4 color = Watercolor::Output::Composite[coord_];
	const float4 colorBlurred = Watercolor::Input::BlurV.SampleLevel(BilinearClamp, uv_, 0.0f);
	const float4 factors0 = Watercolor::Input::Geometry::Factors0.SampleLevel(BilinearClamp, uv_, 0.0f);
	float factor_Bleeding = Watercolor::Input::Simulation::Pigment.SampleLevel(BilinearClamp, uv_ * 0.5f, 0.0f);
	factor_Bleeding = saturate(factor_Bleeding * 1.5f);
	
	Watercolor::Output::Composite[coord_].rgb = lerp(color.rgb, colorBlurred.rgb, factor_Bleeding);
}

void CalculateEdgeDarkening(in uint2 coord_, in float2 uv_, in float4 color_) {
	const float edgeDensity = Watercolor::Input::EdgeDensity.SampleLevel(BilinearClamp, uv_, 0.0f);
	float3 color = Watercolor::Output::Composite[coord_].rgb * 0.9f + color_.rgb * 0.1f;
	color = saturate(color * (1.0f - edgeDensity));
	color = pow(color.rgb, 1.0f + edgeDensity);
	Watercolor::Output::Composite[coord_].rgb = lerp(
		color,
		Watercolor::Output::Composite[coord_].rgb,
		edgeDensity * 0.25f
	);
	//Watercolor::Output::Composite[coord_].rgb = edgeDensity;
}

void ApplySubstrateColor(in uint2 coord_, in float2 uv_) {
	const float4 color = Watercolor::Output::Composite[coord_];
	const float4 substrateColor = Watercolor::Input::Substrate::Albedo.SampleLevel(BilinearClamp, uv_, 0.0f);
	Watercolor::Output::Composite[coord_].rgb = color.rgb * color.a + substrateColor.rgb * (1.0f - color.a);
	Watercolor::Output::Composite[coord_].a = 1.0f;
	Watercolor::Output::Composite[coord_].rgb *= substrateColor.rgb;
}

[numthreads(_NUM_THREADS_X_, _NUM_THREADS_Y_, 1)]
void main(uint3 tid_ : SV_DispatchThreadID) {
	const uint2 coord = tid_.xy;
	const float2 uv = Watercolor::TexelSize * coord;
	
	const float4 color = Watercolor::Input::Geometry::Albedo.SampleLevel(BilinearClamp, uv, 0.0f);
	Watercolor::Output::Composite[coord] = color;
	
	AdjustColorByPigmentDensity(coord, uv);
	CalculateColorBleeding(coord, uv);
	CalculateEdgeDarkening(coord, uv, color);
	ApplySubstrateColor(coord, uv);
}