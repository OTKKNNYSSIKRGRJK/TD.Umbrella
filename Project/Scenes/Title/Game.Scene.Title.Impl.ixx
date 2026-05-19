export module Game.Scene.Title : Impl;

import <memory>;
import <vector>;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.D3D12;
import Lumina.D3D12.Aux.View;
import Lumina.MeshManager;
import Lumina.Utils.Camera;
import Lumina.Primitive;
import Lumina.Core.String;

import Lumina.CG3D.Struct;

import Lumina.Watercolor;

namespace Game::Scene::Impl {
	export class Title {
	private:
		void Render_Geometry();
		void Render_Merge();

	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Update_(_ARGs&&...args_) -> void;
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Render_(_ARGs&&...args_) -> void;

	public:
		void Update();
		void Render();

	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Initialize_(_ARGs&&...args_) -> void;

	public:
		void Initialize();

		Title();
		virtual ~Title();

	private:
		Lumina::CG3D::Collection Collection_;
		Lumina::CG3D::MyAnimation Animation_;
		Lumina::CG3D::Skeleton Skeleton_;
		Lumina::CG3D::SkinCluster SkinCluster_;

		Lumina::Math::F32x3 MeshScale_;
		Lumina::Math::F32x3 MeshRotate_;
		Lumina::Math::F32x3 MeshTranslate_;
		Lumina::D3D12::UploadBuffer VertexBuffer_;
		Lumina::D3D12::UploadBuffer IndexBuffer_;
		Lumina::D3D12::VBV VBV_;
		Lumina::D3D12::IBV IBV_;

	private:
		struct MeshMaterial {
			Lumina::F32x4 RGBA{ 1.0f, 1.0f, 1.0f, 1.0f };
			Lumina::U32 ID_DiffuseMap;
			Lumina::U32 ID_SpecularMap;
			Lumina::U32 ID_NormalMap;
		};

		Lumina::D3D12::RootSignature RS_Skinning_;
		Lumina::D3D12::Shader VS_SkinnedMeshDeferredGeometry_;
		Lumina::D3D12::Shader PS_SkinnedMeshDeferredGeometry_;
		Lumina::D3D12::GraphicsPSO GraphicsPSO_SkinnedMeshDeferredGeometry_;

		Lumina::D3D12::Canvas Canvas_;
		Lumina::D3D12::Canvas Canvas_GeometryPass_;

		Lumina::D3D12::RenderPass GeometryPass_;
		Lumina::D3D12::RenderPass MergePass_;

		MeshMaterial Material0_;
		std::vector<std::unique_ptr<Lumina::D3D12::UploadBuffer>> UB_Materials_;
		Lumina::D3D12::DescriptorTable GlobalTable_Materials_;
		Lumina::D3D12::UploadBuffer UB_Transforms_;

		Lumina::D3D12::DescriptorTable GlobalTable_SRV_ImageTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_CanvasTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_CBV_Scene_;

		Lumina::D3D12::DescriptorTable GlobalTable_SRV_GBufferForWaterColor_;

		std::unique_ptr<Lumina::Utils::Camera> Camera_;
		std::unique_ptr<Lumina::Math::F32x4x4<>> WorldToHomogeneous_;

		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager_;

		Lumina::F32 AnimationTimer_;

		std::unique_ptr<Lumina::Watercolor> Watercolor_;
	};
}