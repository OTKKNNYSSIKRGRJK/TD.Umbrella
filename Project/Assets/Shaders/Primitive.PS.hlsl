#include"Primitive.hlsli"

struct PSOutput {
	float4 Color : SV_TARGET0;
};

Texture2D<float4> Textures[] : register(t0, space1);
SamplerState Sampler : register(s0);

PSOutput main(VSOutput input_) {
	PSOutput output;
	float4 texColor = Textures[input_.TexID].Sample(Sampler, input_.TexCoord);
	output.Color = texColor * input_.Color;
	//output.Color = input_.Color;
	return output;
}