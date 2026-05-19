#include"Watercolor.hlsli"
#include"MathUtils.hlsli"

#define _NUM_THREADS_X_ 8
#define _NUM_THREADS_Y_ 8



namespace Kernel {
	namespace Sobel {
		static const float Horizontal[3][3] = {
			{ -1.0f, 0.0f, 1.0f },
			{ -2.0f, 0.0f, 2.0f },
			{ -1.0f, 0.0f, 1.0f },
		};
		static const float Vertical[3][3] = {
			{ -1.0f, -2.0f, -1.0f },
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 2.0f, 1.0f },
		};
	}
	
	namespace Gaussian {
		static const float Sigma1[3][3] = {
			{ 21.0f / 256.0f, 31.0f / 256.0f, 21.0f / 256.0f },
			{ 31.0f / 256.0f, 48.0f / 256.0f, 31.0f / 256.0f },
			{ 21.0f / 256.0f, 31.0f / 256.0f, 21.0f / 256.0f },
		};
	}
}



static const int2 CoordOffsets[9] = {
	{ -1, -1 },
	{ -1, 0 },
	{ -1, 1 },
	{ 0, -1 },
	{ 0, 0 },
	{ 0, 1 },
	{ 1, -1 },
	{ 1, 0 },
	{ 1, 1 },
};

void SampleUVs(out float2 uvs_[9], in float2 uv_Center_, float uvStepFactor_) {
	for (int i = 0; i < 9; ++i) {
		uvs_[i] = uv_Center_ + CoordOffsets[i] * Watercolor::UVStep * uvStepFactor_;
	}
}



void SampleLuminances(out float luminances_[9], in Texture2D<float4> tex_Albedo_, in float2 uvs_[9]) {
	for (int i = 0; i < 9; ++i) {
		luminances_[i] = Color::Luminance(tex_Albedo_.SampleLevel(BilinearClamp, uvs_[i], 0.0f).rgb);
	}
}

float2 Convolve_Luminance(in Texture2D<float4> tex_Albedo_, in float2 uvs_[9], in float kernel_[3][3]) {
	float luminances[9];
	SampleLuminances(luminances, tex_Albedo_, uvs_);
	
	float2 ret = { 0.0f, 0.0f };
	
	ret += kernel_[0][0] * luminances[0];
	ret += kernel_[0][1] * luminances[1];
	ret += kernel_[0][2] * luminances[2];
	
	ret += kernel_[1][0] * luminances[3];
	ret += kernel_[1][1] * luminances[4];
	ret += kernel_[1][2] * luminances[5];
	
	ret += kernel_[2][0] * luminances[6];
	ret += kernel_[2][1] * luminances[7];
	ret += kernel_[2][2] * luminances[8];
	
	return ret;
}

float2 Edge_Luminance(in Texture2D<float4> tex_Albedo_, in float2 uvs_[9]) {
	float2 ret;
	
	ret.x = Convolve_Luminance(tex_Albedo_, uvs_, Kernel::Sobel::Horizontal);
	ret.y = Convolve_Luminance(tex_Albedo_, uvs_, Kernel::Sobel::Vertical);
	
	return ret;
}


void SampleDepths(out float depths_[9], in Texture2D<float> tex_Depth_, in float2 uvs_[9]) {
	for (int i = 0; i < 9; ++i) {
		depths_[i] = Color::Luminance(tex_Depth_.SampleLevel(BilinearClamp, uvs_[i], 0.0f));
	}
}

float2 Convolve_Depth(in Texture2D<float> tex_Depth_, in float2 uvs_[9], in float kernel_[3][3]) {
	float depths[9];
	SampleDepths(depths, tex_Depth_, uvs_);
	
	float2 ret = { 0.0f, 0.0f };
	
	ret += kernel_[0][0] * depths[0];
	ret += kernel_[0][1] * depths[1];
	ret += kernel_[0][2] * depths[2];
	
	ret += kernel_[1][0] * depths[3];
	ret += kernel_[1][1] * depths[4];
	ret += kernel_[1][2] * depths[5];
	
	ret += kernel_[2][0] * depths[6];
	ret += kernel_[2][1] * depths[7];
	ret += kernel_[2][2] * depths[8];
	
	return ret;
}

float2 Edge_Depth(in Texture2D<float> tex_Depth_, in float2 uvs_[9]) {
	float2 ret;
	
	ret.x = Convolve_Depth(tex_Depth_, uvs_, Kernel::Sobel::Horizontal);
	ret.y = Convolve_Depth(tex_Depth_, uvs_, Kernel::Sobel::Vertical);
	
	return ret;
}

[numthreads(_NUM_THREADS_X_, _NUM_THREADS_Y_, 1)]
void Detect(uint3 dtid_ : SV_DispatchThreadID) {
	const uint2 coord = dtid_.xy;
	const float2 uv_Center = Watercolor::TexelSize * coord;
	float2 uvs[9];
	SampleUVs(uvs, uv_Center, 1.0f);
	
	float4 result;
	result.xy = Edge_Luminance(Watercolor::Input::Geometry::Albedo, uvs) * Watercolor::Weight_Luminance;
	result.zw = Edge_Depth(Watercolor::Input::Geometry::Depth, uvs) * Watercolor::Weight_Depth;
	
	Watercolor::Output::Edge[coord] = saturate(dot(result, result));
}



void Sample(out float vals_[9], in Texture2D<float> tex_, in float2 uvs_[9]) {
	for (int i = 0; i < 9; ++i) {
		vals_[i] = tex_.SampleLevel(BilinearClamp, uvs_[i], 0.0f);
	}
}

float2 Convolve(in Texture2D<float> tex_, in float2 uvs_[9], in float kernel_[3][3]) {
	float values[9];
	Sample(values, tex_, uvs_);
	
	float2 ret = { 0.0f, 0.0f };
	
	ret += kernel_[0][0] * values[0];
	ret += kernel_[0][1] * values[1];
	ret += kernel_[0][2] * values[2];
	
	ret += kernel_[1][0] * values[3];
	ret += kernel_[1][1] * values[4];
	ret += kernel_[1][2] * values[5];
	
	ret += kernel_[2][0] * values[6];
	ret += kernel_[2][1] * values[7];
	ret += kernel_[2][2] * values[8];
	
	return ret;
}

[numthreads(_NUM_THREADS_X_, _NUM_THREADS_Y_, 1)]
void CalculateDensity(uint3 tid_ : SV_DispatchThreadID) {
	const uint2 coord = tid_.xy;
	const float2 uv_Center = Watercolor::TexelSize * coord;
	
	const float4 factors0 = Watercolor::Input::Geometry::Factors0.SampleLevel(BilinearClamp, uv_Center, 0.0f);
	const float factor_Bleeding = factors0.r;
	const float factor_EdgeDensity = factors0.g;
	
	float result = 0.0f;
	float2 uvs[9];
	SampleUVs(uvs, uv_Center, 1.0f);
	result += Convolve(Watercolor::Input::Edge, uvs, Kernel::Gaussian::Sigma1);
	SampleUVs(uvs, uv_Center, 2.0f);
	result += Convolve(Watercolor::Input::Edge, uvs, Kernel::Gaussian::Sigma1);
	
	const float noise = Watercolor::Input::Noise.SampleLevel(BilinearClamp, uv_Center * 0.5f, 0.0f);
	result *= (factor_EdgeDensity + noise) * 0.5f;
	
	// * Where the more bleeding, the less indistinct edge. 
	result *= lerp(1.0f, 0.2f, smoothstep(0.0f, 1.0f, factor_Bleeding));
	
	Watercolor::Output::EdgeDensity[coord] = saturate(result);
}