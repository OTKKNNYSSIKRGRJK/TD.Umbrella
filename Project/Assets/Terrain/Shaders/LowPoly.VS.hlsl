
namespace VertexElementArrays {
	StructuredBuffer<float3> Position[] : register(t0, space16);
	StructuredBuffer<float2> UV[] : register(t0, space17);
	StructuredBuffer<float3> Normal[] : register(t0, space18);
	StructuredBuffer<float3> Tangent[] : register(t0, space19);
}

StructuredBuffer<float4x4> Matrices_LocalToWorld : register(t0, space0);

uint MeshIndex : register(b0, space0);

cbuffer Paramaters : register(b1, space0) {
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
	
	const float3 pos_Local = VertexElementArrays::Position[MeshIndex][input_.Index_Position];
	const float2 uv = VertexElementArrays::UV[MeshIndex][input_.Index_UV];
	const float3 normal = VertexElementArrays::Normal[MeshIndex][input_.Index_Normal];
	const float3 tangent = VertexElementArrays::Tangent[MeshIndex][input_.Index_Tangent];
	
	const float4x4 mat_localToWorld = Matrices_LocalToWorld[MeshIndex];
	const float3x3 mat_localToWorld_NoTranslation = (float3x3) mat_localToWorld;
	
	output.Position = mul(float4(pos_Local, 1.0f), mat_localToWorld);
	output.Position = mul(output.Position, Matrix_WorldToProjective);
	output.UV = uv;
	output.Normal = mul(normal, mat_localToWorld_NoTranslation);
	output.Normal = normalize(output.Normal);
	output.Tangent = mul(normal, mat_localToWorld_NoTranslation);
	output.Tangent = normalize(output.Tangent);
	output.Bitangent = cross(output.Normal, output.Tangent);
	
	return output;
}