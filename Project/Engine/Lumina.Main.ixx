module;

#include<Windows.h>

#include<d3d12.h>

export module Lumina : Main;

import <memory>;

import Lumina.Core.Common;

import Lumina.OS.Windows.Context;
import Lumina.OS.Windows.RawInput;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;

import Lumina.ResourceManager;
export import Lumina.Scene;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

#if defined(_DEBUG)
namespace {
	void SetImGuiAppearance() {
		//ImGui::GetIO().Fonts->AddFontFromFileTTF("C:/Windows/Fonts/consola.ttf", 12.0f);
		ImGui::GetIO().Fonts->AddFontFromFileTTF("Assets/Fonts/AnonymousPro/AnonymousPro-Regular.ttf", 12.0f);

		ImGuiStyle& style{ ImGui::GetStyle() };
		style.WindowRounding = 3.0f;
		style.ChildRounding = 3.0f;
		style.PopupRounding = 3.0f;
		style.FrameRounding = 3.0f;
		style.FrameBorderSize = 1.0f;
		style.GrabRounding = 3.0f;
		style.TabBorderSize = 1.0f;
		style.TabRounding = 3.0f;
		style.SeparatorTextBorderSize = 1.0f;
		style.SeparatorTextPadding.y = 6.0f;
		style.CellPadding.y = 6.0f;

		ImVec4* colors{ style.Colors };
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.02f, 0.02f, 0.03f, 0.94f };
		colors[ImGuiCol_ChildBg] = ImVec4{ 0.02f, 0.02f, 0.03f, 0.06f };
		colors[ImGuiCol_PopupBg] = ImVec4{ 0.06f, 0.08f, 0.10f, 0.94f };
		colors[ImGuiCol_Border] = ImVec4{ 0.71f, 0.54f, 0.13f, 0.25f };
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.39f, 0.63f, 0.87f, 0.19f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.28f, 0.41f, 0.52f, 0.19f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.39f, 0.63f, 0.87f, 0.38f };
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.00f, 0.01f, 0.02f, 1.00f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.02f, 0.09f, 0.16f, 1.00f };
		colors[ImGuiCol_CheckMark] = ImVec4{ 0.50f, 0.58f, 0.68f, 1.00f };
		colors[ImGuiCol_SliderGrab] = ImVec4{ 0.39f, 0.56f, 0.61f, 0.75f };
		colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.46f, 0.71f, 0.79f, 0.75f };
		colors[ImGuiCol_Button] = ImVec4{ 0.01f, 0.06f, 0.08f, 0.75f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.03f, 0.09f, 0.18f, 0.25f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.12f, 0.38f, 0.71f, 0.50f };
		colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.10f, 0.24f, 0.40f, 0.78f };
		colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.05f, 0.24f, 0.45f, 1.00f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.20f, 0.47f, 0.59f, 0.75f };
		colors[ImGuiCol_Tab] = ImVec4{ 0.05f, 0.20f, 0.25f, 0.13f };
		colors[ImGuiCol_TabSelected] = ImVec4{ 0.07f, 0.29f, 0.44f, 0.63f };
		colors[ImGuiCol_TabSelectedOverline] = ImVec4{ 0.07f, 0.29f, 0.44f, 0.63f };
		colors[ImGuiCol_TableHeaderBg] = ImVec4{ 0.01f, 0.07f, 0.11f, 0.50f };
		colors[ImGuiCol_TableRowBgAlt] = ImVec4{ 0.24f, 0.23f, 0.21f, 0.06f };
		colors[ImGuiCol_Header] = ImVec4{ 0.12f, 0.33f, 0.47f, 0.31f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.07f, 0.29f, 0.44f, 0.63f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.20f, 0.47f, 0.59f, 0.75f };
		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.00f, 0.01f, 0.02f, 0.25f };
		colors[ImGuiCol_DragDropTarget] = ImVec4{ 0.80f, 0.68f, 0.20f, 0.75f };
		colors[ImGuiCol_TableBorderStrong] = ImVec4{ 0.76f, 0.57f, 0.20f, 0.38f };
		colors[ImGuiCol_TableBorderLight] = ImVec4{ 0.38f, 0.38f, 0.34f, 0.25f };
	}
}
#endif

namespace Lumina {
	export class Context {
	public:
		auto WinAppContext() const noexcept
			-> OS::Windows::Context const& { return WinAppContext_; }
		auto RawInputContext() const noexcept
			-> OS::Windows::RawInput const& { return *MainWindowRawInput_; }
		auto D3DContext() const noexcept
			-> D3D12::Context const& { return D3DContext_; }
		auto ResourceContext() const noexcept
			-> ResourceManager const& { return ResourceManager_; }

	public:
		auto Run() -> B1;

	public:
		void Initialize();
		void Finalize();

	private:
		OS::Windows::Context WinAppContext_{};
		OS::Windows::RawInput const* MainWindowRawInput_{ nullptr };
		D3D12::Context D3DContext_{};
		ResourceManager ResourceManager_{};

		D3D12::CommandAllocator CmdAllocator_{};
		D3D12::CommandList CmdList_{};
	};

	auto Context::Run() -> B1 {
		if (WinAppContext_.ProcessMessage() == 0) {
			[[maybe_unused]] ID3D12DescriptorHeap* descriptorHeaps[]{ D3DContext_.GlobalDescriptorHeap().Get() };

			[[maybe_unused]] auto& directQueue{ D3DContext_.DirectQueue() };

			[[maybe_unused]] auto const& keyboard = MainWindowRawInput_->Keyboard();
			[[maybe_unused]] auto const& mouse = MainWindowRawInput_->Mouse();

			D3DContext_.BeginFrame(CmdList_);

			SceneManager::Instance().Update();
			SceneManager::Instance().Render();

			D3DContext_.EndFrame(CmdAllocator_, CmdList_);

			//----	------	------	------	------	----//

			if (keyboard.IsPressed(Lumina::OS::Windows::KEY::ESC)) {
				::SendMessage(WinAppContext_.WindowInstance(L"Main").Handle(), WM_CLOSE, 0, 0);
			}

			return 1;
		}

		return 0;
	}

	void Context::Initialize() {
		Lumina::OS::Windows::WindowConfig mainWindowConfig_{
			.Name{ L"Main" },
			.Title{ L"Usus Magister Est Optimus" },
			.Style{
				Lumina::OS::Windows::WindowStyle::TitleBar |
				Lumina::OS::Windows::WindowStyle::WindowMenu |
				Lumina::OS::Windows::WindowStyle::MinimizeButton
			},
			.ClientWidth{ 1280U },
			.ClientHeight{ 720U },
		};
		WinAppContext_.Initialize(mainWindowConfig_);
		auto const& mainWindow{ WinAppContext_.WindowInstance(L"Main") };
		MainWindowRawInput_ = &WinAppContext_.RawInputContext(mainWindow);

		//----	------	------	------	------	----//

		D3DContext_.Initialize(WinAppContext_);
		auto const& device{ D3DContext_.Device() };

		auto const& gpuDH{ D3DContext_.GlobalDescriptorHeap() };

		CmdAllocator_.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		CmdList_.Initialize(device, CmdAllocator_);

		ResourceManager_.Initialize(D3DContext_);

		//----	------	------	------	------	----//

		#if defined(_DEBUG)
		[[maybe_unused]] auto const& swapChain{ D3DContext_.SwapChain() };
		Lumina::Utils::ImGuiManager::Initialize(mainWindow.Handle(), device, swapChain, gpuDH);
		WinAppContext_.RegisterCallback(Lumina::Utils::ImGuiManager::WindowProcedure);
		SetImGuiAppearance();
		#endif

		SceneManager::Instance().Initialize();
	}

	void Context::Finalize() {
		SceneManager::Instance().Finalize();

		D3DContext_.DirectQueue().SignalAndCPUWait();

		#if defined(_DEBUG)
		Lumina::Utils::ImGuiManager::Finalize();
		#endif

		ResourceManager_.Finalize();

		WinAppContext_.Finalize();
	}
}