// * World-space Positions, Normals and Tangents

namespace VertexElementArray {
	StructuredBuffer<float3> Position : register(t0, space16);
	StructuredBuffer<float2> UV : register(t0, space17);
	StructuredBuffer<float3> Normal : register(t0, space18);
	StructuredBuffer<float3> Tangent : register(t0, space19);
}

cbuffer Paramaters_B0Space0 : register(b0, space0) {
	float4x4 Matrix_WorldToProjective;
}

struct VSInput {
	uint Index_Position : IDX_POSITION0;
	uint Index_UV : IDX_UV0;
	uint Index_Normal : IDX_NORMAL0;
	uint Index_Tangent : IDX_TANGENT0;
};

struct VSOutput {
	float4 Position : SV_POSITION;
	float2 UV : UV0;
	float3 Normal : NORMAL0;
	float3 Tangent : TANGENT0;
	float3 Bitangent : BITANGENT0;
};

VSOutput main(VSInput input_) {
	VSOutput output;
	
	const float3 pos_World = VertexElementArray::Position[input_.Index_Position];
	const float2 uv = VertexElementArray::UV[input_.Index_UV];
	const float3 normal = VertexElementArray::Normal[input_.Index_Normal];
	const float3 tangent = VertexElementArray::Tangent[input_.Index_Tangent];
	
	output.Position = mul(float4(pos_World, 1.0f), Matrix_WorldToProjective);
	output.UV = uv;
	
	output.Normal = normal;
	output.Tangent = tangent;
	output.Bitangent = cross(output.Normal, output.Tangent);
	
	return output;
}