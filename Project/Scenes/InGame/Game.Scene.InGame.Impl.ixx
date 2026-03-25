export module Game.Scene.InGame : Impl;

import <memory>;

import <vector>;

import Lumina;
import Lumina.MeshManager;
import Lumina.Utils.Data.Mesh;
import Game.CharacterTest;
import Game.TerrainEditor;

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

	public:
		void Update();
		void Render();

	public:
		template<typename...ArgTypes>
		void Initialize(typename ArgTypes const&...args_);

		InGame();
		virtual ~InGame();

	private:
		std::unique_ptr<Game::CharacterTest> Test_;

	private:
		struct MeshMaterial {
			Lumina::F32 RGBA[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
			Lumina::U32 ID_DiffuseMap;
			Lumina::U32 ID_SpecularMap;
			Lumina::U32 ID_NormalMap;
		};

		MeshMaterial KinokoMaterial_;
		std::vector<Lumina::MeshShaderAsset> MeshShaderAssets_;
		Lumina::D3D12::Shader VS_MeshDeferredGeometry_;
		Lumina::D3D12::Shader PS_MeshDeferredGeometry_;
		Lumina::D3D12::GraphicsPSO GraphicsPSO_MeshDeferredGeometry_;

		Lumina::D3D12::Canvas Canvas_;
		std::unique_ptr<Lumina::D3D12::RenderPass> GeometryPass_;
		Lumina::D3D12::Canvas Canvas_GeometryPass_;

		std::unique_ptr<TerrainEditor> TerrainEditor_;
	};
}