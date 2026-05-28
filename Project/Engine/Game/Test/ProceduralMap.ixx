module;

#include<d3d12.h>

export module Lumina.ProceduralMap;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import Lumina.Core.Common;
import Lumina.Core.Math;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import Lumina.Core.String;

namespace Lumina {
	export class ProceduralMap {
	public:
		struct Parameter {
			F32x3 Offset;
			F32 Frequency;
			F32 Persistence;
			U32 NUM_Octaves;
		};

	public:
		auto Texture() const noexcept
			-> D3D12::ComputeTexture2D const& { return Texture_; }
		auto LocalUAV() const noexcept
			-> D3D12_CPU_DESCRIPTOR_HANDLE { return LocalHeap_.CPUHandle(0U); }
		auto LocalSRV() const noexcept
			-> D3D12_CPU_DESCRIPTOR_HANDLE { return LocalHeap_.CPUHandle(1U); }

	protected:
		template<StringLiteral _Name, typename..._ARGs>
		auto Render(_ARGs&&...args_) -> void;

	public:
		template<typename..._ARGs>
		auto Render(_ARGs&&...args_) -> void;

	protected:
		template<StringLiteral _Name, typename..._ARGs>
		auto Initialize(_ARGs&&...args_) -> void;

	public:
		auto Initialize(
			D3D12::Context const& d3d12Context_,
			U32 const width_,
			U32 const height_,
			DXGI_FORMAT const format_
		) -> void;

	public:
		ProceduralMap();
		~ProceduralMap();

	protected:
		D3D12::ComputeTexture2D Texture_;
		D3D12::DescriptorHeap LocalHeap_;
		D3D12::CommandAllocator ComputeAllocator_{};
		D3D12::CommandList ComputeList_{};
		D3D12::CommandAllocator DirectAllocator_{};
		D3D12::CommandList DirectList_{};

		U32 Width_;
		U32 Height_;

		/*D3D12::RootSignature NoiseRS_{};
		D3D12::Shader NoiseShader_{};
		D3D12::ComputePSO NoisePSO_{};*/

		//D3D12::RootSignature RenderRS_{};
		//D3D12::Shader RenderVertexShader_{};
		//D3D12::Shader RenderPixelShader_{};
		//D3D12::GraphicsPSO RenderPSO_{};

		/*NoiseParam ElevationNoiseParam_{
			.Frequency{ 0.4f },
			.Redist{ 2.0f },
			.Offset{ 0.0f, 0.0f, 0.0f },
			.Num_Octaves{ 8U },
			.Persistance{ 0.5f },
		};
		float Insulation_{ 0.3f };
		int IsFormingTerraces_{ 0 };
		float TerraceFactor_{ 8.0f };

		NoiseParam TemperatureNoiseParam_{
			.Frequency{ 1.0f },
			.Redist{ 1.0f },
			.Offset{ 1.0f, 3.0f, 5.0f },
			.Num_Octaves{ 2U },
			.Persistance{ 0.25f },
		};
		NoiseParam PrecipitationNoiseParam_{
			.Frequency{ 2.0f },
			.Redist{ 1.0f },
			.Offset{ 2.0f, 3.0f, 4.0f },
			.Num_Octaves{ 2U },
			.Persistance{ 0.25f },
		};*/
	};
}