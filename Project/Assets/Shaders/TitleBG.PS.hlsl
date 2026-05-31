#include"Basic.hlsli"

struct PSOutput {
	float4 Color : SV_TARGET0;
};

Texture2D<float4> Textures[] : register(t0, space1);
SamplerState Sampler : register(s0);

cbuffer SceneConstants : register(b0, space1) {
	float BloomRadius;
	float BloomIntensity;
	float BloomAttenuation;
	uint IsFadingOut;
	float BlurRadius;
	float BlurIntensity;
	float BlurAttenuation;
}

static const float Kernel[3][3] = {
	0.075f, 0.124f, 0.075f,
	0.124f, 0.204f, 0.124f,
	0.075f, 0.124f, 0.075f,
};
static const float Kernel2[3][3] = {
	-0.25f, -0.5f, 0.25f,
	-0.5f, 0.0f, 0.5f,
	-0.25f, 0.5f, 0.25f,
};

static const float2 inv_WH = { 1.0f / 1280.0f, 1.0f / 720.0f };

float4 RGBSplit(in float2 texCoord_, uint texID_, in float2 offset) {
	//float2 offset = { sin(Time * 128.0f) * 0.01f, cos(Time * 105.0f) * 0.01f };
	//return float4(
	//	Textures[texID_].Sample(Sampler, texCoord_ + offset).r,
	//	Textures[texID_].Sample(Sampler, texCoord_).g,
	//	Textures[texID_].Sample(Sampler, texCoord_ - offset).b,
	//	1.0f
	//);
	
	float2 coord = float2(0.5f, 0.5f) + (texCoord_ - float2(0.5f, 0.5f)) * 0.985f;
	float2 coord2 = float2(0.5f, 0.5f) + (texCoord_ - float2(0.5f, 0.5f)) * 1.015f;
	return float4(
		Textures[texID_].Sample(Sampler, coord).r,
		Textures[texID_].Sample(Sampler, texCoord_).g,
		Textures[texID_].Sample(Sampler, coord2).b,
		1.0f
	);
}

float4 Convolve(in float2 texCoord_, uint texID_, in float kernel_[3][3], float rad_) {
	float4 texColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	[unroll]
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			float2 offset = float2(i - 1, j - 1) * inv_WH * rad_;
			texColor += Textures[texID_].Sample(Sampler, texCoord_ + offset) * kernel_[2 - i][2 - j];
		}
	}
	return texColor;
}

float4 Negative(float4 rgba_) {
	return float4(1.0f, 1.0f, 1.0f, 1.0f) - rgba_;
}

float4 Bloom(in float2 texCoord_, uint texID_, float rad_, float intense_, float intenseAttenuation_) {
	//float4 ret = Textures[64].Sample(g_Sampler, texCoord_);
	float4 ret = 0.0f;
	//float intense = abs(sin(Time * 50.0f) * 1.0f) + 0.5f;
	//float intense = .7f * exp(-SceneParameters.Time * 7.0f);
	
	[unroll]
	for (int i = 1; i < 8; ++i) {
		intense_ *= (1.0f - intenseAttenuation_);
		ret += Convolve(texCoord_, texID_, Kernel, i * rad_) * intense_;
	}
	//ret += Convolve(texCoord_, Kernel, 10.0f) * 0.6f * intense;
	//ret += Convolve(texCoord_, Kernel, 15.0f) * 0.4f * intense;
	//ret += Convolve(texCoord_, Kernel, 20.0f) * 0.2f * intense;
	
	return ret;
}

//float4 Distort(float2 texCoord_, float2 offset_, float len_) {
//	//float2 offset = float2(
//	//	cos(SceneParameters.EnemyPos.x * .72f) * sin(texCoord_.y * 7.5f) * 0.01f,
//	//	sin(SceneParameters.EnemyPos.y * .75f) * sin(texCoord_.x * 7.2f) * 0.01f
//	//);
//	//float d = exp(-len_ * 0.5f - SceneParameters.Time * 1.3f) * 0.5f;
//	float d1 =
//		exp(-(len_ * 0.5f + SceneParameters.Time * 7.0f)) *
//		sin(len_ * 2.0f + SceneParameters.Time * 25.0f) *
//		0.34f;
//	float d2 =
//		exp(-(len_ * 0.7f + SceneParameters.Time * 6.0f)) *
//		sin(len_ * 3.0f + SceneParameters.Time * 20.0f) *
//		0.35f;
//	float d3 =
//		exp(-(len_ * 0.9f + SceneParameters.Time * 5.0f)) *
//		sin(len_ * 5.0f + SceneParameters.Time * 15.0f) *
//		0.36f;
	
//	return float4(
//		Textures[64].Sample(Sampler, texCoord_ + d1 * offset_).r,
//		Textures[64].Sample(Sampler, texCoord_ + d2 * offset_).g,
//		Textures[64].Sample(Sampler, texCoord_ + d3 * offset_).b,
//		1.0f
//	);
//	//return Textures[64].Sample(Sampler, texCoord_ + d * offset);
//}

PSOutput main(VSOutput input_) {
	PSOutput output;
	//float4 texColor = Textures[input_.TexID].Sample(Sampler, input_.TexCoord);
	//output.Color = texColor * input_.Color;
	float4 bloom = Bloom(input_.TexCoord, input_.TexID, BloomRadius, BloomIntensity, BloomAttenuation);
	output.Color = bloom;
	if (IsFadingOut) {
		float4 blur = Bloom(input_.TexCoord, input_.TexID, BlurRadius, BlurIntensity, BlurAttenuation);
		output.Color = max(bloom, blur);
	}
	output.Color *= input_.Color;
	
	return output;
}