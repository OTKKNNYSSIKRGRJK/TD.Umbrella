export module Game.Scene.Title : Impl;

namespace Game::Scene::Impl {
	export class Title {
	public:
		void Update();
		void Render();

	public:
		void Initialize();

		Title();
		virtual ~Title();
	};
}