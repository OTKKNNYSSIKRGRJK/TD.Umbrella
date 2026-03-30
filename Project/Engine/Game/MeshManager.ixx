export module Lumina.MeshManager;

import <cstdint>;

import <memory>;

import <vector>;

import <string>;

import <d3d12.h>;

import nlohmann.json;

import Lumina.Core.Math;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import Lumina.Utils.Data;
export import Lumina.Utils.Data.Mesh;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace Lumina {
	export class MeshShaderAsset {
		friend class MeshUploader;

	public:
		inline D3D12::DescriptorHeap const& LocalDescriptors() const noexcept { return *LocalHeap_; }
		inline D3D12::DefaultBuffer const& VertexBuffer() const noexcept { return *VertexBuffer_; }
		constexpr uint32_t Num_Vertices() const noexcept { return Num_Vertices_; }

	private:
		std::unique_ptr<D3D12::DefaultBuffer> Positions_{};
		std::unique_ptr<D3D12::DefaultBuffer> TexCoords_{};
		std::unique_ptr<D3D12::DefaultBuffer> Normals_{};
		std::unique_ptr<D3D12::DefaultBuffer> Tangents_{};
		std::unique_ptr<D3D12::DescriptorHeap> LocalHeap_{};

		std::unique_ptr<D3D12::DefaultBuffer> VertexBuffer_{};
		D3D12::VBV VBV_{};
		
		uint32_t Num_Vertices_{};
	};

	export class MeshUploader {
	public:
		void Begin();
		void Batch(Utils::Mesh const& mesh_);
		void End(std::vector<MeshShaderAsset>& assets_);

	public:
		void Initialize(D3D12::Context const& dx12Context_);

	private:
		std::vector<MeshShaderAsset> MeshShaderAssets_{};

		D3D12::Context const* D3D12Context_{ nullptr };
		D3D12::CommandAllocator CommandAllocator_Upload_{};
		D3D12::CommandList CommandList_Upload_{};
		std::vector<std::unique_ptr<D3D12::UploadBuffer>> UploadBuffers_{};
	};
}

namespace Lumina {
	export class MeshManager {
	public:
		enum class BlendMode : uint32_t {
			PremultipliedAlphaBlend,
			StraightAlphaBlend,
			Additive,
			Subtract,
			Multiply,
			Screen,

			Count,
		};

		std::vector<std::string> BlendModeNames_{};

	public:
		D3D12::RootSignature const& RootSignature() const noexcept { return RS_; }

	public:
		void Initialize(
			D3D12::Context const& dx12Context_,
			uint32_t maxNum_Batches_,
			uint32_t maxNum_BatchedVertices_
		);

		void Begin(D3D12::CommandList const& cmdList_);

		void End();

		void BatchBegin();

		void Batch(
			MeshShaderAsset const& mesh_,
			uint32_t num_Instances_,
			D3D12_CPU_DESCRIPTOR_HANDLE localCBV_Material_,
			Math::F32x4x4<> const& world_
		);

		void BatchEnd();

		void Render(
			D3D12::GraphicsPipelineState const& pso_,
			D3D12_GPU_DESCRIPTOR_HANDLE globalSRV_TextureStart_,
			D3D12_CPU_DESCRIPTOR_HANDLE localCBV_VP_
		);

	public:
		MeshManager(){}

		~MeshManager() {
			CommandSignature_->Release();
		}

	private:
		D3D12::RootSignature RS_{};

		D3D12::DescriptorTable Table_Materials_{};
		D3D12::DescriptorTable Table_PositionArrays_{};
		D3D12::DescriptorTable Table_TexCoordArrays_{};
		D3D12::DescriptorTable Table_NormalArrays_{};
		D3D12::DescriptorTable Table_TangentArrays_{};
		D3D12::DescriptorTable Table_VP_And_Worlds_{};
		D3D12::DefaultBuffer DB_Worlds_{};
		D3D12::UploadBuffer UB_Worlds_{};
		D3D12::DefaultBuffer DB_VP_{};
		D3D12::UploadBuffer UB_VP_{};

		D3D12::DefaultBuffer DB_BatchedVertices_{};
		D3D12::VBV VBV_BatchedVertices_{};

		ID3D12CommandSignature* CommandSignature_{ nullptr };
		D3D12::UploadBuffer UB_CommandArgs_{};

		std::vector<MeshShaderAsset const*> BatchedMeshes_{};

		uint32_t Count_UnuploadedBatches_{ 0U };
		uint32_t Count_UploadedBatches_{ 0U };
		uint32_t Count_BatchedVertices_{ 0U };

		D3D12::Context const* D3D12Context_{ nullptr };
		ID3D12GraphicsCommandList* CommandList_{ nullptr };

		uint32_t MaxNum_Batches_{ 2048U };
		uint32_t MaxNum_BatchedVertices_{ 65536U };

		enum class RootSignatureEntry : uint32_t {
			Constant_BatchIndex = 0U,
			Table_VertexData = 1U,
			Table_VP_And_Worlds = 2U,
			Table_Materials = 3U,
			Table_Textures = 4U,
			Table_Lighting = 5U,
		};

		struct CommandArgs {
			uint32_t BatchIndex;
			D3D12_DRAW_ARGUMENTS DrawArgs;
		};
		std::vector<CommandArgs> Array_CommandArgs_{};
	};
}