module Lumina.Watercolor;

import <vector>;

import Lumina.D3D12;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;
import Lumina.D3D12.Context;
import Lumina.ResourceManager;
import Lumina.Main;
import Lumina.Utils.Data;
import Lumina.Core.Common;

namespace Lumina {
	template<>
	auto Watercolor::Render_<"">() -> void {

	}

	template<>
	auto Watercolor::Initialize_<"Resource">() -> void {
		auto const& context{ Lumina::Context::Instance() };
		D3D12::Context const& d3d12Context{ context.D3D12Context() };
		D3D12::GraphicsDevice const& d3d12Device{ d3d12Context.Device() };

		CT_Wetness_[0].Initialize(d3d12Device, 1280U, 720U);
		CT_Wetness_[1].Initialize(d3d12Device, 1280U, 720U);
		CT_Pigment_[0].Initialize(d3d12Device, 1280U, 720U);
		CT_Pigment_[1].Initialize(d3d12Device, 1280U, 720U);

		CT_Edge_.Initialize(d3d12Device, 1280U, 720U);
		CT_EdgeDensity_.Initialize(d3d12Device, 1280U, 720U);
		CT_BlurH_.Initialize(d3d12Device, 1280U, 720U);
		CT_BlurV_.Initialize(d3d12Device, 1280U, 720U);
		CT_Noise_.Initialize(d3d12Device, 128U, 128U);
		CT_Composite_.Initialize(d3d12Device, 1280U, 720U);

		UB_Constants_.Initialize(d3d12Device, 1024U);
	}

	template<>
	auto Watercolor::Initialize_<"Descriptor">() -> void {
		auto const& context{ Lumina::Context::Instance() };
		D3D12::Context const& d3d12Context{ context.D3D12Context() };
		D3D12::GraphicsDevice const& d3d12Device{ d3d12Context.Device() };

		/*LocalHeap_RTV_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1U, false);
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{
				.Format{ RT_Watercolor_.Format() },
				.ViewDimension{ D3D12_RTV_DIMENSION_TEXTURE2D },
				.Texture2D{
					.MipSlice{ 0U },
					.PlaneSlice{ 0U },
				},
			};
			d3d12Device->CreateRenderTargetView(
				RT_Watercolor_.Get(),
				&rtvDesc,
				LocalHeap_RTV_.CPUHandle(0U)
			);
		}*/


		GlobalTable_CBVSRVUAV_ = d3d12Context.GlobalDescriptorHeap().Allocate(
			static_cast<uint32_t>(VIEW_NAME::COUNT)
		);

		D3D12::CBV::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::CBV_CONSTANTS),
			UB_Constants_
		);

		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_WETNESS_0),
			CT_Wetness_[0]
		);
		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_PIGMENT_0),
			CT_Pigment_[0]
		);
		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_WETNESS_1),
			CT_Wetness_[1]
		);
		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_PIGMENT_1),
			CT_Pigment_[1]
		);

		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_WETNESS_1),
			CT_Wetness_[1]
		);
		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_PIGMENT_1),
			CT_Pigment_[1]
		);
		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_WETNESS_0),
			CT_Wetness_[0]
		);
		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_PIGMENT_0),
			CT_Pigment_[0]
		);

		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_EDGE),
			CT_Edge_
		);
		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_EDGEDENSITY),
			CT_EdgeDensity_
		);
		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_BLURH),
			CT_BlurH_
		);
		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_BLURV),
			CT_BlurV_
		);
		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_NOISE),
			CT_Noise_
		);
		D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_COMPOSITE),
			CT_Composite_
		);

		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_EDGE),
			CT_Edge_
		);
		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_EDGEDENSITY),
			CT_EdgeDensity_
		);
		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_BLURH),
			CT_BlurH_
		);
		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_BLURV),
			CT_BlurV_
		);
		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_NOISE),
			CT_Noise_
		);
		D3D12::UAV<void>::Create(
			d3d12Device,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::UAV_COMPOSITE),
			CT_Composite_
		);
	}

	template<>
	auto Watercolor::Initialize_<"Image Texture">() -> void {
		auto const& context{ Lumina::Context::Instance() };
		D3D12::Context const& d3d12Context{ context.D3D12Context() };
		D3D12::GraphicsDevice const& d3d12Device{ d3d12Context.Device() };
		ResourceManager const& resContext{ context.ResourceContext() };

		std::vector<uint32_t> texIDs{};
		std::vector<std::pair<std::string, std::string>> texturesToLoad = {
			{ "SubstrateAlbedo", "Assets/Watercolor/SubstrateAlbedo.png" },
			{ "SubstrateNormal", "Assets/Watercolor/SubstrateNormal.png" },
		};
		resContext.Graphics().LoadImageTextures(
			texIDs,
			texturesToLoad
		);
		d3d12Device->CopyDescriptorsSimple(
			1U,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_PAPER_TEXTURE_ALBEDO),
			resContext.Graphics().CPUHandle(texIDs.at(0U)),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
		d3d12Device->CopyDescriptorsSimple(
			1U,
			GlobalTable_CBVSRVUAV_.CPUHandle(VIEW_NAME::SRV_PAPER_TEXTURE_NORMAL),
			resContext.Graphics().CPUHandle(texIDs.at(1U)),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
	}

	template<>
	auto Watercolor::Initialize_<"Pipeline">() -> void {
		auto const& context{ Lumina::Context::Instance() };
		D3D12::Context const& d3d12Context{ context.D3D12Context() };
		D3D12::GraphicsDevice const& d3d12Device{ d3d12Context.Device() };

		auto settings{ Utils::LoadFromFile<nlohmann::json>("Watercolor.json", "Assets/Configs") };
		auto computeRSSetup{ D3D12::LoadRootSignatureSetup(settings.at("ComputeRS")) };
		RS_ComputeCommon_.Initialize(d3d12Device, computeRSSetup, "ComputeRS");
		
		d3d12Context.Compile(
			CS_Noise_,
			L"Assets/Shaders/Watercolor/Noise.CS.hlsl",
			L"cs_6_6",
			L"main",
			"Noise"
		);
		
		d3d12Context.Compile(
			CS_EdgeDetection_,
			L"Assets/Shaders/Watercolor/Edge.CS.hlsl",
			L"cs_6_6",
			L"Detect",
			"EdgeDetect"
		);
		d3d12Context.Compile(
			CS_EdgeDensity_,
			L"Assets/Shaders/Watercolor/Edge.CS.hlsl",
			L"cs_6_6",
			L"CalculateDensity",
			"EdgeDensity"
		);
		d3d12Context.Compile(
			CS_BlurHorizontal_,
			L"Assets/Shaders/Watercolor/Blur.CS.hlsl",
			L"cs_6_6",
			L"Convolve_Horizontal",
			"BlurH"
		);
		d3d12Context.Compile(
			CS_BlurVertical_,
			L"Assets/Shaders/Watercolor/Blur.CS.hlsl",
			L"cs_6_6",
			L"Convolve_Vertical",
			"BlurV"
		);
		d3d12Context.Compile(
			CS_Pigment_,
			L"Assets/Shaders/Watercolor/Pigment.CS.hlsl",
			L"cs_6_6",
			L"main",
			"Pigment"
		);
		d3d12Context.Compile(
			CS_Composite_,
			L"Assets/Shaders/Watercolor/Composite.CS.hlsl",
			L"cs_6_6",
			L"main",
			"Composite"
		);

		PSO_Noise_.Initialize(
			d3d12Device,
			RS_ComputeCommon_,
			CS_Noise_,
			"Noise"
		);
		PSO_EdgeDetection_.Initialize(
			d3d12Device,
			RS_ComputeCommon_,
			CS_EdgeDetection_,
			"EdgeDetect"
		);
		PSO_EdgeDensity_.Initialize(
			d3d12Device,
			RS_ComputeCommon_,
			CS_EdgeDensity_,
			"EdgeDensity"
		);
		PSO_BlurHorizontal_.Initialize(
			d3d12Device,
			RS_ComputeCommon_,
			CS_BlurHorizontal_,
			"BlurH"
		);
		PSO_BlurVertical_.Initialize(
			d3d12Device,
			RS_ComputeCommon_,
			CS_BlurVertical_,
			"BlurV"
		);
		PSO_Pigment_.Initialize(
			d3d12Device,
			RS_ComputeCommon_,
			CS_Pigment_,
			"Pigment"
		);
		PSO_Composite_.Initialize(
			d3d12Device,
			RS_ComputeCommon_,
			CS_Composite_,
			"Composite"
		);
	}

	auto Watercolor::Initialize() -> void {
		Initialize_<"Resource">();
		Initialize_<"Descriptor">();
		Initialize_<"Image Texture">();
		Initialize_<"Pipeline">();

		Constants_.TexelSize = { 1.0f / 1280.0f, 1.0f / 720.0f };
		Constants_.UVStep = { 1.0f / 1280.0f, 1.0f / 720.0f };
		Constants_.Weight_Luminance = 4.0f;
		Constants_.Weight_Depth = 3.0f;
		Constants_.Time = 0.0f;

		std::vector<D3D12_RESOURCE_BARRIER> const barriers{
			D3D12::Barrier::Transition(
				CT_Noise_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_Edge_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_EdgeDensity_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_BlurH_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_BlurV_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_Pigment_[0],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_Composite_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
		};
		auto const& context{ Lumina::Context::Instance() };
		D3D12::CommandList const& cmdList{ context.MainCommandList() };
		cmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
	}

	namespace {
		constexpr uint32_t DispatchX{ 1280U / 8U };
		constexpr uint32_t DispatchY{ 720U / 8U };
	}

	auto Watercolor::Update(float deltaTime_) -> void {
		Constants_.Time += deltaTime_;
		UB_Constants_.Store(&Constants_, sizeof(Constants), 0LLU);
	}

	template<>
	auto Watercolor::Render_<"SetComputeRootDescriptorTable">(
		D3D12::CommandList const& cmdList_,
		D3D12::DescriptorTable const& gBuffers_
	) -> void {
		cmdList_->SetComputeRootDescriptorTable(
			0U,
			GlobalTable_CBVSRVUAV_.GPUHandle(VIEW_NAME::CBV_CONSTANTS)
		);
		cmdList_->SetComputeRootDescriptorTable(
			5U,
			gBuffers_.GPUHandle(0U)
		);
		cmdList_->SetComputeRootDescriptorTable(
			1U,
			GlobalTable_CBVSRVUAV_.GPUHandle(16U + IDX_PingPong_Wetness_ * 2U)
		);
		cmdList_->SetComputeRootDescriptorTable(
			2U,
			GlobalTable_CBVSRVUAV_.GPUHandle(20U + IDX_PingPong_Pigment_ * 2U)
		);
		cmdList_->SetComputeRootDescriptorTable(
			3U,
			GlobalTable_CBVSRVUAV_.GPUHandle(VIEW_NAME::SRV_PAPER_TEXTURE_ALBEDO)
		);
		cmdList_->SetComputeRootDescriptorTable(
			4U,
			GlobalTable_CBVSRVUAV_.GPUHandle(VIEW_NAME::UAV_EDGE)
		);
	}

	auto Watercolor::Render(
		D3D12::DescriptorTable const& gBuffers_
	) -> void {
		auto const& context{ Lumina::Context::Instance() };
		D3D12::Context const& d3d12Context{ context.D3D12Context() };
		D3D12::CommandList const& cmdList{ context.MainCommandList() };

		ID3D12DescriptorHeap* descriptorHeaps[]{
			d3d12Context.GlobalDescriptorHeap().Get(),
		};
		cmdList->SetDescriptorHeaps(1U, descriptorHeaps);

		cmdList->SetComputeRootSignature(RS_ComputeCommon_.Get());

		Render_<"SetComputeRootDescriptorTable">(cmdList, gBuffers_);
		
		std::vector<D3D12_RESOURCE_BARRIER> const uavBarriers{
			{
				.Type{ D3D12_RESOURCE_BARRIER_TYPE_UAV },
				.UAV{ .pResource{ CT_Noise_.Get() }, },
			},
			{
				.Type{ D3D12_RESOURCE_BARRIER_TYPE_UAV },
				.UAV{.pResource{ CT_Pigment_[0].Get() }, },
			},
		};

		std::vector<D3D12_RESOURCE_BARRIER> const barriers{
			D3D12::Barrier::Transition(
				CT_Noise_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_Edge_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_EdgeDensity_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_BlurH_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_BlurV_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_Pigment_[0],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				CT_Composite_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
		};

		std::vector<D3D12_RESOURCE_BARRIER> const barriers2{
			D3D12::Barrier::Transition(
				CT_Noise_,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			),
			D3D12::Barrier::Transition(
				CT_Edge_,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			),
			D3D12::Barrier::Transition(
				CT_EdgeDensity_,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			),
			D3D12::Barrier::Transition(
				CT_BlurH_,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			),
			D3D12::Barrier::Transition(
				CT_BlurV_,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			),
			D3D12::Barrier::Transition(
				CT_Pigment_[0],
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			),
			D3D12::Barrier::Transition(
				CT_Composite_,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			),
		};

		cmdList->ResourceBarrier(static_cast<uint32_t>(barriers2.size()), barriers2.data());

		cmdList->SetPipelineState(PSO_Noise_.Get());
		cmdList->Dispatch(128U, 128U, 1U); 
		cmdList->ResourceBarrier(1U, uavBarriers.data() + 0);
		cmdList->ResourceBarrier(1U, barriers.data() + 0);

		cmdList->SetPipelineState(PSO_EdgeDetection_.Get());
		cmdList->Dispatch(DispatchX, DispatchY, 1U);
		cmdList->ResourceBarrier(1U, barriers.data() + 1);
		cmdList->SetPipelineState(PSO_EdgeDensity_.Get());
		cmdList->Dispatch(DispatchX, DispatchY, 1U);
		cmdList->ResourceBarrier(1U, barriers.data() + 2);
		cmdList->SetPipelineState(PSO_BlurHorizontal_.Get());
		cmdList->Dispatch(DispatchX, DispatchY, 1U);
		cmdList->ResourceBarrier(1U, barriers.data() + 3);
		cmdList->SetPipelineState(PSO_BlurVertical_.Get());
		cmdList->Dispatch(DispatchX, DispatchY, 1U);
		cmdList->ResourceBarrier(1U, barriers.data() + 4);
		cmdList->SetPipelineState(PSO_Pigment_.Get());
		cmdList->Dispatch(DispatchX, DispatchY, 1U);
		cmdList->ResourceBarrier(1U, uavBarriers.data() + 1);
		cmdList->ResourceBarrier(1U, barriers.data() + 5);
		cmdList->SetComputeRootDescriptorTable(
			2U,
			GlobalTable_CBVSRVUAV_.GPUHandle(20U + (IDX_PingPong_Pigment_ ^ 1U) * 2U)
		);
		cmdList->SetPipelineState(PSO_Composite_.Get());
		cmdList->Dispatch(DispatchX, DispatchY, 1U);
		cmdList->ResourceBarrier(1U, barriers.data() + 6);
	}

	auto Watercolor::GlobalTable() const noexcept -> D3D12::DescriptorTable const& {
		return GlobalTable_CBVSRVUAV_;
	}
}