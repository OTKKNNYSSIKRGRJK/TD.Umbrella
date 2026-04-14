export module Lumina.Scene;

import <cstdint>;

import <vector>;
import <list>;
import <unordered_map>;

import <string>;
import <format>;

import Lumina.Core.Common;
import Lumina.Core.String;

namespace Lumina {
	class Scene;
	class SceneManager;

	//////	//////	//////	//////	//////	//////

	export class Scene : public NonCopyable<Scene> {
	public:
		virtual void Update() = 0;
		virtual void Render() = 0;

	public:
		virtual ~Scene() = default;
	};

	namespace Concept {
		template<typename T>
		concept Scene = std::is_base_of_v<Lumina::Scene, T>;
	}

	//////	//////	//////	//////	//////	//////

	export class SceneManager : public NonCopyable<SceneManager> {
	public:
		static constexpr int MaxNum_Scenes{ 128 };

	private:
		struct SceneNode {
			std::unique_ptr<Scene> Data;
			U32 IsActive;
		};

	public:
		static inline SceneManager& Instance() {
			static SceneManager inst{};
			return inst;
		}

	public:
		bool IsActive(std::string_view name_) {
			auto&& it{ LoadedScenes_.find(name_.data()) };
			return { (it != LoadedScenes_.cend()) && (it->second.IsActive) };
		}

	public:
		template<Concept::Scene _Scene, typename..._ARGs>
		void Load(std::string_view name_, _ARGs&&...args_) {
			if (LoadedScenes_.find(name_.data()) == LoadedScenes_.cend()) {
				LoadedScenes_.emplace(
					name_,
					SceneNode{
						.Data{ std::make_unique<_Scene>(std::forward<_ARGs>(args_)...) },
						.IsActive{ 0 }
					}
				);
			}
		}

		template<StringLiteral SceneName>
		void Load();

		void Unload(std::string_view name_) {
			auto&& it_SceneNodeKV{ LoadedScenes_.find(name_.data()) };
			if (it_SceneNodeKV != LoadedScenes_.cend()) {
				it_SceneNodeKV->second.Data.reset(nullptr);
				LoadedScenes_.erase(it_SceneNodeKV);
			}
		}

		void Activate(std::string_view name_) {
			ActivationQueue_.emplace_back(name_);
		}

		void Update() {
			for (auto const& name : ActivationQueue_) {
				auto&& it_Scene{ LoadedScenes_.find(name) };
				if (it_Scene != LoadedScenes_.cend()) {
					it_Scene->second.IsActive = 1U;
				}
			}
			ActivationQueue_.clear();

			for (auto const& name : DeactivationQueue_) {
				auto&& it_Scene{ LoadedScenes_.find(name) };
				if (it_Scene != LoadedScenes_.cend()) {
					it_Scene->second.IsActive = 0U;
				}
			}
			DeactivationQueue_.clear();
		}

		void UpdateActive() {
			for (auto& kv : LoadedScenes_) {
				auto& sceneNode{ kv.second };
				if (sceneNode.IsActive) {
					sceneNode.Data->Update();
				}
			}
		}

		void RenderActive() {
			for (auto& kv : LoadedScenes_) {
				auto& sceneNode{ kv.second };
				if (sceneNode.IsActive) {
					sceneNode.Data->Render();
				}
			}
		}

	public:
		void Initialize() {}
		void Finalize() {
			for (auto& kv : LoadedScenes_) {
				kv.second.Data.reset(nullptr);
			}
			LoadedScenes_.clear();
		}

	private:
		constexpr SceneManager() noexcept = default;
	public:
		~SceneManager() { Finalize(); }

	private:
		std::unordered_map<std::string, SceneNode> LoadedScenes_{};

		std::vector<std::string> ActivationQueue_{};
		std::vector<std::string> DeactivationQueue_{};
	};
}