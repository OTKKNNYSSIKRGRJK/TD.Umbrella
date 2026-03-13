export module Game.Scene.InGame : Impl;

import <memory>;

import <vector>;

import Lumina;
import Game.EnemyBase;

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
		// 敵の管理用コンテナ
		std::vector<std::unique_ptr<Game::EnemyBase>> Enemies_{};
	};
}