export module Lumina.DeferredLighting;

import <cstdint>;

import <vector>;
import <array>;

import Lumina.Core.Common;
import Lumina.Core.Math;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux.View;

namespace Lumina {
	export struct PointLight {
		F32x4 WorldPosition;
		F32x3 RGB;
		float Intensity;
	};

	export float LightSphereRadius(
		F32 inv_Threshold_,
		F32 maxComp_intensity_,
		F32 factor_Constant_,
		F32 factor_Linear_,
		F32 factor_Quadratic_
	) {
		return std::sqrt(
			-factor_Linear_ +
			std::sqrt(
				factor_Linear_ * factor_Linear_ -
				4.0f * factor_Quadratic_ * (factor_Constant_ - maxComp_intensity_ * inv_Threshold_)
			) / (2.0f * factor_Quadratic_)
		);
	}

	export class DeferredLighting {
	public:
		auto RenderTexture() const noexcept -> D3D12::RenderTexture2D const&;

	public:
		void Update(
			List<PointLight> const& list_PointLight_,
			List<Math::F32x4x4<>> const& list_WorldMatrix_LightSphere_,
			std::vector<U32> const& arr_Index_ActivePointLight_
		);
		void Render(
			D3D12::GraphicsDevice const& device_,
			D3D12::CommandList const& cmdList_,
			D3D12::DescriptorTable const& globalSRV_Arr_GBuffer_,
			D3D12_CPU_DESCRIPTOR_HANDLE localCBV_WorldToNDC_,
			D3D12_CPU_DESCRIPTOR_HANDLE localCBV_ScreenToWorld_
		);

	public:
		void Initialize(
			D3D12::Context const& dxContext_,
			U32 canvasWidth_,
			U32 canvasHeight_
		);

	private:
		U32 MaxNum_PointLights_{ 2048U };
		U32 Num_ActivePointLights_{ 0U };

		D3D12::RenderPass RenderPass_{};
		D3D12::Canvas Canvas_{};

		D3D12::RootSignature RootSignature_{};
		D3D12::Shader VertexShader_{};
		D3D12::Shader PixelShader_{};
		D3D12::GraphicsPSO GraphicsPSO_{};

		D3D12::DescriptorTable GlobalTable_{};

		D3D12::UploadBuffer UB_Arr_PointLight_{};
		D3D12::UploadBuffer UB_Arr_WorldMatrix_LightSphere_{};
		D3D12::UploadBuffer UB_Arr_Index_ActivePointLight_{};

		D3D12::UploadBuffer UB_Vertices_LightSphere_{};
		D3D12::UploadBuffer UB_Indices_LightSphere_{};
		D3D12::VBV VBV_LightSphere_{};
		D3D12::IBV IBV_LightSphere_{};
		U32 Num_IndicesPerSphere_{};
	};
}