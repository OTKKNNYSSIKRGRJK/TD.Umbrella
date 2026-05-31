//export module Game.Scene.Clear;
//
//import <memory>;
//
//import Lumina.Scene;
//
//namespace Game::Scene {
//	namespace Impl { class Clear; }
//
//	export class Clear : public Lumina::Scene {
//	public:
//		virtual void Update() override;
//		virtual void Render() override;
//
//	public:
//		template<typename...ArgTypes>
//		void Initialize(typename ArgTypes const&...args_);
//
//		Clear();
//		virtual ~Clear();
//
//	private:
//		std::unique_ptr<Impl::Clear> Impl_{ nullptr };
//	};
//}
//
//namespace Lumina {
//	template<>
//	void SceneManager::Load<"Clear">() {
//		Load<Game::Scene::Clear>("Clear");
//	}
//}