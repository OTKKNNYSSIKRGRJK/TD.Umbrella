module;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// dxguid.lib			| GUID (Globally Unique Identifier)
#pragma comment(lib, "dxguid.lib")

//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////

export module Lumina.D3D12;

//****	******	******	******	******	****//

export import : GraphicsDevice;

export import : Command;
export import : Barrier;

export import : RenderPass;

export import : Resource;
export import : ImageTexture;
export import : Descriptor;

export import : Shader;
export import : RootSignature;
export import : PipelineState;

export import : Canvas;
export import : FrameBufferSwapChain;

export import : Debug;

export import <d3d12.h>;