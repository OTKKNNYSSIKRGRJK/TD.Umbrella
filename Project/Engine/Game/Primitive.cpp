module;

#include"d3d12.h"

module Lumina.Primitive;

import Lumina.D3D12;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;
import nlohmann.json;

namespace Lumina {
	class LineManager {
	public:
		void Initialize(
			D3D12::GraphicsDevice const& device_,
			nlohmann::json const& config_,
			D3D12::RootSignature const& rs_,
			D3D12::Shader const& vs_,
			D3D12::Shader const& ps_,
			bool isAdditive_,
			bool depthEnabled_
		) {
			PSO_.Initialize(
				device_,
				rs_,
				vs_,
				ps_,
				(isAdditive_) ?
				(D3D12::LoadBlendState(config_.at("Line.PSO.2"))) :
				(D3D12::LoadBlendState(config_.at("Line.PSO"))),
				D3D12::LoadRasterizerState(config_.at("Line.PSO")),
				(depthEnabled_) ?
				(D3D12::LoadDepthStencilState(config_.at("Line.PSO"))) :
				(D3D12::LoadDepthStencilState(config_.at("Line.PSO.2"))),
				D3D12::LoadInputLayout(config_.at("Line.PSO")),
				D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
				{ DXGI_FORMAT_R8G8B8A8_UNORM, },
				D3D12::GraphicsPSO::DefaultDSVFormat
			);
			PSO_SRGB_.Initialize(
				device_,
				rs_,
				vs_,
				ps_,
				(isAdditive_) ?
				(D3D12::LoadBlendState(config_.at("Line.PSO.2"))) :
				(D3D12::LoadBlendState(config_.at("Line.PSO"))),
				D3D12::LoadRasterizerState(config_.at("Line.PSO")),
				(depthEnabled_) ?
				(D3D12::LoadDepthStencilState(config_.at("Line.PSO"))) :
				(D3D12::LoadDepthStencilState(config_.at("Line.PSO.2"))),
				D3D12::LoadInputLayout(config_.at("Line.PSO")),
				D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
				{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, },
				D3D12::GraphicsPSO::DefaultDSVFormat
			);

			//DB_Vertices_.Initialize(device_, sizeof(PrimitiveVertex) * (MaxNum_ * 2U));
			UB_Vertices_.Initialize(device_, sizeof(PrimitiveVertex) * (MaxNum_ * 2U));
			VBV_ = D3D12::VBV::Create<PrimitiveVertex>(UB_Vertices_);
		}

		void Batch(
			PrimitiveVertex const& vert0_,
			PrimitiveVertex const& vert1_
		) {
			UB_Vertices_.Store(&vert0_, sizeof(PrimitiveVertex), sizeof(PrimitiveVertex) * Count_ * 2U);
			UB_Vertices_.Store(&vert1_, sizeof(PrimitiveVertex), sizeof(PrimitiveVertex) * (Count_ * 2U + 1U));
			++Count_;
		}

		void End([[maybe_unused]] D3D12::CommandList const& cmdList_) {}

		void Render(D3D12::CommandList const& cmdList_, int32_t flag_SRGB_) {
			cmdList_->SetPipelineState((flag_SRGB_) ? (PSO_SRGB_.Get()) : (PSO_.Get()));
			cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
			cmdList_->IASetVertexBuffers(0U, 1U, &VBV_);
			cmdList_->DrawInstanced(Count_ * 2U, 1U, 0U, 0U);

			Count_ = 0U;
		}

		void RenderBatched(D3D12::CommandList const& cmdList_, int32_t flag_SRGB_) {
			cmdList_->SetPipelineState((flag_SRGB_) ? (PSO_SRGB_.Get()) : (PSO_.Get()));
			cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
			cmdList_->IASetVertexBuffers(0U, 1U, &VBV_);
			cmdList_->DrawInstanced(Count_ * 2U, 1U, 0U, 0U);

			Count_ = 0U;
		}

	private:
		D3D12::DefaultBuffer DB_Vertices_{};
		D3D12::UploadBuffer UB_Vertices_{};
		D3D12::VBV VBV_{};

		D3D12::GraphicsPSO PSO_{};
		D3D12::GraphicsPSO PSO_SRGB_{};

		uint32_t Count_{ 0U };

		static constinit inline uint32_t const MaxNum_{ 128U };
	};

	class TriangleManager {
	public:
		void Initialize(
			D3D12::GraphicsDevice const& device_,
			nlohmann::json const& config_,
			D3D12::RootSignature const& rs_,
			D3D12::Shader const& vs_,
			D3D12::Shader const& ps_,
			bool isAdditive_,
			bool depthEnabled_
		) {
			PSO_.Initialize(
				device_,
				rs_,
				vs_,
				ps_,
				(isAdditive_) ?
				(D3D12::LoadBlendState(config_.at("Triangle.PSO.2"))) :
				(D3D12::LoadBlendState(config_.at("Triangle.PSO"))),
				D3D12::LoadRasterizerState(config_.at("Triangle.PSO")),
				(depthEnabled_) ?
				(D3D12::LoadDepthStencilState(config_.at("Triangle.PSO"))) :
				(D3D12::LoadDepthStencilState(config_.at("Triangle.PSO.2"))),
				D3D12::LoadInputLayout(config_.at("Triangle.PSO")),
				D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				{ DXGI_FORMAT_R8G8B8A8_UNORM, },
				D3D12::GraphicsPSO::DefaultDSVFormat
			);
			PSO_SRGB_.Initialize(
				device_,
				rs_,
				vs_,
				ps_,
				(isAdditive_) ?
				(D3D12::LoadBlendState(config_.at("Triangle.PSO.2"))) :
				(D3D12::LoadBlendState(config_.at("Triangle.PSO"))),
				D3D12::LoadRasterizerState(config_.at("Triangle.PSO")),
				(depthEnabled_) ?
				(D3D12::LoadDepthStencilState(config_.at("Triangle.PSO"))) :
				(D3D12::LoadDepthStencilState(config_.at("Triangle.PSO.2"))),
				D3D12::LoadInputLayout(config_.at("Triangle.PSO")),
				D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, },
				D3D12::GraphicsPSO::DefaultDSVFormat
			);

			//DB_Vertices_.Initialize(device_, sizeof(PrimitiveVertex) * (MaxNum_ * 3U));
			UB_Vertices_.Initialize(device_, sizeof(PrimitiveVertex) * (MaxNum_ * 3U));
			VBV_ = D3D12::VBV::Create<PrimitiveVertex>(UB_Vertices_);
		}

		void Batch(
			PrimitiveVertex const& vert0_,
			PrimitiveVertex const& vert1_,
			PrimitiveVertex const& vert2_
		) {
			UB_Vertices_.Store(&vert0_, sizeof(PrimitiveVertex), sizeof(PrimitiveVertex) * Count_ * 3U);
			UB_Vertices_.Store(&vert1_, sizeof(PrimitiveVertex), sizeof(PrimitiveVertex) * (Count_ * 3U + 1U));
			UB_Vertices_.Store(&vert2_, sizeof(PrimitiveVertex), sizeof(PrimitiveVertex) * (Count_ * 3U + 2U));
			++Count_;
		}

		void End([[maybe_unused]] D3D12::CommandList const& cmdList_) {}

		void Render(D3D12::CommandList const& cmdList_, I32 flag_SRGB_) {
			cmdList_->SetPipelineState((flag_SRGB_) ? (PSO_SRGB_.Get()) : (PSO_.Get()));
			cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList_->IASetVertexBuffers(0U, 1U, &VBV_);
			cmdList_->DrawInstanced(Count_ * 3U, 1U, 0U, 0U);

			Count_ = 0U;
		}

		void RenderBatched(D3D12::CommandList const& cmdList_, I32 flag_SRGB_) {
			cmdList_->SetPipelineState((flag_SRGB_) ? (PSO_SRGB_.Get()) : (PSO_.Get()));
			cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList_->IASetVertexBuffers(0U, 1U, &VBV_);
			cmdList_->DrawInstanced(Count_ * 3U, 1U, 0U, 0U);

			Count_ = 0U;
		}

	private:
		D3D12::DefaultBuffer DB_Vertices_{};
		D3D12::UploadBuffer UB_Vertices_{};
		D3D12::VBV VBV_{};

		D3D12::GraphicsPSO PSO_{};
		D3D12::GraphicsPSO PSO_SRGB_{};

		uint32_t Count_{ 0U };

		static constinit inline uint32_t const MaxNum_{ 128U };
	};
}

namespace Lumina {
	void PrimitiveManager::Initialize(
		D3D12::Context const& d3d12Context_,
		WStringView filePath_VS_,
		WStringView filePath_PS_,
		bool isAdditive_,
		bool depthEnabled_
	) {
		auto config{ Utils::LoadFromFile<nlohmann::json>("Engine/Assets/Configs/Primitive.json") };
		auto&& rsSetup{ D3D12::LoadSetup<D3D12::RootSignature>(config.at("CommonRS")) };
		RS_.Initialize(d3d12Context_.Device(), rsSetup);
		d3d12Context_.Compile(
			VS_,
			filePath_VS_.Data(),
			L"vs_6_6",
			L"main",
			"Primitive.VS"
		);
		d3d12Context_.Compile(
			PS_,
			filePath_PS_.Data(),
			L"ps_6_6",
			L"main",
			"Primitive.PS"
		);
		
		LineManager_ = std::make_unique<LineManager>();
		LineManager_->Initialize(d3d12Context_.Device(), config, RS_, VS_, PS_, isAdditive_, depthEnabled_);

		TriangleManager_ = std::make_unique<TriangleManager>();
		TriangleManager_->Initialize(d3d12Context_.Device(), config, RS_, VS_, PS_, isAdditive_, depthEnabled_);

		UB_VP_.Initialize(d3d12Context_.Device(), (sizeof(Math::F32x4x4<>) + 0xFFU) & ~0xFFU);
	}

	void PrimitiveManager::Begin(
		D3D12::CommandList const& cmdList_
	) {
		cmdList_->SetGraphicsRootSignature(RS_.Get());
	}

	void PrimitiveManager::End(
		D3D12::CommandList const& cmdList_,
		D3D12::DescriptorTable const& texTable_,
		Math::F32x4x4<> const& vp_,
		int32_t flag_SRGB_
	) {
		UB_VP_.Store(&vp_, sizeof(Math::F32x4x4<>), 0LLU);
		cmdList_->SetGraphicsRootConstantBufferView(0U, UB_VP_->GetGPUVirtualAddress());
		cmdList_->SetGraphicsRootDescriptorTable(1U, texTable_.GPUHandle(0U));

		LineManager_->RenderBatched(cmdList_, flag_SRGB_);
		TriangleManager_->RenderBatched(cmdList_, flag_SRGB_);
	}

	void PrimitiveManager::End(
		D3D12::CommandList const& cmdList_
	) {
		LineManager_->End(cmdList_);
		TriangleManager_->End(cmdList_);
	}

	void PrimitiveManager::Render(
		D3D12::CommandList const& cmdList_,
		D3D12::DescriptorTable const& texTable_,
		Math::F32x4x4<> const& vp_,
		int32_t flag_SRGB_,
		D3D12_GPU_DESCRIPTOR_HANDLE cbv_
	) {
		UB_VP_.Store(&vp_, sizeof(Math::F32x4x4<>), 0LLU);
		cmdList_->SetGraphicsRootConstantBufferView(0U, UB_VP_->GetGPUVirtualAddress());
		cmdList_->SetGraphicsRootDescriptorTable(1U, texTable_.GPUHandle(0U));
		if (cbv_.ptr != 0LLU) {
			cmdList_->SetGraphicsRootDescriptorTable(2U, cbv_);
		}

		LineManager_->Render(cmdList_, flag_SRGB_);
		TriangleManager_->Render(cmdList_, flag_SRGB_);
	}

	void PrimitiveManager::BatchLine(
		PrimitiveVertex const& vert0_,
		PrimitiveVertex const& vert1_
	) {
		LineManager_->Batch(vert0_, vert1_);
	}

	void PrimitiveManager::BatchTriangle(
		PrimitiveVertex const& vert0_,
		PrimitiveVertex const& vert1_,
		PrimitiveVertex const& vert2_
	) {
		TriangleManager_->Batch(vert0_, vert1_, vert2_);
	}

	PrimitiveManager::PrimitiveManager() {}
	PrimitiveManager::~PrimitiveManager() {}
}