struct TessellationPatch {
	float EdgeTess[3] : SV_TessFactor;
	float InsideTess[1] : SV_InsideTessFactor;
};

struct VSOutput {
	float3 Position : SV_POSITION;
	float2 UV : UV0;
	float3 Normal : NORMAL0;
	float3 Tangent : TANGENT0;
	float3 Bitangent : BITANGENT0;
};

struct DSOutput {
	float4 Position : SV_POSITION;
	float2 UV : UV0;
	float3 Normal : NORMAL0;
	float3 Tangent : TANGENT0;
	float3 Bitangent : BITANGENT0;
};

cbuffer Paramaters_Space0Slot0 : register(b0, space0) {
	float4x4 Matrix_WorldToProjective;
	float Scale_SurfaceElevation;
	float Scale_MaterialElevation;
}

static const float Scale_Bary = 125.0f;
static const float Offset_Elevation = -0.25f;

struct Material {
	uint ID_Albedo;
	uint ID_Normal;
	uint ID_Elevation;
};

StructuredBuffer<Material> MaterialDatabase : register(t0, space64);

// * Procedurally Generated Surface Information
namespace Surface {
	// * R8_UINT
	Texture2D<uint> Material : register(t0, space72);
	// * R16G16B16A16_FLOAT
	Texture2D<float4> BlendAndElevation : register(t1, space72);
	// * R8G8B8A8_UNORM
	Texture2D<float4> Normal : register(t2, space72);
}

// * Images Used by Materials
namespace Maps {
	Texture2D<float4> Albedo[] : register(t0, space80);
	Texture2D<float4> Normal[] : register(t0, space81);
	Texture2D<float4> Elevation[] : register(t0, space82);
}

SamplerState BilinearWrap : register(s0);

uint2 GetCoord(in float2 uv_) {
	const float2 coord = uv_ * float2(512.0f, 384.0f);
	return uint2(coord);
}

float GetMaterialElevation(in float2 uv_) {
	const uint2 coord = GetCoord(uv_);
			
	const uint id_Material = Surface::Material.Load(int3(coord, 0));
	const Material material = MaterialDatabase[id_Material];
	
	return Maps::Elevation[material.ID_Elevation].Sample(BilinearWrap, uv_).r;
}

[domain("tri")]
DSOutput main(
	TessellationPatch tessPatch_,
	float3 bary_ : SV_DomainLocation,
	const OutputPatch<VSOutput, 3> hsPatch_
) {
	DSOutput output;
	
	float3 pos =
		bary_.x * hsPatch_[0].Position +
		bary_.y * hsPatch_[1].Position +
		bary_.z * hsPatch_[2].Position;
	const float2 uv =
		bary_.x * hsPatch_[0].UV +
		bary_.y * hsPatch_[1].UV +
		bary_.z * hsPatch_[2].UV;
	const float3 normal = normalize(
		bary_.x * hsPatch_[0].Normal +
		bary_.y * hsPatch_[1].Normal +
		bary_.z * hsPatch_[2].Normal
	);
	const float3 tangent = normalize(
		bary_.x * hsPatch_[0].Tangent +
		bary_.y * hsPatch_[1].Tangent +
		bary_.z * hsPatch_[2].Tangent
	);
	const float3 bitangent = normalize(
		bary_.x * hsPatch_[0].Bitangent +
		bary_.y * hsPatch_[1].Bitangent +
		bary_.z * hsPatch_[2].Bitangent
	);
	
	const float elevation_Surface = Surface::BlendAndElevation.Sample(BilinearWrap, uv).a;
	const float elevation_Material = GetMaterialElevation(uv);
	
	const float factor_Bary = bary_.x * bary_.y * bary_.z * Scale_Bary;
	pos += normal * (elevation_Surface * Scale_SurfaceElevation + elevation_Material * Scale_MaterialElevation + Offset_Elevation) * factor_Bary;
	
	//pos.y += pos.z * 0.1f;
	
	output.Position = mul(float4(pos, 1.0f), Matrix_WorldToProjective);
	output.UV = uv;
	output.Normal = normal;
	output.Tangent = tangent;
	output.Bitangent = bitangent;

	return output;
}