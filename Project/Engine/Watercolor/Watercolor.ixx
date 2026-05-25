export module Lumina.Watercolor;

//import <d3d12.h>;

import Lumina.Core.String;
import Lumina.D3D12;

namespace Lumina {
	export class Watercolor {
	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Update_(_ARGs&&...args_) -> void;
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Render_(_ARGs&&...args_) -> void;
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Initialize_(_ARGs&&...args_) -> void;

	public:
		auto Update(float deltaTime_) -> void;
		auto Render(D3D12::DescriptorTable const& gBuffers_) -> void;

	public:
		auto GlobalTable() const noexcept -> D3D12::DescriptorTable const&;

	public:
		auto Initialize() -> void;

	public:
		enum class VIEW_NAME : uint32_t {
			CBV_CONSTANTS,

			SRV_PAPER_TEXTURE_ALBEDO,
			SRV_PAPER_TEXTURE_NORMAL,
			SRV_PAPER_TEXTURE_HEIGHT,

			SRV_EDGE,
			SRV_EDGEDENSITY,
			SRV_BLURH,
			SRV_BLURV,
			SRV_NOISE,
			SRV_COMPOSITE,

			UAV_EDGE,
			UAV_EDGEDENSITY,
			UAV_BLURH,
			UAV_BLURV,
			UAV_NOISE,
			UAV_COMPOSITE,

			SRV_WETNESS_0,
			UAV_WETNESS_1,

			SRV_WETNESS_1,
			UAV_WETNESS_0,

			SRV_PIGMENT_0,
			UAV_PIGMENT_1,

			SRV_PIGMENT_1,
			UAV_PIGMENT_0,

			COUNT,
		};

	private:
		D3D12::DescriptorHeap LocalHeap_RTV_;
		D3D12::DescriptorHeap LocalHeap_DSV_;
		D3D12::DescriptorTable GlobalTable_CBVSRVUAV_;

		D3D12::RenderTexture2D RT_Watercolor_;

		D3D12::ComputeTexture2D CT_Edge_;
		D3D12::ComputeTexture2D CT_EdgeDensity_;
		D3D12::ComputeTexture2D CT_BlurH_;
		D3D12::ComputeTexture2D CT_BlurV_;
		D3D12::ComputeTexture2D CT_Noise_;
		D3D12::ComputeTexture2D CT_Composite_;

		D3D12::ComputeTexture2D CT_Wetness_[2];
		D3D12::ComputeTexture2D CT_Pigment_[2];
		int IDX_PingPong_Wetness_{ 0 };
		int IDX_PingPong_Pigment_{ 0 };

		D3D12::UploadBuffer UB_Constants_;

		D3D12::RootSignature RS_ComputeCommon_;

		D3D12::Shader CS_Noise_;
		D3D12::ComputePSO PSO_Noise_;

		D3D12::Shader CS_EdgeDetection_;
		D3D12::ComputePSO PSO_EdgeDetection_;
		D3D12::Shader CS_EdgeDensity_;
		D3D12::ComputePSO PSO_EdgeDensity_;
		D3D12::Shader CS_BlurHorizontal_;
		D3D12::ComputePSO PSO_BlurHorizontal_;
		D3D12::Shader CS_BlurVertical_;
		D3D12::ComputePSO PSO_BlurVertical_;
		D3D12::Shader CS_Pigment_;
		D3D12::ComputePSO PSO_Pigment_;

		D3D12::Shader CS_Composite_;
		D3D12::ComputePSO PSO_Composite_;

		D3D12::RenderPass WatercolorPass_;

		struct Constants {
			F32x2 TexelSize;
			F32x2 UVStep;
			F32 Weight_Luminance;
			F32 Weight_Depth;
			F32 Time;
		};
		Constants Constants_;
	};
}