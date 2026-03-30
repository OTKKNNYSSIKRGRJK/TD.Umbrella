export module Lumina.Sprite;

import <d3d12.h>;

import <cstdint>;
import <type_traits>;
import <algorithm>;

import Lumina.Core.Common;
import Lumina.Core.Math;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import Lumina.Utils.Data;

namespace Lumina {
	export class Sprite {
	public:
		Math::F32x2 Scale{ 1.0f, 1.0f };
		F32 Rotate{ 0.0f };
		Math::F32x2 Translate{ 0.0f, 0.0f };

		Math::F32x2 AnchorPoint{ 0.0f, 0.0f };

		Math::F32x2 UVs[4]{ { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };
		F32 RGBA[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		U32 TextureID;
	};


	export class SpriteRenderer {
		struct Material {
			F32 RGBA[4];
			U32 TextureID;
		};

		struct QuadUVs {
			Math::F32x2 UVs[4];
		};

	public:
		D3D12::RootSignature const& RootSignature() const noexcept { return RS_; }

	public:
		void Initialize(
			D3D12::Context const& d3d12Context_,
			uint32_t maxNum_Batches_
		);

		void Begin(D3D12::CommandList const& cmdList_);

		void End();

		void BatchBegin();

		void Batch(Sprite const& sprite_);

		void BatchEnd();

		void Render(
			D3D12::GraphicsPipelineState const& pso_,
			D3D12_GPU_DESCRIPTOR_HANDLE globalSRV_TextureStart_,
			D3D12_CPU_DESCRIPTOR_HANDLE localCBV_VP_
		);

	public:
		SpriteRenderer() {}

		~SpriteRenderer() {}

	private:
		D3D12::RootSignature RS_{};

		D3D12::DescriptorTable Table_Materials_{};
		D3D12::DescriptorTable Table_UVs_{};
		D3D12::DescriptorTable Table_VP_And_Worlds_{};

		D3D12::DefaultBuffer DB_Materials_;
		D3D12::UploadBuffer UB_Materials_;
		D3D12::DefaultBuffer DB_UVs_;
		D3D12::UploadBuffer UB_UVs_;
		D3D12::DefaultBuffer DB_Worlds_;
		D3D12::UploadBuffer UB_Worlds_;
		D3D12::DefaultBuffer DB_VP_;
		D3D12::UploadBuffer UB_VP_;

		uint32_t Count_UnuploadedBatches_{ 0U };
		uint32_t Count_UploadedBatches_{ 0U };
		uint32_t Count_BatchedVertices_{ 0U };

		D3D12::Context const* D3D12Context_{ nullptr };
		ID3D12GraphicsCommandList* CommandList_{ nullptr };

		uint32_t MaxNum_Batches_{ 2048U };

		D3D12::DefaultBuffer QuadVertexBuffer_{};
		D3D12::DefaultBuffer QuadIndexBuffer_{};
		D3D12::VBV QuadVBV_{};
		D3D12::IBV QuadIBV_{};
	};
}