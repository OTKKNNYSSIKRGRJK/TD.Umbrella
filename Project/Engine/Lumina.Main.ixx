export module Lumina.Main;

import <memory>;

import Lumina.Core.Common;
import Lumina.Core.Math;

import Lumina.OS.Windows.Context;
import Lumina.OS.Windows.RawInput;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;

import Lumina.ResourceManager;
export import Lumina.Scene;
export import Lumina.Sprite;
export import Lumina.MeshManager;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace Lumina {
	export class Context : public NonCopyable<Context> {
	public:
		static inline auto Instance() -> Context& {
			static std::unique_ptr<Context> inst{ std::make_unique<Context>() };
			return *inst;
		}
		
	public:
		auto WinAppContext() const noexcept
			-> OS::Windows::Context const& { return WinAppContext_; }
		auto RawInputContext() const noexcept
			-> OS::Windows::RawInput const& { return *MainWindowRawInput_; }
		auto D3D12Context() const noexcept
			-> D3D12::Context const& { return D3D12Context_; }
		auto MainCommandList() const noexcept
			-> D3D12::CommandList const& { return CmdList_; }
		auto ResourceContext() const noexcept
			-> ResourceManager const& { return ResourceManager_; }
		auto SpriteManager() const noexcept
			-> SpriteRenderer& { return *SpriteRenderer_; }
		auto MeshContext() const noexcept
			-> MeshManager& { return *MeshManager_; }

	public:
		auto Run() -> I32;

	public:
		void Initialize();
		void Finalize();

	private:
		Math::F32x4x4<> OrthoProjMat_;

		OS::Windows::Context WinAppContext_;
		OS::Windows::RawInput const* MainWindowRawInput_{ nullptr };
		D3D12::Context D3D12Context_;
		ResourceManager ResourceManager_;
		std::unique_ptr<SpriteRenderer> SpriteRenderer_{ nullptr };
		std::unique_ptr<MeshManager> MeshManager_{ nullptr };

		D3D12::DescriptorTable GlobalTable_ImageTextures_;
		D3D12::DescriptorHeap LocalHeap_OrthoProjMat_;

		D3D12::UploadBuffer UB_OrthoProjMat_;

		D3D12::CommandAllocator CmdAllocator_;
		D3D12::CommandList CmdList_;

		D3D12::Shader VS_Sprite_;
		D3D12::Shader PS_Sprite_;
		D3D12::GraphicsPSO PSO_Sprite_;
	};
}