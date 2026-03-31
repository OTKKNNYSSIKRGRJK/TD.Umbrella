module Game.Scene.InGame;

import <vector>;

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

namespace Game::Scene::Impl {

	// テクスチャ読み込み
	auto InGame::LoadImageTextures() -> void {
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

	// メッシュ読み込み
	auto InGame::LoadMeshes() -> void {
		auto const& d3d12Context{ Lumina::Context::Instance().D3D12Context() };

		// マルチメッシュ対応なのでstd::vector<Lumina::Utils::Mesh>形式に
		// Lumina::Utils::Meshにはメッシュ1個分が入る
		auto&& teapot{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"teapot.obj", "Assets"
				)
			)
		};

		using MeshCollection = std::vector<Lumina::Utils::Mesh>;
		
		// アップロード用vector
		MeshCollection meshesToBeUploaded{};

		// メッシュvectorをアップロードリストに追加
		// 可読性向上させるべくラムダ式に
		auto addMeshesToBeUploaded{
			[&] (MeshCollection const& meshCollection_) -> void {
				meshesToBeUploaded.insert(
					meshesToBeUploaded.cend(),
					meshCollection_.cbegin(),
					meshCollection_.cend()
				);
			}
		};

		addMeshesToBeUploaded(teapot);

		// メッシュデータをGPU側にアップロードするやつ
		Lumina::MeshUploader meshUploader{};
		meshUploader.Initialize(d3d12Context);
		meshUploader.Begin();
		for (auto const& mesh : meshesToBeUploaded) {
			meshUploader.Batch(mesh);
		}
		meshUploader.End(MeshShaderAssets_);

		// MeshShaderAssets_ : Lumina::MeshShaderAssetが入ってる
		// Lumina::MeshShaderAsset : バッファとかいろいろシェーダーが使えるやつが入ってて、描画の際にLumina::MeshManagerに渡す
	}
	
	// シェーダーにマテリアルを使ってもらうにはバッファとビューが必要だから
	// ここでこいつらの下ごしらえを
	auto InGame::InitializeMeshMaterials() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		// とりあえず64個分のアップロードバッファを確保する
		UB_Materials_.resize(64U);
		for (auto& ub : UB_Materials_) {
			ub = std::make_unique<Lumina::D3D12::UploadBuffer>();
			ub->Initialize(d3d12Device, 256LLU);
		}

		// マテリアル用ディスクリプタヒープ（64個分）
		// シェーダー側には見えないけど、メッシュバッチとともにメッシュマネージャになんとかしてもらう
		LocalHeap_Materials_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64U, false);
		
		// CBV作成
		// --- パラメータ ---
		// GraphicsDevice const& device_ : D3D12デバイス
		// D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_ : マテリアル用ディスクリプタのCPUハンドル
		// BufferType const& buffer_ : マテリアル用バッファ
		Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_Materials_.CPUHandle(0U), *UB_Materials_[0]);

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

	void InGame::Initialize() {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		MotionManager::GetInstance()->LoadMotions("Assets/Data/Motion/");
		TerrainEditor_ = std::make_unique<TerrainEditor>();
		TerrainEditor_->Initialize();

		LoadImageTextures();
		LoadMeshes();
		InitializeMeshMaterials();

		Camera_ = std::make_unique<Lumina::Utils::Camera>();
		Camera_->LookAt({ 0.0f, 0.0f, -30.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
		Camera_->Perspective(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
		WorldToHomogeneous_ = std::make_unique<Lumina::Math::F32x4x4<>>();
		*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
		UB_WorldToHomogeneous_.Initialize(d3d12Device, 256LLU);
		UB_WorldToHomogeneous_.Store(*WorldToHomogeneous_, sizeof(Lumina::Math::F32x4x4<>), 0LLU);
		LocalHeap_Scene_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16U, false);
		Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_Scene_.CPUHandle(0U), UB_WorldToHomogeneous_);

		//////	//////	//////	//////	//////	//////	//////

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

		auto&& terrainScreenPos{ std::make_unique<TerrainShapeCollection>() };
		terrainScreenPos = std::make_unique<TerrainShapeCollection>();
		terrainScreenPos->Initialize(
			Lumina::Utils::LoadFromFile<nlohmann::json>(
				"zxcv.json", "Assets/Data/Terrain"
			)
		);
		Terrain_ = std::make_unique<TerrainShapeCollection>();
		terrainScreenPos->ConvertToWorldCoordinate(
			*Terrain_,
			*Camera_,
			{ 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f }
		);
		TerrainRenderer_ = std::make_unique<TerrainRenderer>();
		TerrainRenderer_->Initialize();

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

		Player_ = std::make_unique<Player>();
		Player_->Initialize();
		Player_->SetMesh(MeshShaderAssets_[0]);
		Player_->SetMeshMaterialCBV(LocalHeap_Materials_.CPUHandle(0U));
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