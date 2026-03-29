export module Lumina.Utils.Misc;

import Lumina.Core.Common;

namespace Lumina::Utils {
	//	The same as D3D12_VIEWPORT
	export struct Viewport {
		F32 TopLeftX;
		F32 TopLeftY;
		F32 Width;
		F32 Height;
		F32 MinDepth;
		F32 MaxDepth;
	};
}