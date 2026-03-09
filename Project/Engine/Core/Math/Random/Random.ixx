export module Lumina.Core.Math : Random;

//****	******	******	******	******	****//

import <random>;

import <memory>;

//////	//////	//////	//////	//////	//////

namespace {
	template<typename T>
	using UniPtr = std::unique_ptr<T>;
}

//****	******	******	******	******	****//

namespace Lumina::Math {
	export class Random {
		class Engine {
			friend Random;

		public:
			inline auto operator()() -> std::mt19937::result_type { return MT19937_(); }

		private:
			std::mt19937 MT19937_{ std::random_device{}() };
		};

	public:
		inline static Engine& Generator() {
			static UniPtr<Engine> engine{ std::make_unique<Engine>() };
			return *engine;
		}
	};
}