struct PSInput {
	float4 ScreenPosition : SV_POSITION;
	float2 UV : UV0;
	float3 Normal : NORMAL0;
	float3 Tangent : TANGENT0;
	float3 Bitangent : BITANGENT0;
};

struct PSOutput {
	float4 Albedo : SV_TARGET0;
	float4 Normal : SV_TARGET1;
	//float4 Specular : SV_TARGET3;
};

cbuffer Parameters : register(b0, space64) {
	float2 Scale_SurfaceBlendUV;
	float Scale_SurfaceNormal;
	float Scale_MaterialNormal;
}

struct Material {
	uint ID_Albedo;
	uint ID_Normal;
	uint ID_Height;
};

StructuredBuffer<Material> MaterialDatabase : register(t0, space64);

// * Procedurally Generated Surface Information
namespace Surface {
	Texture2D<uint> Material : register(t0, space72);
	Texture2D<float3> Blend : register(t1, space72);
	Texture2D<float4> NormalHeight : register(t2, space72);
}

// * Images Used by Materials
namespace Maps {
	Texture2D<float4> Albedo[] : register(t0, space80);
	Texture2D<float4> Normal[] : register(t0, space81);
	Texture2D<float> Height[] : register(t0, space82);
}

namespace Output {
	RWTexture2D<float> Height : register(u0, space96);
}

SamplerState BilinearWrap : register(s0);
SamplerState PointWrap : register(s1);

float BlendMap(
	in Texture2D<float> maps_[],
	uint id_Map_0_,
	uint id_Map_1_,
	in float2 uv_0_,
	in float2 uv_1_,
	float lerpFactor_
) {
	const Texture2D<float> map_0 = maps_[id_Map_0_];
	const Texture2D<float> map_1 = maps_[id_Map_1_];
	
	const float4 val_0 = map_0.Sample(BilinearWrap, uv_0_);
	const float4 val_1 = map_1.Sample(BilinearWrap, uv_1_);
	
	return val_0 * (1.0f - lerpFactor_) + val_1 * lerpFactor_;
}

float4 BlendMap(
	in Texture2D<float4> maps_[],
	uint id_Map_0_,
	uint id_Map_1_,
	in float2 uv_0_,
	in float2 uv_1_,
	float lerpFactor_
) {
	const Texture2D<float4> map_0 = maps_[id_Map_0_];
	const Texture2D<float4> map_1 = maps_[id_Map_1_];
	
	const float4 val_0 = map_0.Sample(BilinearWrap, uv_0_);
	const float4 val_1 = map_1.Sample(BilinearWrap, uv_1_);
	
	return val_0 * (1.0f - lerpFactor_) + val_1 * lerpFactor_;
}

void BlendMaterial(
	out float4 albedo_,
	out float3 normal_,
	out float height_,
	in float2 uv_
) {
	const float3 blend = Surface::Blend.Sample(BilinearWrap, uv_);
	const float2 blendGradient = blend.rg;
	const float blendLERPFactor = blend.b;
	
	const float2 uv_1 = uv_ + blendGradient * Scale_SurfaceBlendUV;
	
	const uint id_Material_0 = Surface::Material.Sample(PointWrap, uv_);
	const uint id_Material_1 = Surface::Material.Sample(PointWrap, uv_1);
	
	const Material material_0 = MaterialDatabase[id_Material_0];
	const Material material_1 = MaterialDatabase[id_Material_1];

	const float4 albedo = BlendMap(
		Maps::Albedo,
		material_0.ID_Albedo,
		material_1.ID_Albedo,
		uv_,
		uv_1,
		blendLERPFactor
	);
	// * [0, 1]
	albedo_ = saturate(albedo);
	
	const float3 normal = BlendMap(
		Maps::Normal,
		material_0.ID_Normal,
		material_1.ID_Normal,
		uv_,
		uv_1,
		blendLERPFactor
	).rgb;
	// * [0, 1] -> [-1, 1]
	normal_ = normal * 2.0f - 1.0f;
	
	height_ = BlendMap(
		Maps::Height,
		material_0.ID_Height,
		material_1.ID_Height,
		uv_,
		uv_1,
		blendLERPFactor
	);
}

float3 BlendNormal(
	in float3 normal_Polygon_,
	in float3 normal_Surface_,
	in float3 normal_Material_
) {
	return normalize(
		normal_Polygon_ +
		normal_Surface_ * Scale_SurfaceNormal +
		normal_Material_ * Scale_MaterialNormal
	);
}

float3 CalculateSurfaceMaterialNormal(
	in PSInput input_,
	in float3 normal_Surface_,
	in float3 normal_Material_
) {
	const float3 normal_Untransfromed =
		normal_Surface_ * Scale_SurfaceNormal +
		normal_Material_ * Scale_MaterialNormal;
	
	const float3 normal_Transfromed =
		input_.Tangent * normal_Untransfromed.x +
		input_.Bitangent * normal_Untransfromed.y +
		input_.Normal * normal_Untransfromed.z;
	return normalize(normal_Transfromed);
}

PSOutput main(in PSInput input_) {
	float4 albedo_Material;
	float3 normal_Material;
	float height_Material;
	BlendMaterial(
		albedo_Material,
		normal_Material,
		height_Material,
		input_.UV
	);
	
	const float4 normalHeight_Surface = Surface::NormalHeight.Sample(BilinearWrap, input_.UV);
	const float3 normal_Surface = normalHeight_Surface.rgb;
	const float height_Surface = normalHeight_Surface.a;
	
	const float3 normal = CalculateSurfaceMaterialNormal(
		input_,
		normal_Surface,
		normal_Material
	);

	PSOutput output;
	output.Albedo = albedo_Material;
	output.Normal.rgb = normal;
	
	const uint2 coord_UAV = uint2(input_.ScreenPosition.xy);
	Output::Height[coord_UAV] = height_Material + height_Surface;

	return output;
}