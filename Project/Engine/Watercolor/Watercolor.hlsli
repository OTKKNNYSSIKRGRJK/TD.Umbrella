namespace SimulationMap {
	namespace SRV {
		Texture2D<float> Wetness : register(t0);
		Texture2D<float4> PigmentConcentration : register(t1);
	}
	
	namespace UAV {
		RWTexture2D<float> Wetness : register(u0);
		RWTexture2D<float4> PigmentConcentration : register(u1);
	}
}

namespace GBuffer {
	namespace SRV {
		Texture2D<float4> AlbedoWetness : register(t0, space1);
		Texture2D<float2> Normal : register(t1, space1);
		Texture2D<float> Curvature : register(t2, space1);
	}
}

Texture2D<float> BrushMap : register(t0, space2);

Texture2D<float4> ImageTexture_Paper : register(t0, space3);

SamplerState BilinearClamp : register(s0);