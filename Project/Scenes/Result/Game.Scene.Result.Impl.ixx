export module Game.Scene.Result : Impl;

import <memory>;

import Lumina.Core.Common;
import Lumina.Core.Math;

namespace Game::Scene::Impl {
	export class Result {
	public:
		void Update();
		void Render();

	public:
		void Initialize();

		Result();
		virtual ~Result();

	private:
		// リザルト画面の状態
		float AnimationTimer_{ 0.0f };
		bool IsReturnRequested_{ false };
		float FadeAlpha_{ 0.0f };       // フェードアウト用
		float DisplayTimer_{ 0.0f };    // リザルト表示演出用
	};
}
