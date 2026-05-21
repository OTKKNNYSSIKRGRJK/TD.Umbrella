export module Lumina.Grassland;

import <memory>;

import <vector>;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import Lumina.Main;

import Lumina.Utils.Data;
import Lumina.Utils.Data.Mesh;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import Lumina.Core.Common;
import Lumina.Core.Math;

namespace Lumina {
	export class Grassland {
		struct Constant_System {
			F32 Time;
		};
		struct Constant_Scene {
			Math::F32x4x4<> WorldToProjective;
			Math::F32x3 PlayerWorldPos;
			F32 PlayerRadius;
			Math::F32x3 EnemyWorldPos;
			F32 EnemyRadius;
		};

	public:
		void Initialize(
			D3D12::Context const& dxContext_,
			U32 mapWidth_,
			U32 mapHeight_
		);

		void Render(
			D3D12::Context const& dxContext_,
			D3D12::CommandList const& cmdList_,
			Math::F32x4x4<> const& worldToProj_,
			Math::F32x3 const& playerWorldPos_,
			F32 playerRadius_,
			Math::F32x3 const& enemyWorldPos_,
			F32 enemyRadius_
		);

	private:
		U32 MapWidth_;
		U32 MapHeight_;

		std::vector<std::unique_ptr<D3D12::ComputeTexture2D>> Arr_Maps_{};
		std::vector<DXGI_FORMAT> Arr_MapResourceFormats_{};
		U32 Num_Maps_{};

		D3D12::DescriptorTable GlobalTable_Graphics_{};
		D3D12::DescriptorTable GlobalTable_ImageTextures_{};
		D3D12::DescriptorTable GlobalTable_Compute_{};
		D3D12::DescriptorHeap LocalHeap_CBV_{};
		D3D12::DescriptorHeap LocalHeap_SRV_Maps_{};
		D3D12::DescriptorHeap LocalHeap_UAV_Maps_{};

		D3D12::Shader VertexShader_{};
		D3D12::Shader PixelShader_{};
		std::vector<std::unique_ptr<D3D12::Shader>> Arr_ComputeShaders_{};
		U32 Num_ComputeShaders_{};

		D3D12::GraphicsPipelineState GraphicsPSO_{};
		std::vector<std::unique_ptr<D3D12::ComputePipelineState>> Arr_ComputePSOs_{};
		U32 Num_ComputePSOs_{};

		D3D12::CommandAllocator DirectCommandAllocator0_{};
		D3D12::CommandList DirectCommandList0_{};
		D3D12::CommandAllocator DirectCommandAllocator1_{};
		D3D12::CommandList DirectCommandList1_{};
		D3D12::CommandAllocator ComputeCommandAllocator_{};
		D3D12::CommandList ComputeCommandList_{};

		D3D12::RootSignature GraphicsRS_{};
		D3D12::RootSignature ComputeRS_{};

		Constant_System Constant_System_{};
		D3D12::DefaultBuffer DB_Constant_System_{};
		D3D12::UploadBuffer UB_Constant_System_{};

		Constant_Scene Constant_Scene_{};
		D3D12::DefaultBuffer DB_Constant_Scene_{};
		D3D12::UploadBuffer UB_Constant_Scene_{};

		D3D12::DefaultBuffer DB_Mesh_GrassBlade_{};
		D3D12::VBV VBV_Mesh_GrassBlade_{};
		U32 Num_Vertices_GrassBlade_{};

	private:
		enum class MAP : U32 {
			BEND = 0U,
			TRAMPLING = 1U,
			NOISE = 2U,
			TINT = 3U,
		};

		enum class COMPUTE_SHADER : U32 {
			BEND = 0U,
			TRAMPLING = 1U,
			NOISE = 2U,
			TINT = 3U,
			INIT = 4U,
		};
	};
}