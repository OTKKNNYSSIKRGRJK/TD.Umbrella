module Game.CharacterTest;

import Lumina;

import Lumina.Utils.ImGui;

namespace Game {
	namespace {
		Lumina::Math::F32x4x4<> LookAt(
			Lumina::Math::F32x3 const& src_,
			Lumina::Math::F32x3 const& dst_,
			Lumina::Math::F32x3 const& up_
		) {
			Lumina::Math::F32x3 const forward{ Lumina::Math::F32x3{ dst_ - src_ }.Unit() };
			Lumina::Math::F32x3 const right{ Lumina::Math::F32x3::Cross(up_, forward).Unit() };
			Lumina::Math::F32x3 const up{ Lumina::Math::F32x3::Cross(forward, right) };

			return {
				right.X,
				up.X,
				forward.X,
				0.0f,

				right.Y,
				up.Y,
				forward.Y,
				0.0f,

				right.Z,
				up.Z,
				forward.Z,
				0.0f,

				-Lumina::Math::F32x3::Dot(src_, right),
				-Lumina::Math::F32x3::Dot(src_, up),
				-Lumina::Math::F32x3::Dot(src_, forward),
				1.0f,
			};
		}

		auto SRT(
			Lumina::Math::F32x3 const& scale_,
			Lumina::Math::F32x3 const& rotate_,
			Lumina::Math::F32x3 const& translate_
		) -> Lumina::Math::F32x4x4<> {
			Lumina::F32 const
				cosAlpha{ Lumina::Math::COS(rotate_.X) },
				sinAlpha{ Lumina::Math::SIN(rotate_.X) },
				cosBeta{ Lumina::Math::COS(rotate_.Y) },
				sinBeta{ Lumina::Math::SIN(rotate_.Y) },
				cosGamma{ Lumina::Math::COS(rotate_.Z) },
				sinGamma{ Lumina::Math::SIN(rotate_.Z) };

			Lumina::Math::F32x4x4<> srt{
				cosBeta * cosGamma,
				cosBeta * sinGamma,
				-sinBeta,
				0.0f,
				sinAlpha * sinBeta * cosGamma - cosAlpha * sinGamma,
				sinAlpha * sinBeta * sinGamma + cosAlpha * cosGamma,
				sinAlpha * cosBeta,
				0.0f,
				cosAlpha * sinBeta * cosGamma + sinAlpha * sinGamma,
				cosAlpha * sinBeta * sinGamma - sinAlpha * cosGamma,
				cosAlpha * cosBeta,
				0.0f,
				translate_.X,
				translate_.Y,
				translate_.Z,
				1.0f,
			};

			srt[0] *= scale_.X;
			srt[1] *= scale_.Y;
			srt[2] *= scale_.Z;

			return srt;
		}
	}

	auto PerspectiveFOV(float fovY_, float aspectRatio_, float nearClip_, float farClip_) {
		float const cotTheta{ 1.0f / std::tanf(fovY_ * 0.5f) };
		float const inv_FrustumHeight{ 1.0f / (farClip_ - nearClip_) };
		return Lumina::Math::F32x4x4<>{
			(1.0f / aspectRatio_) * cotTheta, 0.0f, 0.0f, 0.0f,
			0.0f, cotTheta, 0.0f, 0.0f,
			0.0f, 0.0f, farClip_ * inv_FrustumHeight, 1.0f,
			0.0f, 0.0f, -nearClip_ * farClip_ * inv_FrustumHeight, 0.0f,
		};
	}

	auto Invert(Lumina::Math::F32x4x4<>& dst_, Lumina::Math::F32x4x4<>& src_) -> void {
		float const inv_Det{ 1.0f / src_.Determinant() };

		dst_[0][0] =
			inv_Det * (
				src_[1][1] * src_[2][2] * src_[3][3] +
				src_[1][2] * src_[2][3] * src_[3][1] +
				src_[1][3] * src_[2][1] * src_[3][2] -
				src_[1][3] * src_[2][2] * src_[3][1] -
				src_[1][2] * src_[2][1] * src_[3][3] -
				src_[1][1] * src_[2][3] * src_[3][2]
			);
		dst_[0][1] =
			inv_Det * (
				src_[0][3] * src_[2][2] * src_[3][1] +
				src_[0][2] * src_[2][1] * src_[3][3] +
				src_[0][1] * src_[2][3] * src_[3][2] -
				src_[0][1] * src_[2][2] * src_[3][3] -
				src_[0][2] * src_[2][3] * src_[3][1] -
				src_[0][3] * src_[2][1] * src_[3][2]
			);
		dst_[0][2] =
			inv_Det * (
				src_[0][1] * src_[1][2] * src_[3][3] +
				src_[0][2] * src_[1][3] * src_[3][1] +
				src_[0][3] * src_[1][1] * src_[3][2] -
				src_[0][3] * src_[1][2] * src_[3][1] -
				src_[0][2] * src_[1][1] * src_[3][3] -
				src_[0][1] * src_[1][3] * src_[3][2]
			);
		dst_[0][3] =
			inv_Det * (
				src_[0][3] * src_[1][2] * src_[2][1] +
				src_[0][2] * src_[1][1] * src_[2][3] +
				src_[0][1] * src_[1][3] * src_[2][2] -
				src_[0][1] * src_[1][2] * src_[2][3] -
				src_[0][2] * src_[1][3] * src_[2][1] -
				src_[0][3] * src_[1][1] * src_[2][2]
			);

		dst_[1][0] =
			inv_Det * (
				src_[1][3] * src_[2][2] * src_[3][0] +
				src_[1][2] * src_[2][0] * src_[3][3] +
				src_[1][0] * src_[2][3] * src_[3][2] -
				src_[1][0] * src_[2][2] * src_[3][3] -
				src_[1][2] * src_[2][3] * src_[3][0] -
				src_[1][3] * src_[2][0] * src_[3][2]
			);
		dst_[1][1] =
			inv_Det * (
				src_[0][0] * src_[2][2] * src_[3][3] +
				src_[0][2] * src_[2][3] * src_[3][0] +
				src_[0][3] * src_[2][0] * src_[3][2] -
				src_[0][3] * src_[2][2] * src_[3][0] -
				src_[0][2] * src_[2][0] * src_[3][3] -
				src_[0][0] * src_[2][3] * src_[3][2]
			);
		dst_[1][2] =
			inv_Det * (
				src_[0][3] * src_[1][2] * src_[3][0] +
				src_[0][2] * src_[1][0] * src_[3][3] +
				src_[0][0] * src_[1][3] * src_[3][2] -
				src_[0][0] * src_[1][2] * src_[3][3] -
				src_[0][2] * src_[1][3] * src_[3][0] -
				src_[0][3] * src_[1][0] * src_[3][2]
			);
		dst_[1][3] =
			inv_Det * (
				src_[0][0] * src_[1][2] * src_[2][3] +
				src_[0][2] * src_[1][3] * src_[2][0] +
				src_[0][3] * src_[1][0] * src_[2][2] -
				src_[0][3] * src_[1][2] * src_[2][0] -
				src_[0][2] * src_[1][0] * src_[2][3] -
				src_[0][0] * src_[1][3] * src_[2][2]
			);

		dst_[2][0] =
			inv_Det * (
				src_[1][0] * src_[2][1] * src_[3][3] +
				src_[1][1] * src_[2][3] * src_[3][0] +
				src_[1][3] * src_[2][0] * src_[3][1] -
				src_[1][3] * src_[2][1] * src_[3][0] -
				src_[1][1] * src_[2][0] * src_[3][3] -
				src_[1][0] * src_[2][3] * src_[3][1]
			);
		dst_[2][1] =
			inv_Det * (
				src_[0][3] * src_[2][1] * src_[3][0] +
				src_[0][1] * src_[2][0] * src_[3][3] +
				src_[0][0] * src_[2][3] * src_[3][1] -
				src_[0][0] * src_[2][1] * src_[3][3] -
				src_[0][1] * src_[2][3] * src_[3][0] -
				src_[0][3] * src_[2][0] * src_[3][1]
			);
		dst_[2][2] =
			inv_Det * (
				src_[0][0] * src_[1][1] * src_[3][3] +
				src_[0][1] * src_[1][3] * src_[3][0] +
				src_[0][3] * src_[1][0] * src_[3][1] -
				src_[0][3] * src_[1][1] * src_[3][0] -
				src_[0][1] * src_[1][0] * src_[3][3] -
				src_[0][0] * src_[1][3] * src_[3][1]
			);
		dst_[2][3] =
			inv_Det * (
				src_[0][3] * src_[1][1] * src_[2][0] +
				src_[0][1] * src_[1][0] * src_[2][3] +
				src_[0][0] * src_[1][3] * src_[2][1] -
				src_[0][0] * src_[1][1] * src_[2][3] -
				src_[0][1] * src_[1][3] * src_[2][0] -
				src_[0][3] * src_[1][0] * src_[2][1]
			);

		dst_[3][0] =
			inv_Det * (
				src_[1][2] * src_[2][1] * src_[3][0] +
				src_[1][1] * src_[2][0] * src_[3][2] +
				src_[1][0] * src_[2][2] * src_[3][1] -
				src_[1][0] * src_[2][1] * src_[3][2] -
				src_[1][1] * src_[2][2] * src_[3][0] -
				src_[1][2] * src_[2][0] * src_[3][1]
			);
		dst_[3][1] =
			inv_Det * (
				src_[0][0] * src_[2][1] * src_[3][2] +
				src_[0][1] * src_[2][2] * src_[3][0] +
				src_[0][2] * src_[2][0] * src_[3][1] -
				src_[0][2] * src_[2][1] * src_[3][0] -
				src_[0][1] * src_[2][0] * src_[3][2] -
				src_[0][0] * src_[2][2] * src_[3][1]
			);
		dst_[3][2] =
			inv_Det * (
				src_[0][2] * src_[1][1] * src_[3][0] +
				src_[0][1] * src_[1][0] * src_[3][2] +
				src_[0][0] * src_[1][2] * src_[3][1] -
				src_[0][0] * src_[1][1] * src_[3][2] -
				src_[0][1] * src_[1][2] * src_[3][0] -
				src_[0][2] * src_[1][0] * src_[3][1]
			);
		dst_[3][3] =
			inv_Det * (
				src_[0][0] * src_[1][1] * src_[2][2] +
				src_[0][1] * src_[1][2] * src_[2][0] +
				src_[0][2] * src_[1][0] * src_[2][1] -
				src_[0][2] * src_[1][1] * src_[2][0] -
				src_[0][1] * src_[1][0] * src_[2][2] -
				src_[0][0] * src_[1][2] * src_[2][1]
			);
	}

	namespace {
		template<typename T>
		void InitializeUB(Lumina::D3D12::UploadBuffer& ub_, T const& data_) {
			auto const& device{ Lumina::Context::Instance().D3D12Context().Device() };
			ub_.Initialize(device, 256LLU);
			ub_.Store(&data_, sizeof(T), 0LLU);
		}

		void IterateBox(
			std::vector<std::unique_ptr<Lumina::D3D12::UploadBuffer>>& ub_,
			CharacterPartBox const& box_
		) {
			for (Lumina::U32 i{ 0LLU }; i < static_cast<Lumina::U32>(box_.Contents.size()); ++i) {
				if (box_.Contents[i].get()->IsBox == 0) {
					InitializeUB(
						*ub_.emplace_back(std::make_unique<Lumina::D3D12::UploadBuffer>()),
						static_cast<CharacterPart*>(box_.Contents[i].get())->Material
					);
				}
				else {
					IterateBox(ub_, *static_cast<CharacterPartBox*>(box_.Contents[i].get()));
				}
			}
		}

		void IterateBox2(
			Lumina::MeshShaderAsset const& meshShaderAsset_,
			CharacterPartBox const& box_,
			Lumina::MeshManager& meshMngr_,
			Lumina::D3D12::DescriptorHeap const& localHeap_Materials_,
			Lumina::Math::F32x4x4<> const& mat_ = Lumina::Math::F32x4x4<>::Identity
		) {
			Lumina::Math::F32x4x4<> mat{ mat_ * box_.Transform };
			for (Lumina::U32 i{ 0LLU }; i < static_cast<Lumina::U32>(box_.Contents.size()); ++i) {
				if (box_.Contents[i].get()->IsBox == 0) {
					meshMngr_.Batch(
						// 板ポリ
						meshShaderAsset_,
						// インスタンス1
						1U,
						// マテリアルID 0
						// ごめんだけどマテリアルのID管理はまだ書いてない
						// 今は全部同じマテリアルになっちゃう
						localHeap_Materials_.CPUHandle(0U),
						mat * box_.Contents[i]->Transform
					);
				}
				else {
					IterateBox2(
						meshShaderAsset_,
						*static_cast<CharacterPartBox*>(box_.Contents[i].get()),
						meshMngr_,
						localHeap_Materials_,
						mat
					);
				}
			}
		}
	}

	void CharacterTest::Initialize() {
		auto& context{ Lumina::Context::Instance() };
		auto& resMngr{ context.ResourceContext() };

		// モデル（板ポリ）読み込み

		auto&& plane{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"Kinoko.obj", "Assets/Kinoko"
				)
			)
		};
		std::vector<Lumina::Utils::Mesh> meshes{};
		meshes.insert(meshes.cend(), plane.cbegin(), plane.cend());
		Lumina::MeshUploader meshUploader{};
		meshUploader.Initialize(context.D3D12Context());
		meshUploader.Begin();
		for (auto const& mesh : meshes) {
			meshUploader.Batch(mesh);
		}
		meshUploader.End(MeshShaderAssets_);

		// テクスチャ読み込み

		std::vector<uint32_t> texIDs{};
		resMngr.Graphics().LoadImageTextures(
			texIDs,
			{
				// uvCheckerは一番目に読み込まれるだからIDは0
				{ "uvChecker", "Assets/Img/uvChecker.png" },
			}
		);

		auto& d3d12Context = context.D3D12Context();
		auto& d3d12Device = context.D3D12Context().Device();

		GlobalTable_SRV_ImageTexture_ = d3d12Context.GlobalDescriptorHeap().Allocate(32U);
		for (uint32_t idx{ 0U }; idx < static_cast<uint32_t>(texIDs.size()); ++idx) {
			d3d12Device->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_ImageTexture_.CPUHandle(idx),
				resMngr.Graphics().CPUHandle(texIDs.at(idx)),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

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

		GraphicsPSO_Mesh_.Initialize(
			d3d12Device,
			context.MeshContext().RootSignature(),
			VS_MeshDeferredGeometry_,
			PS_MeshDeferredGeometry_,
			blendState_None,
			Lumina::D3D12::RasterizerState{
				.FillMode{ D3D12_FILL_MODE_SOLID },
				.CullMode{ D3D12_CULL_MODE_NONE },
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

		UB_WorldToNDC_.Initialize(d3d12Device, 256LLU);
		LocalHeap_Scene_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16U, false);
		Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_Scene_.CPUHandle(0U), UB_WorldToNDC_);


		// ノード全体の座標変換
		Parts_.Transform = SRT({ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
		// パーツの数；サンプルだからまず1に
		Parts_.Contents.resize(1);

		// ここはMainBodyだけだけど、ほかのパーツは同じような感じで
		Parts_.Contents[MainBody] = std::make_unique<CharacterPart>();
		auto& mainBody = *static_cast<CharacterPart*>(Parts_.Contents[MainBody].get());
		// 単体座標変換
		// 結果的な座標はノードとノードの親とノードの親の親（中略）に影響される
		mainBody.Transform =
			SRT({ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
		mainBody.Material = {
			.RGBA = { 1.0f, 1.0f, 1.0f, 1.0f },
			.ID_DiffuseMap = 0
		};

		// パーツは全部make_uniqueしないとエラーになっちゃう
		IterateBox(UB_Materials_, Parts_);

		// マテリアルのCBV作成
		auto const& device{ Lumina::Context::Instance().D3D12Context().Device() };
		LocalHeap_Materials_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32U, false);
		for (Lumina::U32 i{ 0LLU }; i < static_cast<Lumina::U32>(UB_Materials_.size()); ++i) {
			Lumina::D3D12::CBV::Create(device, LocalHeap_Materials_.CPUHandle(i), *UB_Materials_[i]);
		}

		// カメラと投影設定
		View_ = LookAt({ 0.0f, 0.0f, -10.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } );
		Projection_ = PerspectiveFOV(
			0.45f,
			1280.0f / 720.0f,
			0.1f,
			200.0f
		);
	}

	void CharacterTest::Update() {
		ImGui::Begin("Editor");
		ImGui::End();

		Lumina::Math::F32x4x4<> vp = View_ * Projection_;
		UB_WorldToNDC_.Store(&vp, sizeof(vp), 0LLU);
	}

	void CharacterTest::Render() {
		auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto& d3d12Context{ context.D3D12Context() };
		//auto& spriteMngr{ context.SpriteManager() };
		auto& meshMngr{ context.MeshContext() };
		auto const& cmdList{ context.MainCommandList() };

		meshMngr.Begin(cmdList);
		meshMngr.BatchBegin();
		IterateBox2(
			MeshShaderAssets_[0],
			Parts_,
			meshMngr,
			LocalHeap_Materials_
		);
		meshMngr.BatchEnd();
		meshMngr.Render(
			GraphicsPSO_Mesh_,
			GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
			LocalHeap_Scene_.CPUHandle(0U)
		);
		meshMngr.End();

		/*spriteMngr.Begin(cmdList);
		spriteMngr.BatchBegin();
		for (auto const& part : Parts_) {
			spriteMngr.Batch(part.Sprite_);
		}
		spriteMngr.BatchEnd();*/
	}
}