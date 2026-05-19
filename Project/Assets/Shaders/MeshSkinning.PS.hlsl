struct VSOutput {
	float4 Pos : SV_Position;
	float3 WorldPos : POSITION0;
	float2 TexCoord : TEXCOORD0;
	float3 Normal : NORMAL0;
};

struct PSOutput {
	float4 Diffuse : SV_TARGET0;
	float4 Normal : SV_TARGET1;
	float4 Factors0 : SV_TARGET2;
	//float4 Specular : SV_TARGET3;
};

Texture2D<float4> Textures[] : register(t0, space1);
SamplerState Sampler : register(s0);

struct MATERIAL {
	float4 Color;
	uint ID_DiffuseMap;
	uint ID_SpecularMap;
	uint ID_NormalMap;
};
#define SPACE_MATERIAL space3
ConstantBuffer<MATERIAL> Material : register(b0, SPACE_MATERIAL);

PSOutput main(VSOutput input_) {
	PSOutput output;
	
	//float4 diffuseColor = Textures[Material.ID_DiffuseMap].Sample(Sampler, input_.TexCoord);
	//output.Diffuse = diffuseColor * Material.Color;
	output.Diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
	output.Normal = float4(normalize(input_.Normal.xyz) * 0.5f + 0.5f, 1.0f);
	// * Bleeding
	output.Factors0.r = abs(output.Normal.x);
	// * Edge Density
	output.Factors0.g = abs(output.Normal.y);
	
	return output;
}