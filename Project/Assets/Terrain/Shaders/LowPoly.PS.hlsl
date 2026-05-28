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
};

cbuffer Parameters_B0Space64 : register(b0, space64) {
	float2 Scale_SurfaceBlendUV;
	float Scale_SurfaceNormal;
	float Scale_MaterialNormal;
}

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

//namespace Output {
//	RWTexture2D<float> Elevation : register(u0, space96);
//}

SamplerState BilinearWrap : register(s0);
SamplerState PointWrap : register(s1);

uint2 GetCoord(in float2 uv_) {
	const float2 coord = uv_ * float2(512.0f, 384.0f);
	return uint2(coord);
}

static const int2 Offsets[25] = {
	{ -2, -2 }, { -1, -2 }, { 0, -2 }, { 1, -2 }, { 2, -2 },
	{ -2, -1 }, { -1, -1 }, { 0, -1 }, { 1, -1 }, { 2, -1 },
	{ -2, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 }, { 2, 0 },
	{ -2, 1 }, { -1, 1 }, { 0, 1 }, { 1, 1 }, { 2, 1 },
	{ -2, 2 }, { -1, 2 }, { 0, 2 }, { 1, 2 }, { 2, 2 },
};

static const float INV_273 = 1.0f / 273.0f;
static const float Kernel[25] = {
	1.0f * INV_273, 4.0f * INV_273, 7.0f * INV_273, 4.0f * INV_273, 1.0f * INV_273,
	4.0f * INV_273, 16.0f * INV_273, 26.0f * INV_273, 16.0f * INV_273, 4.0f * INV_273,
	7.0f * INV_273, 26.0f * INV_273, 41.0f * INV_273, 26.0f * INV_273, 7.0f * INV_273,
	4.0f * INV_273, 16.0f * INV_273, 26.0f * INV_273, 16.0f * INV_273, 4.0f * INV_273,
	1.0f * INV_273, 4.0f * INV_273, 7.0f * INV_273, 4.0f * INV_273, 1.0f * INV_273,
};

void Convolve(
	inout float4 albedo_,
	inout float3 normal_,
	inout float elevation_,
	in float2 uv_
) {
	albedo_ = float4(0.0f, 0.0f, 0.0f, 1.0f);
	normal_ = float3(0.0f, 0.0f, 0.0f);
	elevation_ = 0.0f;
	
	for (uint y = 0; y < 5; ++y) {
		for (uint x = 0; x < 5; ++x) {
			const uint idx = y * 5 + x;
			
			float2 uv = uv_ + Offsets[idx] * Scale_SurfaceBlendUV;
			uv = frac(uv) * 0.5f + 0.25f;
			const uint2 coord = GetCoord(uv);
			
			const uint id_Material = Surface::Material.Load(int3(coord, 0));
			const Material material = MaterialDatabase[id_Material];
			
			const float4 albedo_Fetch = Maps::Albedo[material.ID_Albedo].Sample(BilinearWrap, uv);
			albedo_.rgb += albedo_Fetch.rgb * Kernel[idx];
			
			const float4 normal_Fetch = Maps::Normal[material.ID_Normal].Sample(BilinearWrap, uv);
			normal_ += normal_Fetch.rgb * Kernel[idx];
			
			const float4 elevation_Fetch = Maps::Elevation[material.ID_Elevation].Sample(BilinearWrap, uv);
			elevation_ += elevation_Fetch.r * Kernel[idx];
		}
	}
	
	albedo_ = saturate(albedo_);
	normal_ = normalize(normal_) * 2.0f - 1.0f;
}

void BlendMaterial(
	out float4 albedo_,
	out float3 normal_,
	out float elevation_,
	in float2 uv_
) {
	Convolve(
		albedo_,
		normal_,
		elevation_,
		uv_
	);
	
	//const float4 blendAndElevation = Surface::BlendAndElevation.Sample(BilinearWrap, uv_);
	//const float2 blendGradient = blendAndElevation.rg;
	//const float blendLERPFactor = abs(blendAndElevation.b - 0.5f) * 2.0f;
	
	//const float2 uv_1 = uv_ + blendGradient * Scale_SurfaceBlendUV * 0.1f;
	
	//const uint2 coord_0 = GetCoord(uv_);
	//const uint2 coord_1 = GetCoord(uv_1);
	
	//const uint id_Material_0 = Surface::Material.Load(int3(coord_0, 0));
	//const uint id_Material_1 = Surface::Material.Load(int3(coord_1, 0));
	
	//const Material material_0 = MaterialDatabase[id_Material_0];
	//const Material material_1 = MaterialDatabase[id_Material_1];

	//const float4 albedo = BlendMap(
	//	Maps::Albedo,
	//	material_0.ID_Albedo,
	//	material_1.ID_Albedo,
	//	uv_,
	//	uv_1,
	//	blendLERPFactor
	//);
	//// * [0, 1]
	//albedo_ = saturate(albedo);
	
	//const float4 normal = BlendMap(
	//	Maps::Normal,
	//	material_0.ID_Normal,
	//	material_1.ID_Normal,
	//	uv_,
	//	uv_1,
	//	blendLERPFactor
	//);
	//// * [0, 1] -> [-1, 1]
	//normal_ = normal.rgb * 2.0f - 1.0f;
	
	//const float4 elevation = BlendMap(
	//	Maps::Elevation,
	//	material_0.ID_Elevation,
	//	material_1.ID_Elevation,
	//	uv_,
	//	uv_1,
	//	blendLERPFactor
	//);
	
	//elevation_ = elevation.r;
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
	const float3 normal_Untransfromed = BlendNormal(input_.Normal, normal_Surface_, normal_Material_);
	
	const float3 normal_Transfromed =
		input_.Tangent * normal_Untransfromed.x +
		input_.Bitangent * normal_Untransfromed.y +
		input_.Normal * normal_Untransfromed.z;
	return normalize(normal_Transfromed);
}

PSOutput main(in PSInput input_) {
	float4 albedo_Material;
	float3 normal_Material;
	float elevation_Material;
	BlendMaterial(
		albedo_Material,
		normal_Material,
		elevation_Material,
		input_.UV
	);
	
	const float3 normal_Surface = Surface::Normal.Sample(BilinearWrap, input_.UV).rgb * 2.0f - 1.0f;
	const float elevation_Surface = Surface::BlendAndElevation.Sample(BilinearWrap, input_.UV).a;
	
	const float3 normal = CalculateSurfaceMaterialNormal(
		input_,
		normal_Surface,
		normal_Material
	);

	PSOutput output;
	output.Albedo = albedo_Material;
	output.Albedo.rgb *= float3(0.25f, 0.25f, 0.25f);
	// * [-1, 1] -> [0, 1]
	output.Normal.rgb = (normal + 1.0f) * 0.5f;
	output.Normal.a = 1.0f;
	
	//const float4 blendAndElevation = Surface::BlendAndElevation.Sample(BilinearWrap, input_.UV);
	//const float2 blendGradient = blendAndElevation.rg;
	//const float blendLERPFactor = abs(blendAndElevation.b - 0.5f) * 2.0f;
	//output.Albedo.rgb = blendLERPFactor;

	return output;
}