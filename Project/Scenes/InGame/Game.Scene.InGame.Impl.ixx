export module Game.Scene.InGame : Impl;

import <memory>;

import <vector>;

import Lumina;
import Game.Editor.AreaEditor;
import Game.Editor.EnemyEditor;
import Game.Editor.ActorEditor;

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
		// エディタ統合
		enum class EditorTab { None, Motion, Area, Enemy, Actor, Play };
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
		} playState_;

#if defined(_DEBUG)
		void CheckAndLoadArea(int areaIndex, int previousAreaIndex = -1);
		void DrawPlayMode();
		void UpdatePlayLogic();
#endif

	private:
	};
}