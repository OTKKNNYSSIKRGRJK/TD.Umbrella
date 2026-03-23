export module Game.Scene.InGame : Impl;

import <memory>;

import <vector>;

import Lumina;
import Lumina.MeshManager;
import Lumina.Utils.Data.Mesh;
import Game.CharacterTest;
import Game.Editor.AreaEditor;
import Game.Editor.EnemyEditor;

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
		Game::CharacterTest Test_;

		// エディタ・プレイループ統合
		enum class EditorTab { None, Motion, Area, Enemy, Play };
		EditorTab activeEditor_{ EditorTab::Play };
		Game::Editor::AreaEditor areaEditor_;
		Game::Editor::EnemyEditor enemyEditor_;

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
		};

		struct PlayState {
			bool IsPlaying = false;
			bool IsGoalReached = false;
			Game::Editor::AreaData CurrentArea;
			Character Player;
			std::vector<PlayEnemy> Enemies;

			// Debug buffs
			float PlayerSpeedMultiplier = 1.0f;
			float PlayerAttackPower = 10.0f;
			
			float BuffSpeedTimer = 0.0f;
			float BuffAttackTimer = 0.0f;
			
			float PlayerAttackTimer = 0.0f;
		} playState_;

#if defined(_DEBUG)
		void CheckAndLoadArea(int areaIndex);
		void DrawPlayMode();
		void UpdatePlayLogic();
#endif

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

		std::unique_ptr<Lumina::D3D12::RenderPass> GeometryPass_;
		Lumina::D3D12::Canvas Canvas_GeometryPass_;
	};
}