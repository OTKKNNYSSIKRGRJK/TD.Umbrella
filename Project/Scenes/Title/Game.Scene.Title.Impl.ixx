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
import Lumina.Grassland;
import ParticleSystem;
import Lumina.Skybox;
import Lumina.DeferredLighting;
import Lumina.Sprite;

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
		std::vector<Lumina::MeshShaderAsset> MeshShaderAssets_;
		Lumina::D3D12::Shader VS_MeshDeferredGeometry_;
		Lumina::D3D12::Shader PS_MeshDeferredGeometry_;
		Lumina::D3D12::GraphicsPSO GraphicsPSO_MeshDeferredGeometry_;

		Lumina::Math::F32x3 MeshScale_;
		Lumina::Math::F32x3 MeshRotate_;
		Lumina::Math::F32x3 MeshTranslate_;
		Lumina::D3D12::UploadBuffer VertexBuffer_;
		Lumina::D3D12::UploadBuffer IndexBuffer_;
		Lumina::D3D12::VBV VBV_;
		Lumina::D3D12::IBV IBV_;

		Lumina::Math::F32x3 RootWorldPos_;
		Lumina::Math::F32x3 TipWorldPos_;
		Lumina::Math::F32x3 UmbrellaRotation_;
		std::unique_ptr<Lumina::Math::F32x4x4<>> UmbrellaRootWorld_;
		std::unique_ptr<Lumina::Math::F32x4x4<>> UmbrellaTipWorld_;

	private:
		struct MeshMaterial {
			Lumina::F32x4 RGBA{ 1.0f, 1.0f, 1.0f, 1.0f };
			Lumina::U32 ID_DiffuseMap;
			Lumina::U32 ID_SpecularMap;
			Lumina::U32 ID_NormalMap;
		};

		Lumina::D3D12::Canvas Canvas_GeometryPass_;
		Lumina::D3D12::Canvas Canvas_Merge_;

		Lumina::D3D12::RenderPass GeometryPass_;
		Lumina::D3D12::RenderPass MergePass_;

		MeshMaterial Material0_;
		std::vector<std::unique_ptr<Lumina::D3D12::UploadBuffer>> UB_Materials_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Materials_;
		Lumina::D3D12::UploadBuffer UB_Transforms_;

		Lumina::D3D12::DescriptorTable GlobalTable_SRV_ImageTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_CanvasTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_MergeTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_CBV_Scene_;
		Lumina::D3D12::UploadBuffer UB_WorldToProjective_;
		Lumina::D3D12::UploadBuffer UB_ScreenToWorld_;

		Lumina::D3D12::DescriptorTable GlobalTable_SRV_GBufferForWaterColor_;

		std::unique_ptr<Lumina::Utils::Camera> Camera_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Scene_;
		std::unique_ptr<Lumina::Math::F32x4x4<>> WorldToHomogeneous_;
		std::unique_ptr<Lumina::Math::F32x4x4<>> ScreenToWorld_;

		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager_;
		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager2_;

		Lumina::F32 AnimationTimer_;

		std::unique_ptr<Lumina::Watercolor> Watercolor_;
		std::unique_ptr<Lumina::Grassland> Grassland_;
		std::unique_ptr<Lumina::Skybox> Skybox_;

		// * パーティクル・ライティング
	private:
		std::unique_ptr<Lumina::DeferredLighting> DeferredLighting_;
		Lumina::List<Lumina::PointLight> List_PointLight_;
		Lumina::List<Lumina::Math::F32x4x4<>> List_LocalToWorld_LightSphere_;
		std::vector<Lumina::U32> Arr_Index_ActivePointLight_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_LightingResultTexture_;

		Lumina::D3D12::RootSignature RS_ParticleSystem_;
		Lumina::D3D12::Shader VS_BasicParticle_;
		Lumina::D3D12::Shader PS_BasicParticle_;
		Lumina::D3D12::GraphicsPSO GraphicsPSO_BasicParticle_AdditiveMode_;

		// * パーティクルシェーダー用
	
	private:
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> AmbientSparkles_;
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> Raindrops_;

		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> UmbrellaEffects_;

		// * UI
		
	private:
		std::unique_ptr<Lumina::SpriteRenderer> SpriteRenderer_{ nullptr };
		Lumina::Sprite TitleCaption_;
		Lumina::Sprite UI_StartButton_;
		Lumina::Sprite UI_ExitButton_;
		int SelectedButton_;

		Lumina::D3D12::GraphicsPSO PSO_SpriteUI_{};
		Lumina::D3D12::Shader VS_SpriteUI_{};
		Lumina::D3D12::Shader PS_SpriteUI_{};

		int UITimer_ = 0;
		int UITimer2_ = 0;

		Lumina::D3D12::DescriptorHeap LocalHeap_OrthoProj_;
		Lumina::D3D12::UploadBuffer UB_OrthoProj_;
	};
}