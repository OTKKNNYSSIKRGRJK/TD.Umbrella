module;

#include<d3d12.h>
#include<dxgi1_6.h>

// dxgidebug.h			| Contains IDXGIDebug1, which is used for leak check.
#include<dxgidebug.h>

export module Lumina.D3D12 : Debug;

import Lumina.Core.Debug;

//////	//////	//////	//////	//////	//////

namespace Lumina::D3D12 {
	Debug::Logger& Logger() {
		static Debug::Logger logger{ "D3D12" };
		return logger;
	}
}

namespace Lumina::D3D12 {
	#if defined(_DEBUG)
	// Debug layer should be enabled before initialization of DirectX.
	export void EnableDebugLayer() {
		ID3D12Debug1* debugController{ nullptr };
		if (SUCCEEDED(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			Logger().Message<0U>("Debug interface obtained successfully.\n");

			debugController->EnableDebugLayer();
			debugController->SetEnableGPUBasedValidation(true);

			debugController->Release();
		}
	}
	#endif

	//****	******	******	******	******	****//

	export class LeakChecker final {
	public:
		// Checks if there are objects not yet released.
		~LeakChecker() {
			IDXGIDebug1* debugInterface{ nullptr };
			if (SUCCEEDED(::DXGIGetDebugInterface1(0U, IID_PPV_ARGS(&debugInterface)))) {
				debugInterface->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
				debugInterface->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
				debugInterface->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
				debugInterface->Release();
			}
		}
	};
}