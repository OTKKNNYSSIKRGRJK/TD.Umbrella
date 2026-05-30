struct VSInput {
	float4 Position : POSITION0;
	float2 TexCoord : TEXCOORD0;
	float3 Normal : NORMAL0;
};

struct VSOutput {
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD0;
	float3 Normal : NORMAL0;
	float3 LocalNormal : NORMAL1;
};

cbuffer Constants : register(b0) {
	float4x4 LocalToWorld;
	float4x4 WorldToProjective;
	float Time;
}

VSOutput main(VSInput input_, uint instID_ : SV_InstanceID) {
	VSOutput output;
	output.Position = input_.Position;
	output.Position.xyz *= 1.0f + instID_ * 1.2f;
	output.Position = mul(output.Position, LocalToWorld);
	output.Position = mul(output.Position, WorldToProjective);
	output.TexCoord = input_.TexCoord;
	output.TexCoord.x += Time * 0.1f + instID_ * 1.2f;
	output.TexCoord.y += cos(Time * 7.0f + input_.Normal.x * 5.0f) * 0.1f + 0.1f;
	output.Normal = mul(input_.Normal, (float3x3) WorldToProjective);
	output.LocalNormal = input_.Normal;
	return output;
}