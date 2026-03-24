module;

#include<Windows.h>

module Lumina : Main;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.D3D12.Aux.View;

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

namespace Lumina{
	namespace {
		Math::F32x4x4<> OrthographicProjection(
			F32 l_, F32 r_,
			F32 t_, F32 b_,
			F32 zn_, F32 zf_
		) {
			F32 const inv_W{ 1.0f / (r_ - l_) };
			F32 const inv_H{ 1.0f / (t_ - b_) };
			F32 const inv_D{ 1.0f / (zf_ - zn_) };

			return Math::F32x4x4<>{
				2.0f * inv_W, 0.0f, 0.0f, 0.0f,
				0.0f, 2.0f * inv_H, 0.0f, 0.0f,
				0.0f, 0.0f, inv_D, 0.0f,
				-(l_ + r_) * inv_W, -(t_ + b_) * inv_H, -zn_ * inv_D, 1.0f,
			};
		}
	}
}

namespace Lumina {
	auto Context::Run() -> I32 {
		if (WinAppContext_.ProcessMessage() == 0) {
			[[maybe_unused]] ID3D12DescriptorHeap* descriptorHeaps[]{ D3D12Context_.GlobalDescriptorHeap().Get() };

			[[maybe_unused]] auto& directQueue{ D3D12Context_.DirectQueue() };

			[[maybe_unused]] auto const& keyboard = MainWindowRawInput_->Keyboard();
			[[maybe_unused]] auto const& mouse = MainWindowRawInput_->Mouse();

			CmdList_->SetDescriptorHeaps(1U, descriptorHeaps);

			D3D12Context_.BeginFrame(CmdList_);
			#if defined(_DEBUG)
			Lumina::Utils::ImGuiManager::BeginFrame();
			#endif

			auto rtv{ D3D12Context_.SwapChain().BackBufferRTVCPUHandle() };
			F32 const clearColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
			CmdList_->ClearRenderTargetView(rtv, clearColor, 0U, nullptr);

			SceneManager::Instance().Update();
			SceneManager::Instance().Render();

			CmdList_->OMSetRenderTargets(1U, &rtv, false, nullptr);

			/*SpriteRenderer_->Render(
				PSO_Sprite_,
				GlobalTable_ImageTextures_.GPUHandle(0U),
				LocalHeap_OrthoProjMat_.CPUHandle(0U)
			);*/
			
			#if defined(_DEBUG)
			Lumina::Utils::ImGuiManager::EndFrame(CmdList_);
			#endif
			D3D12Context_.EndFrame(CmdAllocator_, CmdList_);

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

		D3D12Context_.Initialize(WinAppContext_);
		auto const& device{ D3D12Context_.Device() };

		CmdAllocator_.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		CmdList_.Initialize(device, CmdAllocator_);

		ResourceManager_.Initialize(D3D12Context_);

		//----	------	------	------	------	----//

		SpriteRenderer_ = std::make_unique<SpriteRenderer>();
		SpriteRenderer_->Initialize(D3D12Context_, 4096);

		D3D12Context_.Compile(
			VS_Sprite_,
			L"Assets/Shaders/BasicSprite.VS.hlsl",
			L"vs_6_6",
			L"main",
			"BasicSprite.VS"
		);
		D3D12Context_.Compile(
			PS_Sprite_,
			L"Assets/Shaders/BasicSprite.PS.hlsl",
			L"ps_6_6",
			L"main",
			"BasicSprite.PS"
		);

		D3D12::BlendState spriteBlendState{};
		spriteBlendState.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC{
			.BlendEnable{ true },
			.LogicOpEnable{ false },
			.SrcBlend{ D3D12_BLEND_SRC_ALPHA },
			.DestBlend{ D3D12_BLEND_INV_SRC_ALPHA },
			.BlendOp{ D3D12_BLEND_OP_ADD },
			.SrcBlendAlpha{ D3D12_BLEND_SRC_ALPHA },
			.DestBlendAlpha{ D3D12_BLEND_ONE },
			.BlendOpAlpha{ D3D12_BLEND_OP_ADD },
			.LogicOp{ D3D12_LOGIC_OP_NOOP },
			.RenderTargetWriteMask{ D3D12_COLOR_WRITE_ENABLE_ALL },
		};
		D3D12::GraphicsPSO::InputLayout spriteInputLayout{};
		spriteInputLayout.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT);
		PSO_Sprite_.Initialize(
			device,
			SpriteRenderer_->RootSignature(),
			VS_Sprite_,
			PS_Sprite_,
			spriteBlendState,
			Lumina::D3D12::RasterizerState{
				.FillMode{ D3D12_FILL_MODE_SOLID },
				.CullMode{ D3D12_CULL_MODE_NONE },
			},
			Lumina::D3D12::DepthStencilState{
				.DepthEnable{ false },
				.StencilEnable{ false },
			},
			spriteInputLayout,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, },
			Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
		);

		//----	------	------	------	------	----//

		auto const& gpuDH{ D3D12Context_.GlobalDescriptorHeap() };

		#if defined(_DEBUG)
		[[maybe_unused]] auto const& swapChain{ D3D12Context_.SwapChain() };
		Lumina::Utils::ImGuiManager::Initialize(mainWindow.Handle(), device, swapChain, gpuDH);
		WinAppContext_.RegisterCallback(Lumina::Utils::ImGuiManager::WindowProcedure);
		SetImGuiAppearance();
		#endif

		SceneManager::Instance().Initialize();

		OrthoProjMat_ = OrthographicProjection(0.0f, 1280.0f, 0.0f, 720.0f, 0.0f, 1.0f);
		UB_OrthoProjMat_.Initialize(device, 256LLU);
		UB_OrthoProjMat_.Store(&OrthoProjMat_, sizeof(Math::F32x4x4<>), 0LLU);
		LocalHeap_OrthoProjMat_.Initialize(
			device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			1U,
			false,
			"OrthoProjMat"
		);
		D3D12::CBV::Create(device, LocalHeap_OrthoProjMat_.CPUHandle(0U), UB_OrthoProjMat_);

		//	テクスチャ読み込みとシェーダー用SRV作成

		//GlobalTable_ImageTextures_ = gpuDH.Allocate(32U);
		//std::vector<uint32_t> texIDs{};
		//ResourceManager_.Graphics().LoadImageTextures(
		//	texIDs,
		//	{
		//		{ "uvChecker", "Assets/Img/uvChecker.png" },	//	0
		//		{ "CLIMATE", "Assets/Img/CLIMATE.png" },		//	1
		//		{ "OCEAN", "Assets/Img/OCEAN.png" },			//	2
		//	}
		//);
		//for (uint32_t idx{ 0U }; idx < static_cast<uint32_t>(texIDs.size()); ++idx) {
		//	device->CopyDescriptorsSimple(
		//		1U,
		//		GlobalTable_ImageTextures_.CPUHandle(idx),
		//		ResourceManager_.Graphics().CPUHandle(texIDs.at(idx)),
		//		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		//	);
		//}

		MeshManager_ = std::make_unique<MeshManager>();
		MeshManager_->Initialize(D3D12Context_, 1 << 12, 1 << 18);
	}

	void Context::Finalize() {
		SceneManager::Instance().Finalize();

		D3D12Context_.DirectQueue().SignalAndCPUWait();

		#if defined(_DEBUG)
		Lumina::Utils::ImGuiManager::Finalize();
		#endif

		ResourceManager_.Finalize();

		WinAppContext_.Finalize();
	}
}