struct PatchTessellation {
	float EdgeTess[3] : SV_TessFactor;
	float InsideTess : SV_InsideTessFactor;
};

struct VSOutput {
	float3 Position : SV_POSITION;
	float2 UV : UV0;
	float3 Normal : NORMAL0;
	float3 Tangent : TANGENT0;
	float3 Bitangent : BITANGENT0;
};

cbuffer Paramaters_Space0Slot2 : register(b2, space0) {
	float3 WorldPosition_Camera;
	// * Distance at which `Tessellation_MAX` is applied
	float Distance_MIN;
	// * Distance at which `Tessellation_MIN` is applied
	float Distance_MAX;
	float Tessellation_MIN;
	float Tessellation_MAX;
};

// * Distance-based LOD helper
float CalculateTessellationFactor(in float3 worldPos_) {
	//const float dist = distance(worldPos_, WorldPosition_Camera);
	
 //   // * [Distance_MIN, Distance_MAX] -> [Tessellation_MAX, Tessellation_MIN]
	//const float t = saturate((dist - Distance_MIN) / (Distance_MAX - Distance_MIN));
	//return pow(2.0f, lerp(Tessellation_MAX, Tessellation_MIN, t));
	return 64.0f;
}

PatchTessellation CalculatePatchConstant(
	InputPatch<VSOutput, 3> patch_,
	uint patchID_ : SV_PrimitiveID
) {
	PatchTessellation ret;

    // * Tessellate based on midpoint of each edge
	const float3 edge_0 = 0.5f * (patch_[1].Position + patch_[2].Position);
	const float3 edge_1 = 0.5f * (patch_[2].Position + patch_[0].Position);
	const float3 edge_2 = 0.5f * (patch_[0].Position + patch_[1].Position);
	const float3 center = (patch_[0].Position + patch_[1].Position + patch_[2].Position) / 3.0f;

	ret.EdgeTess[0] = CalculateTessellationFactor(edge_0);
	ret.EdgeTess[1] = CalculateTessellationFactor(edge_1);
	ret.EdgeTess[2] = CalculateTessellationFactor(edge_2);
	ret.InsideTess = CalculateTessellationFactor(center);

	return ret;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalculatePatchConstant")]
[maxtessfactor(64.0f)]
VSOutput main(
	InputPatch<VSOutput, 3> patch_,
	uint id_OuputControlPoint_ : SV_OutputControlPointID,
	uint id_Primitive_ : SV_PrimitiveID
) {
    // Pass control points straight through
	return patch_[id_OuputControlPoint_];
}