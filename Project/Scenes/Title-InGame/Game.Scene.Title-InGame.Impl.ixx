export module Game.Scene.Title2InGame : Impl;

namespace Game::Scene::Impl {
	export class Title2InGame {
	public:
		void Update();
		void Render();

	public:
		void Initialize();

		Title2InGame();
		virtual ~Title2InGame();
	};
}