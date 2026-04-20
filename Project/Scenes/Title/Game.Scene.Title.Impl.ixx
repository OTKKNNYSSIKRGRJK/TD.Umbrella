export module Game.Scene.Title : Impl;

import <memory>;

import Lumina.Core.Common;
import Lumina.Core.Math;

namespace Game::Scene::Impl {
	export class Title {
	public:
		void Update();
		void Render();

	public:
		void Initialize();

		Title();
		virtual ~Title();

	private:
		// タイトル画面の状態
		float AnimationTimer_{ 0.0f };
		bool IsStartRequested_{ false };
		float FadeAlpha_{ 0.0f };       // フェードアウト用
		float BlinkTimer_{ 0.0f };      // "Press Start" 点滅用
	};
}
