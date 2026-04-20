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

import Lumina.CG3D;
import Lumina.CG3D.Animation;

namespace Game::Scene::Impl {
	auto Title::LoadMeshes() -> void {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

		// assimpを使っての読み込み
		// メッシュはLumina::CG3D::Collectionの中のMeshesに入ってる
		Collection_ = Lumina::CG3D::Import("Neki.gltf", "Assets/Neki");
		// メッシュの頂点バッファ
		VertexBuffer_.Initialize(
			d3d12Device,
			// バッファサイズ＝頂点サイズ×メッシュの頂点数
			sizeof(Lumina::CG3D::Mesh::Vertex) *
			Collection_.Meshes[0].Vertices.size()
		);
		// 頂点バッファに頂点データを入れる
		VertexBuffer_.Store(
			// データ
			Collection_.Meshes[0].Vertices.data(),
			// データサイズ
			sizeof(Lumina::CG3D::Mesh::Vertex) *
			Collection_.Meshes[0].Vertices.size(),
			// メモリオフセット　気にせんでええ
			0LLU
		);
		// 頂点バッファを使ってビューを作成
		// テンプレートに頂点の変数型を入れる
		VBV_ = Lumina::D3D12::VBV::Create<Lumina::CG3D::Mesh::Vertex>(VertexBuffer_);
	}

	auto Title::LoadImageTextures() -> void {
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
				//{ 適当な名前（重複しちゃダメ）, ファイルパス },
				
				// uvCheckerは1番目に読み込まれるだからIDは0
				{ "uvChecker", "Assets/Img/uvChecker.png" },
				// Diff2は2番目だからIDは1
				{ "Diff2", "Assets/Img/Diff2.png" },
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

	auto Title::InitializeMeshMaterials() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		// とりあえず64個分のアップロードバッファを確保する
		UB_Materials_.resize(64U);
		for (auto& ub : UB_Materials_) {
			ub = std::make_unique<Lumina::D3D12::UploadBuffer>();
			ub->Initialize(d3d12Device, 256LLU);
		}

		GlobalTable_Materials_ = d3d12Context.GlobalDescriptorHeap().Allocate(32U);

		// CBV作成
		// --- パラメータ ---
		// GraphicsDevice const& device_ : D3D12デバイス
		// D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_ : マテリアル用ディスクリプタのCPUハンドル
		// BufferType const& buffer_ : マテリアル用バッファ
		Lumina::D3D12::CBV::Create(d3d12Device, GlobalTable_Materials_.CPUHandle(0U), *UB_Materials_[0]);

		// アップデートでマテリアルをいじったりするのであれば下記のように書くとよろし
		// マテリアルデータを更新
		Material0_.RGBA = { 1.0f, 1.0f, 1.0f, 1.0f };
		Material0_.ID_DiffuseMap = 0;

		// マテリアルデータをCBVと紐づけてあるバッファに格納
		// --- パラメータ ---
		// void const* src_ : 格納されるデータへのポインター。ボイドポインター最強
		// uint64_t sizeInBytes_ : 格納されるサイズ。ここは構造体のサイズで大丈夫
		// uint64_t offsetInBytes_: バッファ先頭からのオフセット。ここは0で大丈夫
		UB_Materials_[0]->Store(&Material0_, sizeof(Material0_), 0LLU);
	}

	auto Title::InitializeRenderPipeline() -> void {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

		auto config{ Lumina::Utils::LoadFromFile<nlohmann::json>("Assets/Configs/SkinnedMesh.json") };
		auto&& rsSetup{ Lumina::D3D12::LoadSetup<Lumina::D3D12::RootSignature>(config.at("RS")) };
		RS_Skinning_.Initialize(d3d12Device, rsSetup);

		d3d12Context.Compile(
			VS_SkinnedMeshDeferredGeometry_,
			L"Assets/Shaders/MeshSkinning.VS.hlsl",
			L"vs_6_6",
			L"main",
			"SkinnedMesh.DeferredGeometry.VS"
		);
		d3d12Context.Compile(
			PS_SkinnedMeshDeferredGeometry_,
			L"Assets/Shaders/MeshSkinning.PS.hlsl",
			L"ps_6_6",
			L"main",
			"SkinnedMesh.DeferredGeometry.PS"
		);

		Lumina::D3D12::BlendState blendState_None{};
		blendState_None.RenderTarget[0].BlendEnable = false;
		blendState_None.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendState_None.RenderTarget[1].BlendEnable = false;
		blendState_None.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		Lumina::D3D12::GraphicsPSO::InputLayout inputLayout_Mesh{};
		inputLayout_Mesh.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout_Mesh.Append("TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT);
		inputLayout_Mesh.Append("NORMAL", 0U, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout_Mesh.Append("WEIGHT", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT);
		inputLayout_Mesh.Append("PALETTE", 0U, DXGI_FORMAT_R32G32B32A32_SINT);

		GraphicsPSO_SkinnedMeshDeferredGeometry_.Initialize(
			d3d12Device,
			RS_Skinning_,
			VS_SkinnedMeshDeferredGeometry_,
			PS_SkinnedMeshDeferredGeometry_,
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

		Lumina::F32 const clearColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
		GeometryPass_.Initialize(2U, true);
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
		MergePass_.DepthStencil().DepthEndingEvent().Preserve();
		MergePass_.DepthStencil().StencilBeginningEvent().NoAccess();
		MergePass_.DepthStencil().StencilEndingEvent().NoAccess();

		PrimitiveManager_ = std::make_unique<Lumina::PrimitiveManager>();
		PrimitiveManager_->Initialize(d3d12Context);

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
	}

	void Title::Initialize() {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

		LoadMeshes();
		LoadImageTextures();
		InitializeMeshMaterials();

		// アニメーション関連

		// ファイル名変えないとエラー出ちゃう
		auto animations{ Lumina::CG3D::LoadAnimationFile("Neki.gltf", "Assets/Neki") };
		Animation_ = animations[0];
		Skeleton_ = Lumina::CG3D::CreateSkeleton(Collection_.Root);
		Lumina::CG3D::CreateSkinCluster(
			SkinCluster_,
			d3d12Device,
			d3d12Context.GlobalDescriptorHeap(),
			Skeleton_,
			// メッシュ
			Collection_.Meshes[0]
		);

		Camera_ = std::make_unique<Lumina::Utils::Camera>();
		Camera_->LookAt({ 0.0f, 0.0f, -30.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
		Camera_->Perspective(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);

		WorldToHomogeneous_ = std::make_unique<Lumina::Math::F32x4x4<>>();
		*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
		UB_Transforms_.Initialize(d3d12Device, 256LLU);
		GlobalTable_CBV_Scene_ = d3d12Context.GlobalDescriptorHeap().Allocate(1U);
		Lumina::D3D12::CBV::Create(d3d12Device, GlobalTable_CBV_Scene_.CPUHandle(0U), UB_Transforms_);

		InitializeRenderPipeline();

		AnimationTimer_ = 0.0f;

		MeshScale_ = { 1.0f, 1.0f, 1.0f };
		MeshRotate_ = { 0.0f, 0.0f, 0.0f };
		MeshTranslate_ = { 0.0f, 0.0f, 0.0f };
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