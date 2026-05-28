export module Game.Scene.InGame : Impl;

import <memory>;

import <string>;
import <vector>;
import <map>;
import <set>;

#if defined(_DEBUG)
import Game.TerrainEditor;
#endif
import Game.Editor.AreaEditor;
import Game.Editor.EnemyEditor;
import Game.Editor.ActorEditor;
import Game.Editor.AudioEditor;
import Game.Editor.ObjMotionEditor;
import Game.TutorialManager;
import Game.UIMenu;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Core.String;
import Lumina.Utils.Data;
import Lumina.D3D12;
import Lumina.D3D12.Aux.View;
import Lumina.MeshManager;

import Game.Terrain;
import Lumina.Utils.Camera;
import Lumina.Primitive;
import Lumina.DeferredLighting;
import ParticleSystem;

import Game.Player;
import Game.ExpOrbManager;
import CollisionManager;
import Game.ConvexColliderDebug;
import Collider;

import Lumina.CG3D.Struct;

import Game.Events.InGame;
import Lumina.Cylinder;
import Lumina.Skybox;
import Lumina.Watercolor;

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

		void Render_Geometry();
		void Render_Merge();

	public:
		void Update();
		void Render();

	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Initialize_(_ARGs&&...args_) -> void;

		void SyncPlayEnemiesFromManager();
		bool HasBossEncounterInCurrentArea() const;
		void StartBossEncounterPresentation();

	public:
		void Initialize();

		InGame();
		virtual ~InGame();

	private:
		struct MeshRange {
			size_t startIndex;
			size_t count;
		};

		struct MeshMaterial {
			Lumina::F32x4 RGBA{ 1.0f, 1.0f, 1.0f, 1.0f };
			Lumina::U32 ID_DiffuseMap;
			Lumina::U32 ID_SpecularMap;
			Lumina::U32 ID_NormalMap;
		};

		struct SkinnedModel {
			Lumina::CG3D::Collection Collection_;
			std::vector<Lumina::CG3D::MyAnimation> Animations_;

			Lumina::D3D12::UploadBuffer VertexBuffer_;
			Lumina::D3D12::UploadBuffer IndexBuffer_;
			Lumina::D3D12::VBV VBV_;
			Lumina::D3D12::IBV IBV_;
		};

		struct SkinnedInstance {
			Lumina::CG3D::Skeleton Skeleton_;
			Lumina::CG3D::SkinCluster SkinCluster_;
			float animTimer_ = 0.0f;
			int currentAnimIndex_ = 0;
			
			Lumina::D3D12::UploadBuffer TransformsBuffer_;
			Lumina::D3D12::DescriptorTable CBV_SceneTable_;
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

		std::map<std::string, MeshRange> EnemyMeshIndices_;
		std::map<std::string, size_t> EnemyMaterialIndices_;
		std::map<std::string, uint32_t> EnemyTextureIndices_;
		std::map<std::string, std::shared_ptr<SkinnedModel>> EnemySkinnedModels_;
		std::map<uint32_t, std::shared_ptr<SkinnedInstance>> EnemySkinnedInstances_;
		std::map<std::string, MeshRange> ActorMeshIndices_;  // actor名 → メッシュ範囲
		size_t CubeMeshIdx_{ 0 };

		std::vector<std::pair<std::string, std::string>> AdditionalTextures_;


		Lumina::D3D12::DescriptorTable GlobalTable_SRV_ImageTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_CanvasTexture_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Scene_;

		std::unique_ptr<Lumina::Utils::Camera> Camera_;
		std::unique_ptr<Lumina::Utils::Camera> Camera_Player_;
		std::unique_ptr<Lumina::Math::F32x4x4<>> WorldToHomogeneous_;

		#if defined(_DEBUG)
		std::unique_ptr<TerrainEditor> TerrainEditor_;
		#endif

		std::unique_ptr<TerrainShapeCollection> TerrainScreenData_;
		std::unique_ptr<TerrainShapeCollection> Terrain_;
		std::unique_ptr<TerrainRenderer> TerrainRenderer_;
		std::unique_ptr<Player> Player_;
		std::unique_ptr<CollisionManager> CollisionManager_;
		std::unique_ptr<ConvexColliderDebugRenderer> ConvexColliderDebugRenderer_;

		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager_;

	private:
		// エディタ統合
		enum class EditorTab { None, Motion, ObjMotion, Area, Enemy, EnemyAction, Actor, Terrain, Audio, Play };
		EditorTab activeEditor_{ EditorTab::Play };
		Game::Editor::AreaEditor areaEditor_;
		Game::Editor::EnemyEditor enemyEditor_;
		Game::Editor::EnemyActionEditor enemyActionEditor_;
		Game::Editor::ActorEditor actorEditor_;
		Game::Editor::AudioEditor audioEditor_;
		Game::Editor::ObjMotionEditor objMotionEditor_;

		// チュートリアルシステム
		std::unique_ptr<Game::TutorialManager> TutorialManager_;
		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager_Tutorial_;

		// ミニマップ専用
		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager_Minimap_;
		bool minimapExpanded_ = false;

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
            std::string CurrentAction = "Idle";
			int CurrentHP = 100;
			bool IsDead = false;
			float HurtTimer = 0.0f;
			bool FacingRight = true;
           float RenderFacingYaw = 0.0f;
		   float RenderPitch = 0.0f;
           float SpawnTimer = 0.0f;
			float SpawnDuration = 0.0f;
			int SizeTier = 1;
			float Scale = 1.0f;
            // Visual-only transform applied during prep/windup (copied from runtime instance)
			Lumina::Math::F32x3 VisualOffset{ 0.0f, 0.0f, 0.0f };
			float VisualYaw = 0.0f;
            // Runtime id for matching across frames
			uint32_t Id = 0;
			int PlacementIndex = -1;
			bool WalkActive = false; // debug flag from behavior
			bool MotionPlaying = false;
			int ActiveNodeIndex = -1;
		};

		struct PlayState {
			bool IsPlaying = false;
			bool IsPaused = false;
			int PauseSelectedIndex = 0;
			float PauseAnimationTimer = 0.0f;
			bool PrevPauseUpHeld = false;
			bool PrevPauseDownHeld = false;
			bool PrevPauseDecideHeld = false;
			bool IsGoalReached = false;
			Game::Editor::AreaData CurrentArea;
			std::set<int> VisitedAreas;
			Character Player;
			std::vector<PlayEnemy> Enemies;
			std::set<std::pair<int, int>> DefeatedEnemies;

			
			float PlayerSpeedMultiplier = 1.0f;
			float PlayerAttackPower = 10.0f;
			
			float BuffSpeedTimer = 0.0f;
			float BuffAttackTimer = 0.0f;
			
			float PlayerAttackTimer = 0.0f;
			
			float TransitionCooldownTimer = 0.0f;
			std::vector<std::shared_ptr<ConvexCollider>> PortalColliders;
           bool IsBossPresentationActive = false;
			float BossPresentationTimer = 0.0f;
			float BossPresentationDuration = 0.0f;
			Lumina::Math::F32x3 BossPresentationFocusPosition{ 0.0f, 0.0f, 0.0f };
			
			// Screen Fade properties
			int ScreenFadeState = 2; // 0: None, 1: FadeOut, 2: FadeIn (Start with FadeIn on load)
			float ScreenFadeAlpha = 1.0f;
			int ScreenFadeNextAction = 0;
			float ScreenFadeSpeed = 1.5f;
		} playState_;

		void CheckAndLoadArea(int areaIndex, int previousAreaIndex = -1);
		void DrawGamePhaseUI();
		void HandleFallDeath();
#if defined(_DEBUG)
		void DrawPlayMode();
		void DrawPauseMenu();
		void DrawEnemyHPBars();
#endif

		// ゲームオーバー用UIメニュー
		Game::UIMenu GameOverMenu_;

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

		// * パーティクルシェーダー用

		Lumina::D3D12::DescriptorHeap LocalHeap_CBV_;
		Lumina::D3D12::UploadBuffer UB_WorldToProjective_;

		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> AmbientSparkles_;
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> Raindrops_;
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> PlayerEffects_;
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle2>> UmbrellaEffects_;
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> KnockEffects_;
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> EnemyEffects_;

	private:
		Lumina::D3D12::RootSignature RS_Skinning_;
		Lumina::D3D12::Shader VS_SkinnedMeshDeferredGeometry_;
		Lumina::D3D12::Shader PS_SkinnedMeshDeferredGeometry_;
		Lumina::D3D12::GraphicsPSO GraphicsPSO_SkinnedMeshDeferredGeometry_;

		Lumina::D3D12::DescriptorTable GlobalTable_Materials_;
		Lumina::D3D12::UploadBuffer UB_Transforms_;

		Lumina::D3D12::DescriptorTable GlobalTable_CBV_Scene_;

		std::unique_ptr<Lumina::Watercolor> Watercolor_;

		// * Portal Cylinders

	private:
		Lumina::D3D12::RootSignature RS_Portal_;
		Lumina::D3D12::Shader VS_Portal_;
		Lumina::D3D12::Shader PS_Portal_;
		Lumina::D3D12::GraphicsPSO PSO_Portal_;
		std::unique_ptr<Lumina::Cylinder> Portals_[8];

		// * Skybox

	private:
		std::unique_ptr<Lumina::Skybox> Skybox_;
	};
}