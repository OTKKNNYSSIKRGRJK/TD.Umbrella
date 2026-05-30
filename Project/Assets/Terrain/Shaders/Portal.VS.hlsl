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
	nointerpolation uint InstanceID : ID0;
};

cbuffer Constants : register(b0) {
	float4x4 WorldToProjective;
	float Time;
}

StructuredBuffer<float4x4> LocalToWorlds : register(t0);

VSOutput main(VSInput input_, uint id_Instance_ : SV_InstanceID) {
	VSOutput output;
	
	output.Position = mul(input_.Position, LocalToWorlds[id_Instance_]);
	output.Position = mul(output.Position, WorldToProjective);
	
	output.TexCoord = input_.TexCoord;
	output.TexCoord.x += Time * (0.1f + id_Instance_ * 0.05f);
	output.TexCoord.y += cos(Time * (7.0f + id_Instance_ * 3.0f)) * 0.1f + 0.1f;
	
	output.Normal = mul(input_.Normal, (float3x3) WorldToProjective);
	output.LocalNormal = input_.Normal;
	
	output.InstanceID = id_Instance_;
	
	return output;
}