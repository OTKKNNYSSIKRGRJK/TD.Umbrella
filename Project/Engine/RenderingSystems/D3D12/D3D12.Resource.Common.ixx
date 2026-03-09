module;

#include<d3d12.h>
#include<dxgi1_6.h>

//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////

export module Lumina.D3D12 : Resource.Common;

//****	******	******	******	******	****//

import <type_traits>;

//////	//////	//////	//////	//////	//////

namespace Lumina::D3D12 {
	consteval bool IsAllocatedInSystemRAM(
		D3D12_HEAP_PROPERTIES const& heapProperties_
	) noexcept {
		return (
			(heapProperties_.Type == D3D12_HEAP_TYPE_UPLOAD) ||
			//(heapProperties_.Type == D3D12_HEAP_TYPE_GPU_UPLOAD) ||
			(heapProperties_.Type == D3D12_HEAP_TYPE_READBACK) ||
			(heapProperties_.MemoryPoolPreference == D3D12_MEMORY_POOL_L0)
		);
	}

	struct ResourceSettings {
		D3D12_HEAP_PROPERTIES HeapProperties;
		D3D12_RESOURCE_FLAGS ResourceFlags;
		D3D12_RESOURCE_STATES InitialState;
	};

	// No multisampling
	export constexpr DXGI_SAMPLE_DESC SampleDesc_NoMultisampling{
		.Count{ 1U },
		.Quality{ 0U },
	};
}

//****	******	******	******	******	****//

namespace Lumina::D3D12 {
	class Resource {};
	class Buffer : public Resource {};
	class Texture2D : public Resource {};
}

namespace Lumina::D3D12::Concept {
	export template<typename T>
	concept Resource = std::is_base_of_v<D3D12::Resource, T>;
	export template<typename T>
	concept Buffer = std::is_base_of_v<D3D12::Buffer, T>;
	export template<typename T>
	concept Texture2D = std::is_base_of_v<D3D12::Texture2D, T>;
}