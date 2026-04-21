export module Game.Scene.Title;

import <memory>;

import Lumina.Scene;

namespace Game::Scene {
	namespace Impl { class Title; }

	export class Title : public Lumina::Scene {
	public:
		virtual void Update() override;
		virtual void Render() override;

	public:
		template<typename...ArgTypes>
		void Initialize(typename ArgTypes const&...args_);

		Title();
		virtual ~Title();

	private:
		std::unique_ptr<Impl::Title> Impl_{ nullptr };
	};
}

namespace Lumina {
	template<>
	void SceneManager::Load<"Title">() {
		Load<Game::Scene::Title>("Title");
	}
}