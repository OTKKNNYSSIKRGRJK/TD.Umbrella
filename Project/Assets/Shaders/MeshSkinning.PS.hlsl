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
	
	float4 diffuseColor = Textures[Material.ID_DiffuseMap].Sample(Sampler, input_.TexCoord);
	output.Diffuse = diffuseColor * Material.Color;
	const float3 normal = normalize(input_.Normal.xyz);
	output.Normal = float4(normal * 0.5f + 0.5f, 1.0f);
	// * Bleeding
	output.Factors0.r = abs(normal.z);
	// * Edge Density
	output.Factors0.g = abs(cos(input_.Pos * 0.01f));
	// * Depth
	//output.Factors0.b = input_.Pos.z / input_.Pos.w;
	
	return output;
}