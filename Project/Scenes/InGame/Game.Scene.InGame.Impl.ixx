export module Game.Scene.InGame : Impl;

import <memory>;

import <vector>;

#if defined(_DEBUG)
import Game.TerrainEditor;
#endif

import Lumina.Core.Common;
import Lumina.Utils.Data;
import Lumina.D3D12;
import Lumina.MeshManager;

import Game.Terrain;
import Lumina.Utils.Camera;
import Lumina.Primitive;

import Game.Player;

namespace Game::Scene::Impl {
	export class InGame {
	private:
		template<typename...ArgTypes>
		void Startup(typename ArgTypes const&...args_);
		template<typename...ArgTypes>
		void InBattle(typename ArgTypes const&...args_);
		template<typename...ArgTypes>
		void Win(typename ArgTypes const&...args_);
		template<typename...ArgTypes>
		void Lose(typename ArgTypes const&...args_);

	private:
		void Render_Geometry();
		void Render_Merge();

	public:
		void Update();
		void Render();

	public:
		template<typename...ArgTypes>
		void Initialize(typename ArgTypes const&...args_);

		InGame();
		virtual ~InGame();

	private:
		struct MeshMaterial {
			Lumina::F32x4 RGBA{ 1.0f, 1.0f, 1.0f, 1.0f };
			Lumina::U32 ID_DiffuseMap;
			Lumina::U32 ID_SpecularMap;
			Lumina::U32 ID_NormalMap;
		};

		std::vector<Lumina::MeshShaderAsset> MeshShaderAssets_;
		Lumina::D3D12::Shader VS_MeshDeferredGeometry_;
		Lumina::D3D12::Shader PS_MeshDeferredGeometry_;
		Lumina::D3D12::GraphicsPSO GraphicsPSO_MeshDeferredGeometry_;

		Lumina::D3D12::Canvas Canvas_;
		Lumina::D3D12::Canvas Canvas_GeometryPass_;

		Lumina::D3D12::RenderPass GeometryPass_;
		Lumina::D3D12::RenderPass MergePass_;

		MeshMaterial Material_;
		std::vector<std::unique_ptr<Lumina::D3D12::UploadBuffer>> UB_Materials_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Materials_;
		Lumina::D3D12::UploadBuffer UB_WorldToHomogeneous_;

		Lumina::D3D12::DescriptorTable GlobalTable_SRV_ImageTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_CanvasTexture_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Scene_;

		std::unique_ptr<Lumina::Utils::Camera> Camera_;
		std::unique_ptr<TerrainEditor> TerrainEditor_;

		std::unique_ptr<TerrainShapeCollection> Terrain_;
		std::unique_ptr<Player> Player_;

		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager_;
	};
}