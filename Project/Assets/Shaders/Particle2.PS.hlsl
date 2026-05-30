struct GLOBAL_DATA {
	float4x4 ViewToWorld;
};

ConstantBuffer<GLOBAL_DATA> GlobalData : register(b0, space1);

struct PSInput {
	float4 Position : SV_POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
	uint TexID : TEXID0;
};

struct PSOutput {
	float4 Color : SV_TARGET0;
	//float4 Factors0 : SV_TARGET1;
};

Texture2D<float4> ImageTextures[] : register(t0, space1);
Texture2D<float4> GBuffer_Albedo : register(t0, space2);
Texture2D<float4> GBuffer_Normal : register(t0, space2);
Texture2D<float> GBuffer_Depth : register(t3, space2);
SamplerState Sampler : register(s0);

//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////

PSOutput main(in PSInput input_) {
	PSOutput output;
	
	const float4 texColor = ImageTextures[input_.TexID].Sample(Sampler, input_.TexCoord);
	output.Color = texColor * input_.Color;
	
	//output.Color = float4(input_.Position.xy * float2(1.0f / 1280.0f, 1.0f / 720.0f) + 0.5f, 1.0f, 0.75f);
	
	const float depth = GBuffer_Depth.Load(int3(input_.Position.xy, 0.0f));
	output.Color.a = depth < input_.Position.z ? output.Color.a * 0.1f : output.Color.a;
	
	//output.Factors0 = float4(0.5f, 0.0f, 0.0f, 0.0f);
	
	return output;
}