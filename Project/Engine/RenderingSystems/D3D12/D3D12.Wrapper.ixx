module;

#include<d3d12.h>
#include<dxgi1_6.h>

//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////

export module Lumina.D3D12 : Wrapper;

//****	******	******	******	******	****//

import <type_traits>;

import <memory>;

import <format>;

import : Debug;

import Lumina.Core.Common;
import Lumina.Core.Debug;

//////	//////	//////	//////	//////	//////

namespace Lumina::D3D12 {
	template<typename WrapperType, typename WrappedType>
		requires(std::is_base_of_v<IUnknown, WrappedType>)
	class Wrapper {
	public:
		constexpr WrappedType* operator->() const noexcept { return Wrapped_; }
		constexpr WrappedType* Get() const noexcept { return Wrapped_; }
		constexpr WrappedType** GetAddressOf() noexcept { return &Wrapped_; }

		//----	------	------	------	------	----//

	public:
		constexpr bool IsInitialized() const noexcept { return (Wrapped_ != nullptr); }
		inline void ThrowIfInitialized(std::string_view debugName_) const {
			(Wrapped_ == nullptr) ||
			Debug::ThrowIfFalse<std::logic_error>{
				std::format(
					"<D3D12> Attempted to re-initialize {}!\n",
					debugName_
				)
			};
		}

		//----	------	------	------	------	----//

	public:
		constexpr std::string_view DebugName() const noexcept { return DebugName_; }
		constexpr void SetDebugName(std::string_view debugName_) {
			if (DebugName_ == nullptr) {
				DebugName_ = new C8[debugName_.size() + 1LLU];
				debugName_.copy(DebugName_, debugName_.size());
				DebugName_[debugName_.size()] = '\0';
			}

			#if defined(_DEBUG)
			if constexpr (
				std::is_base_of_v<ID3D12Object, WrappedType> ||
				std::is_base_of_v<IDXGIObject, WrappedType>
			) {
				Wrapped_->SetPrivateData(
					WKPDID_D3DDebugObjectName,
					static_cast<uint32_t>(DebugName().size()),
					DebugName_
				);
			}
			#endif
		}

		//----	------	------	------	------	----//

	protected:
		constexpr Wrapper() noexcept = default;
		virtual ~Wrapper() noexcept {
			if (IsInitialized()) {
				if (DebugName_ != nullptr) {
					Logger().Message<0U>(
						"Wrapper,{},Releasing {} object...\n",
						DebugName_,
						typeid(WrappedType).name()
					);
				}

				Wrapped_->Release();

				if (DebugName_ != nullptr) {
					Logger().Message<0U>(
						"Wrapper,{},{} object released successfully.\n",
						DebugName_,
						typeid(WrappedType).name()
					);
				}
			}

			if (DebugName_ != nullptr) {
				delete[] DebugName_;
				DebugName_ = nullptr;
			}
		}

		//====	======	======	======	======	====//

	protected:
		WrappedType* Wrapped_{ nullptr };

		//----	------	------	------	------	----//

	private:
		C8* DebugName_{ nullptr };
	};
}