module Game.Scene.InGame;

import <vector>;

import <d3d12.h>;

import nlohmann.json;

import Lumina;

import Lumina.Utils.Data;

import : Impl;

import MotionManager;

namespace Game::Scene::Impl {
	namespace {
	}

	template<>
	void InGame::Initialize() {
		MotionManager::GetInstance()->LoadMotions("Assets/Data/Motion/");
		Test_ = std::make_unique<Game::CharacterTest>();
		Test_->Initialize();


		auto&& bossModel{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"Kinoko.obj", "Assets/Kinoko"
				)
			)
		};

		auto const& d3d12Context{ Lumina::Context::Instance().D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		std::vector<Lumina::Utils::Mesh> meshes{};
		meshes.insert(meshes.cend(), bossModel.cbegin(), bossModel.cend());
		Lumina::MeshUploader meshUploader{};
		meshUploader.Initialize(d3d12Context);
		meshUploader.Begin();
		for (auto const& mesh : meshes) {
			meshUploader.Batch(mesh);
		}
		meshUploader.End(MeshShaderAssets_);

		d3d12Context.Compile(
			VS_MeshDeferredGeometry_,
			L"Assets/Shaders/MeshCommon.VS.hlsl",
			L"vs_6_6",
			L"main",
			"Mesh.DeferredGeometry.VS"
		);
		d3d12Context.Compile(
			PS_MeshDeferredGeometry_,
			L"Assets/Shaders/MeshCommon.PS.hlsl",
			L"ps_6_6",
			L"main",
			"Mesh.DeferredGeometry.PS"
		);

		Lumina::D3D12::BlendState blendState_None{};
		blendState_None.RenderTarget[0].BlendEnable = false;
		blendState_None.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendState_None.RenderTarget[1].BlendEnable = false;
		blendState_None.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		Lumina::D3D12::GraphicsPSO::InputLayout inputLayout_Mesh{};
		inputLayout_Mesh.Append("IDX_POSITION", 0U, DXGI_FORMAT_R32_UINT);
		inputLayout_Mesh.Append("IDX_TEXCOORD", 0U, DXGI_FORMAT_R32_UINT);
		inputLayout_Mesh.Append("IDX_NORMAL", 0U, DXGI_FORMAT_R32_UINT);
		inputLayout_Mesh.Append("IDX_TANGENT", 0U, DXGI_FORMAT_R32_UINT);

		GraphicsPSO_MeshDeferredGeometry_.Initialize(
			d3d12Device,
			Lumina::Context::Instance().MeshContext().RootSignature(),
			VS_MeshDeferredGeometry_,
			PS_MeshDeferredGeometry_,
			blendState_None,
			Lumina::D3D12::RasterizerState{
				.FillMode{ D3D12_FILL_MODE_SOLID },
				.CullMode{ D3D12_CULL_MODE_BACK },
			},
			Lumina::D3D12::DepthStencilState{
				.DepthEnable{ true },
				.DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ALL },
				.DepthFunc{ D3D12_COMPARISON_FUNC_LESS_EQUAL },
				.StencilEnable{ false },
			},
			inputLayout_Mesh,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM, },
			Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
		);

		Canvas_.AllocateTextures(2U, true);
		Canvas_.RenderTexture(0U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		Canvas_.RenderTexture(1U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM);
		Canvas_.DepthTexture().Initialize(d3d12Device, 1280U, 720U);
		Canvas_.TransitionResourceStates(d3d12Device, d3d12Context.DirectQueue());
		Canvas_.CreateViews(d3d12Device);
		Canvas_.Viewport(0U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 1280.0f },
			.Height{ 720.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_.ScissorRect(0U) = D3D12_RECT{
			.left{ 0 },
			.top{ 0 },
			.right{ 1280 },
			.bottom{ 720 },
		};
		Canvas_.Viewport(1U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 0.0f },
			.Height{ 0.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_.ScissorRect(1U) = D3D12_RECT{
			.left{ 640 },
			.top{ 360 },
			.right{ 1280 },
			.bottom{ 720 },
		};

		Canvas_GeometryPass_.AllocateTextures(2U, true);
		Canvas_GeometryPass_.RenderTexture(0U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		Canvas_GeometryPass_.RenderTexture(1U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM);
		Canvas_GeometryPass_.DepthTexture().Initialize(d3d12Device, 1280U, 720U);
		Canvas_GeometryPass_.TransitionResourceStates(d3d12Device, d3d12Context.DirectQueue());
		Canvas_GeometryPass_.CreateViews(d3d12Device);
		Canvas_GeometryPass_.Viewport(0U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 1280.0f },
			.Height{ 720.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_GeometryPass_.ScissorRect(0U) = D3D12_RECT{
			.left{ 0 },
			.top{ 0 },
			.right{ 1280 },
			.bottom{ 720 },
		};
		Canvas_GeometryPass_.Viewport(1U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 0.0f },
			.Height{ 0.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_GeometryPass_.ScissorRect(1U) = D3D12_RECT{
			.left{ 640 },
			.top{ 360 },
			.right{ 1280 },
			.bottom{ 720 },
		};

		GeometryPass_ = std::make_unique<Lumina::D3D12::RenderPass>();
		Lumina::F32 const clearColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
		GeometryPass_->Initialize(2U, true);
		GeometryPass_->RenderTarget(0).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			clearColor
		);
		GeometryPass_->RenderTarget(0).EndingEvent().Preserve();
		GeometryPass_->RenderTarget(1).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			clearColor
		);
		GeometryPass_->RenderTarget(1).EndingEvent().Preserve();
		GeometryPass_->DepthStencil().DepthBeginningEvent().ClearTarget(
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			{ .Depth{ 1.0f }, }
		);
		GeometryPass_->DepthStencil().DepthEndingEvent().Preserve();
		GeometryPass_->DepthStencil().StencilBeginningEvent().NoAccess();
		GeometryPass_->DepthStencil().StencilEndingEvent().NoAccess();

		for (uint32_t idx{ 0U }; idx < Canvas_GeometryPass_.Num_RenderTargets(); ++idx) {
			GeometryPass_->RenderTarget(idx).View() = Canvas_GeometryPass_.RTV(idx);
		}
		GeometryPass_->DepthStencil().View() = Canvas_GeometryPass_.DSV();

		TerrainEditor_ = std::make_unique<TerrainEditor>();
		TerrainEditor_->Initialize();
	}

	InGame::InGame() = default;
	InGame::~InGame() = default;
}

namespace Game::Scene {
	template<>
	void InGame::Initialize() {
		Impl_ = std::make_unique<Impl::InGame>();
		Impl_->Initialize();
	}

	InGame::InGame() { Initialize(); }
	InGame::~InGame() = default;
}