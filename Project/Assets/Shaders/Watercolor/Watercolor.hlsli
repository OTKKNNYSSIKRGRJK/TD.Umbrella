namespace Watercolor {
	cbuffer Parameters : register(b0) {
		float2 TexelSize;
		float2 UVStep;
		float Weight_Luminance;
		float Weight_Depth;
		float Time;
	};
	
	namespace Input {
		namespace Simulation {
			Texture2D<float> Wetness : register(t0, space0);
			//Texture2D<float4> Pigment : register(t1, space0);
			Texture2D<float> Pigment : register(t1, space0);
		}
		
		namespace Geometry {
			// * Scene rendered with lighting applied
			Texture2D<float4> Albedo : register(t0, space1);
			// * R - Bleeding
			// * G - EdgeDensity
			Texture2D<float4> Factors0 : register(t1, space1);
			//Texture2D<float> Curvature : register(t2, space1);
			Texture2D<float> Depth : register(t2, space1);
			//Texture2D<float4> WetnessAndPigment : register(t3, space1);
		}
		
		namespace Substrate {
			Texture2D<float4> Albedo : register(t0, space2);
			Texture2D<float4> Normal : register(t1, space2);
			Texture2D<float> Height : register(t2, space2);
		}
		
		Texture2D<float> Edge : register(t0, space3);
		Texture2D<float> EdgeDensity : register(t1, space3);
		Texture2D<float4> BlurH : register(t2, space3);
		Texture2D<float4> BlurV : register(t3, space3);
		Texture2D<float> Noise : register(t4, space3);
	}
	
	namespace Output {
		namespace Simulation {
			RWTexture2D<float> Wetness : register(u0, space0);
			//RWTexture2D<float4> Pigment : register(u1, space0);
			RWTexture2D<float> Pigment : register(u1, space0);
		}
		
		RWTexture2D<float> Edge : register(u0, space3);
		RWTexture2D<float> EdgeDensity : register(u1, space3);
		RWTexture2D<float4> BlurH : register(u2, space3);
		RWTexture2D<float4> BlurV : register(u3, space3);
		RWTexture2D<float> Noise : register(u4, space3);
		RWTexture2D<float4> Composite : register(u5, space3);
	}
}

SamplerState BilinearClamp : register(s0);