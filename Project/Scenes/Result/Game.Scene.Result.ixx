export module Game.Scene.Result;

import <memory>;

import Lumina.Scene;

namespace Game::Scene {
	namespace Impl { class Result; }

	export class Result : public Lumina::Scene {
	public:
		virtual void Update() override;
		virtual void Render() override;

	public:
		template<typename...ArgTypes>
		void Initialize(typename ArgTypes const&...args_);

		Result();
		virtual ~Result();

	private:
		std::unique_ptr<Impl::Result> Impl_{ nullptr };
	};
}

namespace Lumina {
	template<>
	void SceneManager::Load<"Result">() {
		Load<Game::Scene::Result>("Result");
		
	}
}
