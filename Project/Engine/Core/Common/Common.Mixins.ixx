export module Lumina.Core.Common : Mixins;

//////	//////	//////	//////	//////	//////

namespace Lumina {
	export template<typename T>
	class NonCopyable {
	protected:
		NonCopyable() noexcept = default;
		~NonCopyable() noexcept = default;

	public:
		NonCopyable(NonCopyable&&) noexcept = default;
		NonCopyable& operator=(NonCopyable&&) noexcept = default;

		NonCopyable(const NonCopyable&) = delete;
		T& operator=(const T&) = delete;
	};
}