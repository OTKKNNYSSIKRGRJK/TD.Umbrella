module Game.Scene.Title;

import nlohmann.json;

//import Lumina;

import Lumina.Utils.Data;
import Lumina.Main;
import Lumina.D3D12;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import : Impl;

import Game.MotionManager;
import Game.Player;
import Game.BGMManager;

import Lumina.CG3D;
import Lumina.CG3D.Animation;

import Game.MathUtils;

namespace Game::Scene::Impl {
	template<>
	auto Title::Initialize_<"Meshes">() -> void {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

		auto&& umbrellaHandle{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"UmbrellaHandle.obj", "Assets/Hamada/Umbrella"
				)
			)
		};

		auto&& umbrellaOpenTop{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"UmbrellaTop.obj", "Assets/Hamada/Umbrella"
				)
			)
		};

		using MeshCollection = std::vector<Lumina::Utils::Mesh>;

		// アップロード用vector
		MeshCollection meshesToBeUploaded{};

		// メッシュvectorをアップロードリストに追加
		// 可読性向上させるべくラムダ式に
		auto addMeshesToBeUploaded{
			[&](MeshCollection const& meshCollection_) -> void {
				meshesToBeUploaded.insert(
					meshesToBeUploaded.cend(),
					meshCollection_.cbegin(),
					meshCollection_.cend()
				);
			}
		};

		addMeshesToBeUploaded(umbrellaHandle);
		addMeshesToBeUploaded(umbrellaOpenTop);

		// メッシュデータをGPU側にアップロードするやつ
		Lumina::MeshUploader meshUploader{};
		meshUploader.Initialize(d3d12Context);
		meshUploader.Begin();
		for (auto const& mesh : meshesToBeUploaded) {
			meshUploader.Batch(mesh);
		}
		meshUploader.End(MeshShaderAssets_);
	}

	template<>
	auto Title::Initialize_<"ImageTextures">() -> void {
		// エンジン
		auto& context{ Lumina::Context::Instance() };
		// 画像や音声の読み込みなどを司るやつ
		auto& resMngr{ context.ResourceContext() };
		// D3D12関連
		auto const& d3d12Context{ context.D3D12Context() };
		// D3D12デバイス
		auto const& d3d12Device{ d3d12Context.Device() };

		std::vector<uint32_t> texIDs{};
		resMngr.Graphics().LoadImageTextures(
			texIDs,
			{
				{ "Particles", "Assets/Img/Particles.png" },
				{ "Title.Blank", "Assets/Img/White16x16.png" },
				{ "Title.UI.Caption", "Assets/Img/UI/Title.png" },
				{ "Title.UI.Start", "Assets/Img/UI/Start.png" },
				{ "Title.UI.Exit", "Assets/Img/UI/EXIT.png" },
			}
		);

		// シェーダーで使えるディスクリプタ
		GlobalTable_SRV_ImageTexture_ = d3d12Context.GlobalDescriptorHeap().Allocate(32U);
		for (uint32_t idx{ 0U }; idx < static_cast<uint32_t>(texIDs.size()); ++idx) {
			
			// さき読み込んだテクスチャのSRVをシェーダーで使えるディスクリプタにコピー
			d3d12Device->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_ImageTexture_.CPUHandle(idx),
				resMngr.Graphics().CPUHandle(texIDs.at(idx)),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}
	}
	
	template<>
	auto Title::Initialize_<"MeshMaterials">() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		UB_Materials_.resize(64U);
		for (auto& ub : UB_Materials_) {
			ub = std::make_unique<Lumina::D3D12::UploadBuffer>();
			ub->Initialize(d3d12Device, 256LLU);
		}

		// マテリアル用ディスクリプタヒープ（64個分）
		// シェーダー側には見えないけど、メッシュバッチとともにメッシュマネージャになんとかしてもらう
		LocalHeap_Materials_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64U, false);

		// CBV作成
		Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_Materials_.CPUHandle(0U), *UB_Materials_[0]);
		Material0_.RGBA = { 1.0f, 1.0f, 1.0f, 1.0f };
		Material0_.ID_DiffuseMap = 1U;
		UB_Materials_[0]->Store(&Material0_, sizeof(Material0_), 0LLU);
	}

	template<>
	auto Title::Initialize_<"RenderPipeline">() -> void {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

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
			{
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				DXGI_FORMAT_R8G8B8A8_UNORM,
			},
			Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
		);

		Canvas_Merge_.AllocateTextures(1U, false);
		Canvas_Merge_.RenderTexture(0U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		//Canvas_Merge_.DepthTexture().Initialize(d3d12Device, 1280U, 720U);
		Canvas_Merge_.TransitionResourceStates(d3d12Device, d3d12Context.DirectQueue());
		Canvas_Merge_.CreateViews(d3d12Device);
		Canvas_Merge_.Viewport(0U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 1280.0f },
			.Height{ 720.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_Merge_.ScissorRect(0U) = D3D12_RECT{
			.left{ 0 },
			.top{ 0 },
			.right{ 1280 },
			.bottom{ 720 },
		};

		Canvas_GeometryPass_.AllocateTextures(3U, true);
		Canvas_GeometryPass_.RenderTexture(0U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		Canvas_GeometryPass_.RenderTexture(1U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM);
		Canvas_GeometryPass_.RenderTexture(2U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM);
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

		Lumina::F32 const clearColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
		GeometryPass_.Initialize(3U, true);
		GeometryPass_.RenderTarget(0).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			clearColor
		);
		GeometryPass_.RenderTarget(0).EndingEvent().Preserve();
		GeometryPass_.RenderTarget(1).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			clearColor
		);
		GeometryPass_.RenderTarget(1).EndingEvent().Preserve();
		GeometryPass_.RenderTarget(2).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			clearColor
		);
		GeometryPass_.RenderTarget(2).EndingEvent().Preserve();
		GeometryPass_.DepthStencil().DepthBeginningEvent().ClearTarget(
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			{ .Depth{ 1.0f }, }
		);
		GeometryPass_.DepthStencil().DepthEndingEvent().Preserve();
		GeometryPass_.DepthStencil().StencilBeginningEvent().NoAccess();
		GeometryPass_.DepthStencil().StencilEndingEvent().NoAccess();

		for (uint32_t idx{ 0U }; idx < Canvas_GeometryPass_.Num_RenderTargets(); ++idx) {
			GeometryPass_.RenderTarget(idx).View() = Canvas_GeometryPass_.RTV(idx);
		}
		GeometryPass_.DepthStencil().View() = Canvas_GeometryPass_.DSV();

		MergePass_.Initialize(1U, true);
		MergePass_.RenderTarget(0).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			clearColor
		);
		MergePass_.RenderTarget(0).EndingEvent().Preserve();
		MergePass_.DepthStencil().DepthBeginningEvent().ClearTarget(
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			{ .Depth{ 1.0f }, }
		);
		MergePass_.DepthStencil().DepthEndingEvent().Preserve();
		MergePass_.DepthStencil().StencilBeginningEvent().NoAccess();
		MergePass_.DepthStencil().StencilEndingEvent().NoAccess();
		MergePass_.RenderTarget(0).View() = Canvas_Merge_.RTV(0);

		PrimitiveManager_ = std::make_unique<Lumina::PrimitiveManager>();
		PrimitiveManager_->Initialize(d3d12Context);
		PrimitiveManager2_ = std::make_unique<Lumina::PrimitiveManager>();
		PrimitiveManager2_->Initialize(d3d12Context,
			L"Assets/Shaders/Primitive.VS.hlsl",
			L"Assets/Shaders/Primitive.PS.hlsl",
			false,
			true,
			16
		);

		GlobalTable_SRV_CanvasTexture_ = d3d12Context.GlobalDescriptorHeap().Allocate(8U);
		Lumina::D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_SRV_CanvasTexture_.CPUHandle(0U),
			Canvas_GeometryPass_.RenderTexture(0U)
		);
		Lumina::D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_SRV_CanvasTexture_.CPUHandle(1U),
			Canvas_GeometryPass_.RenderTexture(1U)
		);
		Lumina::D3D12::SRV<void>::Create<DXGI_FORMAT_R24_UNORM_X8_TYPELESS>(
			d3d12Device,
			GlobalTable_SRV_CanvasTexture_.CPUHandle(3U),
			Canvas_GeometryPass_.DepthTexture()
		);

		GlobalTable_SRV_MergeTexture_ = d3d12Context.GlobalDescriptorHeap().Allocate(1U);
		Lumina::D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_SRV_MergeTexture_.CPUHandle(0U),
			Canvas_Merge_.RenderTexture(0U)
		);

		GlobalTable_SRV_GBufferForWaterColor_ = d3d12Context.GlobalDescriptorHeap().Allocate(3U);
		Lumina::D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_SRV_GBufferForWaterColor_.CPUHandle(0U),
			Canvas_Merge_.RenderTexture(0U)
		);
		Lumina::D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_SRV_GBufferForWaterColor_.CPUHandle(1U),
			Canvas_GeometryPass_.RenderTexture(2U)
		);
		Lumina::D3D12::SRV<void>::Create<DXGI_FORMAT_R24_UNORM_X8_TYPELESS>(
			d3d12Device,
			GlobalTable_SRV_GBufferForWaterColor_.CPUHandle(2U),
			Canvas_GeometryPass_.DepthTexture()
		);

		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		std::vector<D3D12_RESOURCE_BARRIER> const barriers{
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(1U),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_Merge_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
		};
		cmdList->ResourceBarrier(
			static_cast<Lumina::U32>(barriers.size()),
			barriers.data()
		);
	}

	template<>
	auto Title::Initialize_<"Camera">() -> void {
		Camera_ = std::make_unique<Lumina::Utils::Camera>();
		Camera_->LookAt({ 5.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
		Camera_->Perspective(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
	}

	template<>
	auto Title::Initialize_<"Resource, View">() -> void {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

		WorldToHomogeneous_ = std::make_unique<Lumina::Math::F32x4x4<>>();
		*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
		ScreenToWorld_ = std::make_unique<Lumina::Math::F32x4x4<>>();
		UB_Transforms_.Initialize(d3d12Device, 256LLU);
		GlobalTable_CBV_Scene_ = d3d12Context.GlobalDescriptorHeap().Allocate(1U);
		Lumina::D3D12::CBV::Create(d3d12Device, GlobalTable_CBV_Scene_.CPUHandle(0U), UB_Transforms_);
	}

	template<>
	auto Title::Initialize_<"Watercolor">() -> void {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

		Watercolor_ = std::make_unique<Lumina::Watercolor>();
		Watercolor_->Initialize();
	}

	template<>
	auto Title::Initialize_<"Grassland">() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };

		Grassland_ = std::make_unique<Lumina::Grassland>();
		Grassland_->Initialize(d3d12Context, 640U, 320U);
	}

	template<>
	auto Title::Initialize_<"Lighting">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		DeferredLighting_ = std::make_unique<Lumina::DeferredLighting>();
		DeferredLighting_->Initialize(d3d12Context_, 1280U, 720U);

		List_PointLight_.Initialize(1024U);
		List_LocalToWorld_LightSphere_.Initialize(1024U);

		GlobalTable_SRV_LightingResultTexture_ = d3d12Context_.GlobalDescriptorHeap().Allocate(1U);
		Lumina::D3D12::SRV<void>::Create(
			d3d12Context_.Device(),
			GlobalTable_SRV_LightingResultTexture_.CPUHandle(0U),
			DeferredLighting_->RenderTexture()
		);

		// * 定数バッファ初期化

		UB_WorldToProjective_.Initialize(d3d12Device_, 256LLU);
		UB_ScreenToWorld_.Initialize(d3d12Device_, 256LLU);

		LocalHeap_Scene_.Initialize(d3d12Device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16U, false);
		Lumina::D3D12::CBV::Create(d3d12Device_, LocalHeap_Scene_.CPUHandle(0U), UB_WorldToProjective_);
		Lumina::D3D12::CBV::Create(d3d12Device_, LocalHeap_Scene_.CPUHandle(1U), UB_ScreenToWorld_);
	}

	template<>
	auto Title::Initialize_<"Particles">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {

		// * パイプライン初期化

		auto config_ParticleSystem{
			Lumina::Utils::LoadFromFile<nlohmann::json>(
				"Assets/Configs/ParticleSystem.json"
			)
		};
		RS_ParticleSystem_.Initialize(
			d3d12Device_,
			Lumina::D3D12::LoadSetup<Lumina::D3D12::RootSignature>(
				config_ParticleSystem.at("Common RS")
			)
		);

		d3d12Context_.Compile(
			VS_BasicParticle_,
			L"Assets/Shaders/Particle2.VS.hlsl",
			L"vs_6_6",
			L"main",
			"Particle2.VS"
		);
		d3d12Context_.Compile(
			PS_BasicParticle_,
			L"Assets/Shaders/Particle2.PS.hlsl",
			L"ps_6_6",
			L"main",
			"Particle2.PS"
		);

		Lumina::D3D12::BlendState blendState_AdditiveMode{};
		blendState_AdditiveMode.RenderTarget[0] = {
			.BlendEnable{ true },
			.SrcBlend{ D3D12_BLEND_SRC_ALPHA },
			.DestBlend{ D3D12_BLEND_ONE },
			.BlendOp{ D3D12_BLEND_OP_ADD },
			.SrcBlendAlpha{ D3D12_BLEND_SRC_ALPHA },
			.DestBlendAlpha{ D3D12_BLEND_ONE },
			.BlendOpAlpha{ D3D12_BLEND_OP_ADD },
			.RenderTargetWriteMask{ D3D12_COLOR_WRITE_ENABLE_ALL },
		};

		Lumina::D3D12::GraphicsPSO::InputLayout inputLayout_Particle{};
		inputLayout_Particle.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT);
		inputLayout_Particle.Append("TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT);
		GraphicsPSO_BasicParticle_AdditiveMode_.Initialize(
			d3d12Device_,
			RS_ParticleSystem_,
			VS_BasicParticle_,
			PS_BasicParticle_,
			blendState_AdditiveMode,
			Lumina::D3D12::RasterizerState{
				.FillMode{ D3D12_FILL_MODE_SOLID },
				.CullMode{ D3D12_CULL_MODE_NONE },
			},
			Lumina::D3D12::DepthStencilState{
				.DepthEnable{ false },
				//.DepthEnable{ true },
				//.DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ZERO },
				//.DepthFunc{ D3D12_COMPARISON_FUNC_LESS_EQUAL },
				.StencilEnable{ false },
			},
			inputLayout_Particle,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			{
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				DXGI_FORMAT_R8G8B8A8_UNORM,
			},
			Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
		);

		// * レンダラ

		AmbientSparkles_ = std::make_unique<Lumina::ParticleSystem<Lumina::Particle>>();
		AmbientSparkles_->Initialize(d3d12Context_, 384U);
		Raindrops_ = std::make_unique<Lumina::ParticleSystem<Lumina::Particle>>();
		Raindrops_->Initialize(d3d12Context_, 1024U);

		UmbrellaEffects_ = std::make_unique<Lumina::ParticleSystem<Lumina::Particle>>();
		UmbrellaEffects_->Initialize(d3d12Context_, 512U);
	}

	template<>
	auto Title::Initialize_<"Skybox">() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		Skybox_ = std::make_unique<Lumina::Skybox>();
		Skybox_->Initialize(d3d12Context, d3d12Device, "Assets/Img/Skybox.dds");
	}

	template<>
	auto Title::Initialize_<"UI">() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		SpriteRenderer_.reset(new Lumina::SpriteRenderer{});
		SpriteRenderer_->Initialize(d3d12Context, 128U);

		// * Pipeline
		{
			d3d12Context.Compile(
				VS_SpriteUI_,
				L"Assets/Shaders/Sprite2.VS.hlsl",
				L"vs_6_6",
				L"main",
				"SpriteUI.VS"
			);
			d3d12Context.Compile(
				PS_SpriteUI_,
				L"Assets/Shaders/SpriteUI.PS.hlsl",
				L"ps_6_6",
				L"main",
				"SpriteUI.PS"
			);

			Lumina::D3D12::BlendState spriteBlendState{};
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
			Lumina::D3D12::GraphicsPSO::InputLayout spriteInputLayout{};
			spriteInputLayout.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT);
			PSO_SpriteUI_.Initialize(
				d3d12Device,
				SpriteRenderer_->RootSignature(),
				VS_SpriteUI_,
				PS_SpriteUI_,
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
		}

		// * Caption
		{
			TitleCaption_.Translate = { 50.0f, 50.0f };
			TitleCaption_.Scale = { 1280.0f * 0.6f, 430.0f * 0.6f };
			TitleCaption_.AnchorPoint = { 0.0f, 0.0f };
			TitleCaption_.TextureID = 2U;
			TitleCaption_.RGBA = { 0.95f, 0.95f, 0.95f, 0.0f };
		}
		// * Start Button
		{
			UI_StartButton_.Translate = { 1400.0f, 535.0f };
			UI_StartButton_.Scale = { 1280.0f * 0.2f, 450.0f * 0.2f };
			UI_StartButton_.AnchorPoint = { 1.0f, 0.5f };
			UI_StartButton_.TextureID = 3U;
			UI_StartButton_.RGBA ={ 0.99f, 0.98f, 0.97f, 0.0f };
		}
		// * Exit Button
		{
			UI_ExitButton_.Translate = { 1400.0f, 610.0f };
			UI_ExitButton_.Scale = { 1280.0f * 0.2f, 450.0f * 0.2f };
			UI_ExitButton_.AnchorPoint = { 1.0f, 0.5f };
			UI_ExitButton_.TextureID = 4U;
			UI_ExitButton_.RGBA = { 0.99f, 0.98f, 0.97f, 0.0f };
		}

		SelectedButton_ = 0;
		auto const ortho{ Game::MathUtils::Orthographic(0.0f, 1280.0f, 0.0f, 720.0f, 0.0f, 1.0f) };
		UB_OrthoProj_.Initialize(d3d12Device, 256LLU, "OrthoProj");
		UB_OrthoProj_.Store(&ortho, sizeof(Lumina::Math::F32x4x4<>), 0LLU);
		LocalHeap_OrthoProj_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1U, false);
		Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_OrthoProj_.CPUHandle(0U), UB_OrthoProj_);
		
		UITimer_ = 0;
		UITimer2_ = 0;
	}

	void Title::Initialize() {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		Initialize_<"Meshes">();
		Initialize_<"ImageTextures">();
		Initialize_<"MeshMaterials">();
		Initialize_<"RenderPipeline">();
		Initialize_<"Camera">();
		Initialize_<"Resource, View">();
		Initialize_<"Watercolor">();
		Initialize_<"Grassland">();
		Initialize_<"Skybox">();

		Initialize_<"Lighting">(d3d12Context, d3d12Device);
		Initialize_<"Particles">(d3d12Context, d3d12Device);
		Initialize_<"UI">();

		RootWorldPos_ = { 0.0f, 2.0f, 0.0f };
		UmbrellaRotation_ = { 0.0f, 0.0f, 0.0f };
		UmbrellaRootWorld_ = std::make_unique<Lumina::Math::F32x4x4<>>();
		UmbrellaTipWorld_ = std::make_unique<Lumina::Math::F32x4x4<>>();

		Game::BGMManager::GetInstance()->PlaySceneBGM("Title");
	}

	Title::Title() = default;
	Title::~Title() = default;
}

namespace Game::Scene {
	template<>
	void Title::Initialize() {
		Impl_ = std::make_unique<Impl::Title>();
		Impl_->Initialize();
	}

	Title::Title() { Initialize(); }
	Title::~Title() = default;
}