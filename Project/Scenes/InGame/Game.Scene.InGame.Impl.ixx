export module Game.Scene.InGame : Impl;

import <memory>;

import <vector>;
import <map>;

#if defined(_DEBUG)
import Game.TerrainEditor;
#endif
import Game.Editor.AreaEditor;
import Game.Editor.EnemyEditor;
import Game.Editor.ActorEditor;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Core.String;
import Lumina.Utils.Data;
import Lumina.D3D12;
import Lumina.MeshManager;

import Game.Terrain;
import Lumina.Utils.Camera;
import Lumina.Primitive;
import Lumina.DeferredLighting;
import ParticleSystem;

import Game.Player;
import CollisionManager;
import Game.ConvexColliderDebug;
import Collider;

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
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Update_(_ARGs&&...args_) -> void;

	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Render_(_ARGs&&...args_) -> void;

	private:
		void Render_Geometry();
		void Render_Merge();

	public:
		void Update();
		void Render();

	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Initialize_(_ARGs&&...args_) -> void;

		void SyncPlayEnemiesFromManager();

	public:
		void Initialize();

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

		MeshMaterial Material0_;
		std::vector<std::unique_ptr<Lumina::D3D12::UploadBuffer>> UB_Materials_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Materials_;
		Lumina::D3D12::UploadBuffer UB_WorldToHomogeneous_;

		std::map<std::string, size_t> EnemyMeshIndices_;
		size_t CubeMeshIdx_{ 0 };

		Lumina::D3D12::DescriptorTable GlobalTable_SRV_ImageTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_CanvasTexture_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Scene_;

		std::unique_ptr<Lumina::Utils::Camera> Camera_;
		std::unique_ptr<Lumina::Utils::Camera> Camera_Player_;
		std::unique_ptr<Lumina::Math::F32x4x4<>> WorldToHomogeneous_;

		std::unique_ptr<TerrainEditor> TerrainEditor_;

		std::unique_ptr<TerrainShapeCollection> TerrainScreenData_;
		std::unique_ptr<TerrainShapeCollection> Terrain_;
		std::unique_ptr<TerrainRenderer> TerrainRenderer_;
		std::unique_ptr<Player> Player_;
		std::unique_ptr<CollisionManager> CollisionManager_;
		std::unique_ptr<ConvexColliderDebugRenderer> ConvexColliderDebugRenderer_;

		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager_;

	private:
		// エディタ統合
		enum class EditorTab { None, Motion, Area, Enemy, Actor, Terrain, Play };
		EditorTab activeEditor_{ EditorTab::Play };
		Game::Editor::AreaEditor areaEditor_;
		Game::Editor::EnemyEditor enemyEditor_;
		Game::Editor::ActorEditor actorEditor_;

		struct Character {
			Lumina::Math::F32x3 Position{ 100.0f, 0.0f, 0.0f }; // Y=0 is ground
			Lumina::Math::F32x3 Velocity{ 0.0f, 0.0f, 0.0f };
			bool FacingRight = true;
			int HP = 100;
			int MaxHP = 100;
			int Mana = 0;
			float HurtTimer = 0.0f;
			float ManaTimer = 0.0f;
		};

		struct PlayEnemy {
			Game::Editor::EnemyData BaseData;
			Lumina::Math::F32x3 Position{ 0.0f, 0.0f, 0.0f };
			int CurrentHP = 100;
			bool IsDead = false;
			float HurtTimer = 0.0f;
			bool FacingRight = true;
			int SizeTier = 1;
			float Scale = 1.0f;
		};

		struct PlayState {
			bool IsPlaying = false;
			bool IsGoalReached = false;
			Game::Editor::AreaData CurrentArea;
			Character Player;
			std::vector<PlayEnemy> Enemies;

			
			float PlayerSpeedMultiplier = 1.0f;
			float PlayerAttackPower = 10.0f;
			
			float BuffSpeedTimer = 0.0f;
			float BuffAttackTimer = 0.0f;
			
			float PlayerAttackTimer = 0.0f;
			
			float TransitionCooldownTimer = 0.0f;
			std::vector<std::shared_ptr<ConvexCollider>> PortalColliders;
		} playState_;

#if defined(_DEBUG)
		void CheckAndLoadArea(int areaIndex, int previousAreaIndex = -1);
		void DrawPlayMode();
#endif

		/// パーティクル・ライティング

	private:
		std::unique_ptr<Lumina::DeferredLighting> DeferredLighting_;
		Lumina::List<Lumina::PointLight> List_PointLight_;
		Lumina::List<Lumina::Math::F32x4x4<>> List_LocalToWorld_LightSphere_;
		std::vector<Lumina::U32> Arr_Index_ActivePointLight_;

		Lumina::D3D12::RootSignature RS_ParticleSystem_;
		Lumina::D3D12::Shader VS_BasicParticle_;
		Lumina::D3D12::Shader PS_BasicParticle_;
		Lumina::D3D12::GraphicsPSO GraphicsPSO_BasicParticle_AdditiveMode_;
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> AmbientSparkles_;
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> PlayerEffects_;
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> KnockEffects_;

	};
}