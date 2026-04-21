module Lumina.DeferredLighting;

import <numbers>;

import nlohmann.json;

import Lumina.Utils.Data;

import Lumina.D3D12.Aux;

namespace Lumina {
	namespace {
		using Mat4 = Math::F32x4x4<>;
	}
}

namespace Lumina {
	namespace {
		template<U32 Div_Latitudinal = 12U, U32 Div_Longitudinal = 24U>
			requires(Div_Latitudinal > 0U && Div_Longitudinal > 0U)
		void CreateSphere(
			std::vector<F32x4>& vertices_,
			std::vector<U32>& indices_,
			F32 radius_
		) {
			constexpr F32 inv_Div_Lat{ 1.0f / static_cast<float>(Div_Latitudinal) };
			constexpr F32 delta_Lat{ 1.0f * std::numbers::pi_v<F32> * inv_Div_Lat };
			constexpr F32 inv_Div_Lng{ 1.0f / static_cast<float>(Div_Longitudinal) };
			constexpr F32 delta_Lng{ 2.0f * std::numbers::pi_v<F32> * inv_Div_Lng };

			std::array<F32, Div_Latitudinal + 1U> arr_Cos_Lat{};
			std::array<F32, Div_Latitudinal + 1U> arr_Sin_Lat{};
			std::array<F32, Div_Longitudinal + 1U> arr_Cos_Lng{};
			std::array<F32, Div_Longitudinal + 1U> arr_Sin_Lng{};
			for (I32 i_Lat{ 0 }; i_Lat <= Div_Latitudinal; ++i_Lat) {
				F32 const lat{ i_Lat * delta_Lat - 0.5f * std::numbers::pi_v<float> };
				arr_Cos_Lat[i_Lat] = std::cos(lat);
				arr_Sin_Lat[i_Lat] = std::sin(lat);
			}
			for (I32 i_Lng{ 0 }; i_Lng <= Div_Longitudinal; ++i_Lng) {
				float const lng{ i_Lng * delta_Lng };
				arr_Cos_Lng[i_Lng] = std::cos(lng);
				arr_Sin_Lng[i_Lng] = std::sin(lng);
			}

			for (I32 i_Lat{ 0 }; i_Lat <= Div_Latitudinal; ++i_Lat) {
				for (I32 i_Lng{ 0 }; i_Lng <= Div_Longitudinal; ++i_Lng) {
					auto& vert = vertices_.emplace_back();
					{
						vert.X = arr_Cos_Lat[i_Lat] * arr_Cos_Lng[i_Lng] * radius_;
						vert.Z = arr_Cos_Lat[i_Lat] * arr_Sin_Lng[i_Lng] * radius_;
						vert.Y = arr_Sin_Lat[i_Lat] * radius_;
						vert.W = 1.0f;
					}
				}
			}

			for (I32 i_Lat{ 0 }; i_Lat < Div_Latitudinal; ++i_Lat) {
				for (I32 i_Lng{ 0 }; i_Lng < Div_Longitudinal; ++i_Lng) {
					indices_.emplace_back(i_Lat * (Div_Longitudinal + 1) + i_Lng);
					indices_.emplace_back((i_Lat + 1) * (Div_Longitudinal + 1) + i_Lng);
					indices_.emplace_back((i_Lat + 1) * (Div_Longitudinal + 1) + (i_Lng + 1));
					indices_.emplace_back(i_Lat * (Div_Longitudinal + 1) + i_Lng);
					indices_.emplace_back((i_Lat + 1) * (Div_Longitudinal + 1) + (i_Lng + 1));
					indices_.emplace_back(i_Lat * (Div_Longitudinal + 1) + (i_Lng + 1));
				}
			}
		}

		enum class GlobalTableEntry : U32 {
			SRV_Array_WorldMatrix_LightSphere = 0U,
			CBV_Matrix_WorldToNDC = 1U,
			SRV_Array_PointLight = 2U,
			CBV_Matrix_ScreenToWorld = 3U,
			SRV_Array_Index_ActivePointLight = 4U,
		};

		enum class RootParameterEntry : U32 {
			V_Transform = 0U,
			P_PointLight = 1U,
			V_Index = 2U,
			P_GBuffer = 3U,
		};
	}

	auto DeferredLighting::RenderTexture() const noexcept -> D3D12::RenderTexture2D const& {
		return Canvas_.RenderTexture(0U);
	}

	void DeferredLighting::Update(
		List<PointLight> const& list_PointLight_,
		List<Mat4> const& list_WorldMatrix_LightSphere_,
		std::vector<U32> const& arr_Index_ActivePointLight_
	) {
		U32 const num{ std::min<U32>(MaxNum_PointLights_, list_PointLight_.Capacity()) };
		UB_Arr_PointLight_.Store(
			list_PointLight_.Data(),
			sizeof(PointLight) * num,
			0LLU
		);
		UB_Arr_WorldMatrix_LightSphere_.Store(
			list_WorldMatrix_LightSphere_.Data(),
			sizeof(Mat4) * num,
			0LLU
		);

		Num_ActivePointLights_ = std::min<U32>(
			MaxNum_PointLights_,
			static_cast<U32>(arr_Index_ActivePointLight_.size())
		);
		UB_Arr_Index_ActivePointLight_.Store(
			arr_Index_ActivePointLight_.data(),
			sizeof(U32) *
			Num_ActivePointLights_,
			0LLU
		);
	}

	void DeferredLighting::Render(
		D3D12::GraphicsDevice const& device_,
		D3D12::CommandList const& cmdList_,
		D3D12::DescriptorTable const& globalSRV_Arr_GBuffer_,
		D3D12_CPU_DESCRIPTOR_HANDLE localCBV_WorldToNDC_,
		D3D12_CPU_DESCRIPTOR_HANDLE localCBV_ScreenToWorld_
	) {
		device_->CopyDescriptorsSimple(
			1U,
			GlobalTable_.CPUHandle(GlobalTableEntry::CBV_Matrix_WorldToNDC),
			localCBV_WorldToNDC_,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
		device_->CopyDescriptorsSimple(
			1U,
			GlobalTable_.CPUHandle(GlobalTableEntry::CBV_Matrix_ScreenToWorld),
			localCBV_ScreenToWorld_,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		D3D12_RESOURCE_BARRIER const barriers_PreLightingRender[]{
			Lumina::D3D12::Barrier::Transition(
				Canvas_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			),
		};
		D3D12_RESOURCE_BARRIER const barriers_PostLightingRender[]{
			Lumina::D3D12::Barrier::Transition(
				Canvas_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
		};

		cmdList_->ResourceBarrier(1U, barriers_PreLightingRender);

		RenderPass_.Begin(cmdList_);
		{
			cmdList_->RSSetViewports(1U, &Canvas_.Viewport(0U));
			cmdList_->RSSetScissorRects(1U, &Canvas_.ScissorRect(0U));

			cmdList_->SetGraphicsRootSignature(RootSignature_.Get());
			cmdList_->SetPipelineState(GraphicsPSO_.Get());
			cmdList_->SetGraphicsRootDescriptorTable(
				static_cast<uint32_t>(RootParameterEntry::V_Transform),
				GlobalTable_.GPUHandle(GlobalTableEntry::SRV_Array_WorldMatrix_LightSphere)
			);
			cmdList_->SetGraphicsRootDescriptorTable(
				static_cast<uint32_t>(RootParameterEntry::P_PointLight),
				GlobalTable_.GPUHandle(GlobalTableEntry::SRV_Array_PointLight)
			);
			cmdList_->SetGraphicsRootDescriptorTable(
				static_cast<uint32_t>(RootParameterEntry::V_Index),
				GlobalTable_.GPUHandle(GlobalTableEntry::SRV_Array_Index_ActivePointLight)
			);
			cmdList_->SetGraphicsRootDescriptorTable(
				static_cast<uint32_t>(RootParameterEntry::P_GBuffer),
				globalSRV_Arr_GBuffer_.GPUHandle(0U)
			);

			cmdList_->IASetVertexBuffers(0U, 1U, &VBV_LightSphere_);
			cmdList_->IASetIndexBuffer(&IBV_LightSphere_);
			cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			if (Num_ActivePointLights_ > 0U) {
				cmdList_->DrawIndexedInstanced(
					Num_IndicesPerSphere_,
					Num_ActivePointLights_,
					0U, 0U, 0U
				);
			}
		}
		RenderPass_.End();

		cmdList_->ResourceBarrier(1U, barriers_PostLightingRender);
	}

	void DeferredLighting::Initialize(
		D3D12::Context const& dxContext_,
		uint32_t canvasWidth_,
		uint32_t canvasHeight_
	) {
		auto const& device{ dxContext_.Device() };
		auto& cmdQueue{ dxContext_.DirectQueue() };

		Canvas_.AllocateTextures(1U, false);
		Canvas_.RenderTexture(0U).Initialize(
			device,
			canvasWidth_,
			canvasHeight_,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			{ 0.0f, 0.0f, 0.0f, 1.0f }
		);
		Canvas_.TransitionResourceStates(device, cmdQueue);
		Canvas_.CreateViews(device);
		Canvas_.Viewport(0U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ static_cast<float>(canvasWidth_) },
			.Height{ static_cast<float>(canvasHeight_) },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_.ScissorRect(0U) = D3D12_RECT{
			.left{ 0 },
			.top{ 0 },
			.right{ static_cast<I32>(canvasWidth_) },
			.bottom{ static_cast<I32>(canvasHeight_) },
		};

		F32 const clearColor[4]{ 0.0f, 0.0f, 0.0f, 1.0f };
		RenderPass_.Initialize(1U, false);
		RenderPass_.RenderTarget(0U).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			clearColor
		);
		RenderPass_.RenderTarget(0U).EndingEvent().Preserve();
		RenderPass_.RenderTarget(0U).View() = Canvas_.RTV(0U);

		auto config{
			Lumina::Utils::LoadFromFile<nlohmann::json>("Assets/Configs/Lighting.json")
		};
		RootSignature_.Initialize(
			device,
			Lumina::D3D12::LoadRootSignatureSetup(config.at("RS"))
		);

		dxContext_.Compile(
			VertexShader_,
			L"Assets/Shaders/Lighting.VS.hlsl",
			L"vs_6_6",
			L"main",
			"Lighting.VS"
		);
		dxContext_.Compile(
			PixelShader_,
			L"Assets/Shaders/Lighting.PS.hlsl",
			L"ps_6_6",
			L"main",
			"Lighting.PS"
		);

		Lumina::D3D12::BlendState blendState{};
		blendState.RenderTarget[0] = {
			.BlendEnable{ true },
			.LogicOpEnable{ false },
			.SrcBlend{ D3D12_BLEND_SRC_ALPHA },
			.DestBlend{ D3D12_BLEND_ONE },
			.BlendOp{ D3D12_BLEND_OP_ADD },
			.SrcBlendAlpha{ D3D12_BLEND_SRC_ALPHA },
			.DestBlendAlpha{ D3D12_BLEND_ONE },
			.BlendOpAlpha{ D3D12_BLEND_OP_ADD },
			.RenderTargetWriteMask{ D3D12_COLOR_WRITE_ENABLE_ALL },
		};
		Lumina::D3D12::RasterizerState rasterizerState{
			.FillMode{ D3D12_FILL_MODE_SOLID },
			.CullMode{ D3D12_CULL_MODE_FRONT },
		};

		Lumina::D3D12::DepthStencilState depthStencilState{
			.DepthEnable{ false },
			.StencilEnable{ false },
		};
		Lumina::D3D12::GraphicsPSO::InputLayout inputLayout{};
		inputLayout.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT);
		GraphicsPSO_.Initialize(
			device,
			RootSignature_,
			VertexShader_,
			PixelShader_,
			blendState,
			rasterizerState,
			depthStencilState,
			inputLayout,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, },
			DXGI_FORMAT_UNKNOWN
		);

		UB_Arr_PointLight_.Initialize(
			device,
			sizeof(PointLight) * MaxNum_PointLights_,
			"PointLight.Arr.UB"
		);
		UB_Arr_WorldMatrix_LightSphere_.Initialize(
			device,
			sizeof(Mat4)* MaxNum_PointLights_,
			"LightSphere.WorldMat.Arr.UB"
		);
		UB_Arr_Index_ActivePointLight_.Initialize(
			device,
			sizeof(uint32_t)* MaxNum_PointLights_,
			"ActivePointLight.Index.Arr.UB"
		);

		GlobalTable_ = dxContext_.GlobalDescriptorHeap().Allocate(5U);
		Lumina::D3D12::SRV<PointLight>::Create(
			device,
			GlobalTable_.CPUHandle(GlobalTableEntry::SRV_Array_PointLight),
			UB_Arr_PointLight_
		);
		Lumina::D3D12::SRV<Mat4>::Create(
			device,
			GlobalTable_.CPUHandle(GlobalTableEntry::SRV_Array_WorldMatrix_LightSphere),
			UB_Arr_WorldMatrix_LightSphere_
		);
		Lumina::D3D12::SRV<U32>::Create(
			device,
			GlobalTable_.CPUHandle(GlobalTableEntry::SRV_Array_Index_ActivePointLight),
			UB_Arr_Index_ActivePointLight_
		);

		std::vector<F32x4> arr_Vert_LightSphere;
		std::vector<U32> arr_Idx_LightSphere;
		CreateSphere<12U, 12U>(arr_Vert_LightSphere, arr_Idx_LightSphere, 1.0f);
		UB_Vertices_LightSphere_.Initialize(
			device,
			sizeof(F32x4) * arr_Vert_LightSphere.size()
		);
		UB_Vertices_LightSphere_.Store(
			arr_Vert_LightSphere.data(),
			sizeof(F32x4) * arr_Vert_LightSphere.size(),
			0LLU
		);
		UB_Indices_LightSphere_.Initialize(
			device,
			sizeof(U32) * arr_Idx_LightSphere.size()
		);
		UB_Indices_LightSphere_.Store(
			arr_Idx_LightSphere.data(),
			sizeof(U32) * arr_Idx_LightSphere.size(),
			0LLU
		);
		VBV_LightSphere_ = Lumina::D3D12::VBV::Create<F32x4>(UB_Vertices_LightSphere_);
		IBV_LightSphere_ = Lumina::D3D12::IBV::Create(UB_Indices_LightSphere_);
		Num_IndicesPerSphere_ = static_cast<uint32_t>(arr_Idx_LightSphere.size());
	}
}