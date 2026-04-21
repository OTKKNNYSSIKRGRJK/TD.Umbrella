export module Game.Scene.Title2InGame;

import <memory>;

import Lumina.Scene;

namespace Game::Scene {
	namespace Impl { class Title2InGame; }

	export class Title2InGame : public Lumina::Scene {
	public:
		virtual void Update() override;
		virtual void Render() override;

	public:
		template<typename...ArgTypes>
		void Initialize(typename ArgTypes const&...args_);

		Title2InGame();
		virtual ~Title2InGame();

	private:
		std::unique_ptr<Impl::Title2InGame> Impl_{ nullptr };
	};
}

namespace Lumina {
	template<>
	void SceneManager::Load<"Title->InGame">() {
		Load<Game::Scene::Title2InGame>("Title->InGame");
	}
}