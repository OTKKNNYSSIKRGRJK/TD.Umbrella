export module Game.Terrain : Render.Impl;

import <array>;
import <vector>;
import <memory>;
import <format>;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Core.String;
import Lumina.D3D12;
import Lumina.D3D12.Aux.View;
import Lumina.Main;
import : Shape;
import Lumina.ProceduralMap;

namespace Game::Impl {
	export class TerrainRenderer {
	public:
		auto PrepareMesh(
			TerrainShapeCollection const& shapeCollection_
		) -> void;

	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Update(_ARGs&&...args_) -> void;
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Render(_ARGs&&...args_) -> void;

	public:
		auto Update() -> void;
		template<typename..._ARGs>
		auto Render(_ARGs&&...args_) -> void;

	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Initialize(_ARGs&&...args_) -> void;

	public:
		auto Initialize() -> void;

		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//
		//##++	Low Poly												++##//
		//++##	++##++	##++##	++##++	##++##	++##++	##++##	++##++	##++//

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Procedural Generation									--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		
	private:
		Lumina::ProceduralMap Surface_MaterialID_;
		Lumina::ProceduralMap Surface_BlendAndElevation_;
		Lumina::ProceduralMap Surface_Normal_;

		Lumina::D3D12::RootSignature RS_Surface_{};
		Lumina::D3D12::ComputePSO PSO_Surface_{};
		Lumina::D3D12::ComputePSO PSO_Surface2_{};
		Lumina::D3D12::Shader CS_Surface_{};
		Lumina::D3D12::Shader CS_Surface2_{};

		Lumina::D3D12::UploadBuffer UB_Parameter_Common_{};
		Lumina::D3D12::UploadBuffer UB_Parameter_Material_{};
		Lumina::D3D12::UploadBuffer UB_Parameter_Blend_{};
		Lumina::D3D12::UploadBuffer UB_Parameter_Height_{};
		Lumina::D3D12::DescriptorTable GlobalTable_Surface_{};

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Canvas, Render Pass										--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	private:
		Lumina::D3D12::Canvas Canvas_LowPoly_;
		Lumina::D3D12::RenderPass RenderPass_LowPoly_;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Pipeline												--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	private:
		Lumina::D3D12::RootSignature RS_LowPoly_;
		Lumina::D3D12::Shader VS_LowPoly_;
		Lumina::D3D12::Shader HS_LowPoly_;
		Lumina::D3D12::Shader DS_LowPoly_;
		Lumina::D3D12::Shader PS_LowPoly_;
		Lumina::D3D12::GraphicsPSO PSO_LowPoly_;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Resources & Views for Vertex Shader						--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	private:
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_VertexElementArray_;

		std::vector<Lumina::MeshShaderAsset> MeshShaderAssets_;

		Lumina::D3D12::VertexBufferView VBVs_LowPoly_[2];

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Resources & Views for Hull Shader						--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	private:
		struct HSParameters {
			Lumina::Math::F32x3 WorldPosition_Camera;
			// * Distance at which `Tessellation_MAX` is applied
			Lumina::F32 Distance_MIN;
			// * Distance at which `Tessellation_MIN` is applied
			Lumina::F32 Distance_MAX;
			Lumina::F32 Tessellation_MIN;
			Lumina::F32 Tessellation_MAX;
		};

	private:
		Lumina::D3D12::DescriptorTable GlobalTable_CBV_HSParameters_;
		Lumina::D3D12::UploadBuffer UB_HSParameters_;
		HSParameters HSParameters_;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Resources & Views for Domain Shader						--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//

	private:
		struct DSParameters {
			Lumina::F32 WorldToProjective[4][4];
			Lumina::F32 Scale_SurfaceElevation;
			Lumina::F32 Scale_MaterialElevation;
		};

		Lumina::D3D12::DescriptorTable GlobalTable_CBV_DSParameters_;
		Lumina::D3D12::UploadBuffer UB_DSParameters_;
		DSParameters DSParameters_;

		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		//==--	Resources & Views for Pixel Shader						--==//
		//--==	--==--	==--==	--==--	==--==	--==--	==--==	--==--	==--//
		
	private:
		struct TerrainMaterial {
			Lumina::U32 ID_Albedo;
			Lumina::U32 ID_Normal;
			Lumina::U32 ID_Height;
		};

		struct PSParameters {
			Lumina::F32x2 Scale_SurfaceBlendUV;
			Lumina::F32 Scale_SurfaceNormal;
			Lumina::F32 Scale_MaterialNormal;
		};

	private:
		Lumina::D3D12::DescriptorTable GlobalTable_CBV_PSParameters_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_TerrainMaterialMap_Albedo_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_TerrainMaterialMap_Normal_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_TerrainMaterialMap_Height_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_TerrainMaterialDatabase_;

		//Lumina::D3D12::DescriptorTable GlobalTable_SRV_Noise_;

		constexpr static Lumina::U32 Capacity_TerrainMaterialDatabase_{ 32U };
		Lumina::D3D12::UploadBuffer UB_TerrainMaterialDatabase_;
		std::array<TerrainMaterial, Capacity_TerrainMaterialDatabase_> TerrainMaterialDatabase_;
		Lumina::D3D12::UploadBuffer UB_PSParameters_;
		PSParameters PSParameters_;

		Lumina::D3D12::ComputeTexture2D CT_HeightMap_LowPoly_;
		Lumina::D3D12::DescriptorTable GlobalTable_UAV_SRV_HeightMap_LowPoly_;

	private:
		Lumina::D3D12::UploadBuffer VertexBuffer_XY_;
		Lumina::D3D12::UploadBuffer VertexBuffer_Z_;
		Lumina::D3D12::VertexBufferView VBV_;

		constexpr static Lumina::U32 MaxNum_Vertices_{ 1024U };
	};
}