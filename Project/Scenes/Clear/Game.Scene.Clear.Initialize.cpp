//module Game.Scene.Clear;
//
//import <type_traits>;
//
//import : Impl;
//
//namespace Game::Scene {
//	template<>
//	void ClearImpl::Initialize(
//		Lumina::DX12::Context const& dxContext_,
//		Lumina::AssetManager const& assetMngr_,
//		[[maybe_unused]] Lumina::DX12::CommandList const& cmdList_Main_,
//		Lumina::WinApp::RawInput const& input_
//	) {
//		DXContext_ = &dxContext_;
//		Input_ = &input_;
//		CmdList_Main_ = &cmdList_Main_;
//
//		auto const& device{ dxContext_.Device() };
//		AssetManager_ = const_cast<Lumina::AssetManager*>(&assetMngr_);
//
//		CmdAllocator_.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
//		CmdList_.Initialize(device, CmdAllocator_);
//
//		WorldToNDC_ = Lumina::Mat4::Orthographic(0.0f, 1280.0f, 0.0f, 720.0f, 0.0f, 1.0f);
//		{
//			UB_OrthoProj_.Initialize(device, 256LLU, "OrthoProj");
//			UB_OrthoProj_.Store(&WorldToNDC_, sizeof(Lumina::Mat4), 0U);
//			LocalHeap_OrthoProj_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1U, false);
//			Lumina::DX12::CBV::Create(device, LocalHeap_OrthoProj_.CPUHandle(0U), UB_OrthoProj_);
//		}
//
//		BackgroundCenter_ = { 640.0f, 360.0f };
//
//		IterationTime_ = 0;
//		NextIterationTime_ = 0;
//
//		KeyState_Space_ = 0U;
//
//		Count_FadeOut_ = -1;
//
//		// ImageTexture
//		{
//			std::vector<uint32_t> texIDs{};
//			AssetManager_->Graphics().LoadImageTextures(
//				texIDs,
//				{
//					{ "Clear.UI.StageClear", "Assets/UI/Label.StageClear.png" },
//					{ "Clear.UI.PressSpaceKey", "Assets/UI/Label.PressSpaceKey.png" },
//					{ "Particles", "Assets/GFX/Particles.png" },
//					{ "Square", "Assets/GFX/123.png" },
//				}
//			);
//
//			GlobalTable_SRV_ImageTexture_ =
//				DXContext_->GlobalDescriptorHeap().Allocate(
//					static_cast<uint32_t>(texIDs.size())
//				);
//			for (uint32_t idx{ 0U }; idx < static_cast<uint32_t>(texIDs.size()); ++idx) {
//				device->CopyDescriptorsSimple(
//					1U,
//					GlobalTable_SRV_ImageTexture_.CPUHandle(idx),
//					AssetManager_->Graphics().CPUHandle(texIDs.at(idx)),
//					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
//				);
//			}
//		}
//
//		// Mesh
//		{
//			MeshManager_.reset(new Lumina::MeshManager{});
//			MeshManager_->Initialize(dxContext_, 2048U, 65536U);
//
//			// Pipeline
//			{
//				VS_MeshDeferredGeometry_.Initialize(
//					dxContext_.Compiler(),
//					L"Assets/Shaders/MeshCommon.VS.hlsl",
//					L"vs_6_6",
//					L"main",
//					"MeshCommon.VS"
//				);
//				PS_MeshDeferredGeometry_.Initialize(
//					dxContext_.Compiler(),
//					L"Assets/Shaders/Mesh2.PS.hlsl",
//					L"ps_6_6",
//					L"main",
//					"Mesh2.PS"
//				);
//
//				Lumina::DX12::BlendState blendState{};
//				blendState.IndependentBlendEnable = true;
//				blendState.RenderTarget[0].BlendEnable = false;
//				blendState.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC{
//					.BlendEnable{ true },
//					.LogicOpEnable{ false },
//					.SrcBlend{ D3D12_BLEND_SRC_ALPHA },
//					.DestBlend{ D3D12_BLEND_ONE },
//					.BlendOp{ D3D12_BLEND_OP_ADD },
//					.SrcBlendAlpha{ D3D12_BLEND_SRC_ALPHA },
//					.DestBlendAlpha{ D3D12_BLEND_ONE },
//					.BlendOpAlpha{ D3D12_BLEND_OP_ADD },
//					.LogicOp{ D3D12_LOGIC_OP_NOOP },
//					.RenderTargetWriteMask{ D3D12_COLOR_WRITE_ENABLE_ALL },
//				};
//				blendState.RenderTarget[1].BlendEnable = false;
//				blendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//				Lumina::DX12::GraphicsPSO::InputLayout inputLayout{};
//				inputLayout.Append("IDX_POSITION", 0U, DXGI_FORMAT_R32_UINT);
//				inputLayout.Append("IDX_TEXCOORD", 0U, DXGI_FORMAT_R32_UINT);
//				inputLayout.Append("IDX_NORMAL", 0U, DXGI_FORMAT_R32_UINT);
//				inputLayout.Append("IDX_TANGENT", 0U, DXGI_FORMAT_R32_UINT);
//				GraphicsPSO_MeshDeferredGeometry_.Initialize(
//					device,
//					MeshManager_->RootSignature(),
//					VS_MeshDeferredGeometry_,
//					PS_MeshDeferredGeometry_,
//					blendState,
//					Lumina::DX12::RasterizerState{
//						.FillMode{ D3D12_FILL_MODE_SOLID },
//						.CullMode{ D3D12_CULL_MODE_NONE },
//					},
//					Lumina::DX12::DepthStencilState{
//						.DepthEnable{ false },
//						/*.DepthEnable{ true },
//						.DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ALL },
//						.DepthFunc{ D3D12_COMPARISON_FUNC_LESS_EQUAL },*/
//						.StencilEnable{ false },
//					},
//					inputLayout,
//					D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
//					{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM, },
//					Lumina::DX12::GraphicsPSO::DefaultDSVFormat
//					);
//			}
//
//			auto&& plane{
//				Lumina::Utils::Mesh::Load(
//					Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
//						"Plane.obj", "Assets/Models"
//					)
//				)
//			};
//
//			std::vector<Lumina::Utils::Mesh> meshes{};
//			meshes.insert(meshes.cend(), plane.cbegin(), plane.cend());
//
//			Lumina::MeshUploader meshUploader{};
//			meshUploader.Initialize(dxContext_);
//			meshUploader.Begin();
//			for (auto const& mesh : meshes) {
//				meshUploader.Batch(mesh);
//			}
//			meshUploader.End(MeshShaderAssets_);
//		}
//
//		// Mesh Material
//		{
//			Arr_MeshMaterial_[0] = {
//				.RGBA{ 1.0f, 1.0f, 1.0f, 1.0f, },
//				.ID_DiffuseMap{ IT_Square },
//				.ID_SpecularMap{ 0 },
//				.ID_NormalMap{ 0 },
//			};
//
//			for (uint32_t idx{ 0U }; idx < static_cast<uint32_t>(Arr_UB_MeshMaterial_.size()); ++idx) {
//				Arr_UB_MeshMaterial_[idx].Initialize(device, 256LLU);
//				Arr_UB_MeshMaterial_[idx].Store(
//					Arr_MeshMaterial_.data() + idx,
//					sizeof(ClearMeshMaterial),
//					0LLU
//				);
//			}
//
//			LocalHeap_Arr_CBV_MeshMaterial_.Initialize(
//				device,
//				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
//				static_cast<uint32_t>(Arr_UB_MeshMaterial_.size()),
//				false
//			);
//
//			for (uint32_t idx{ 0U }; idx < static_cast<uint32_t>(Arr_UB_MeshMaterial_.size()); ++idx) {
//				Lumina::DX12::CBV::Create(
//					device, LocalHeap_Arr_CBV_MeshMaterial_.CPUHandle(idx),
//					Arr_UB_MeshMaterial_[idx]);
//			}
//		}
//
//		SceneRotation_ = 0.0f;
//
//		// Sprite
//		{
//			SpriteRenderer_.reset(new Lumina::SpriteRenderer{});
//			SpriteRenderer_->Initialize(dxContext_, 128U);
//
//			// Pipeline
//			{
//				DXContext_->Compile(
//					VS_Sprite_,
//					L"Assets/Shaders/Sprite2.VS.hlsl",
//					L"vs_6_6",
//					L"main",
//					"Sprite2.VS"
//				);
//				DXContext_->Compile(
//					PS_Sprite_,
//					L"Assets/Shaders/Sprite2.PS.hlsl",
//					L"ps_6_6",
//					L"main",
//					"Sprite2.PS"
//				);
//
//				Lumina::DX12::BlendState spriteBlendState{};
//				spriteBlendState.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC{
//					.BlendEnable{ true },
//					.LogicOpEnable{ false },
//					.SrcBlend{ D3D12_BLEND_SRC_ALPHA },
//					.DestBlend{ D3D12_BLEND_INV_SRC_ALPHA },
//					.BlendOp{ D3D12_BLEND_OP_ADD },
//					.SrcBlendAlpha{ D3D12_BLEND_SRC_ALPHA },
//					.DestBlendAlpha{ D3D12_BLEND_ONE },
//					.BlendOpAlpha{ D3D12_BLEND_OP_ADD },
//					.LogicOp{ D3D12_LOGIC_OP_NOOP },
//					.RenderTargetWriteMask{ D3D12_COLOR_WRITE_ENABLE_ALL },
//				};
//				Lumina::DX12::GraphicsPSO::InputLayout spriteInputLayout{};
//				spriteInputLayout.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT);
//				PSO_Sprite_.Initialize(
//					device,
//					SpriteRenderer_->RootSignature(),
//					VS_Sprite_,
//					PS_Sprite_,
//					spriteBlendState,
//					Lumina::DX12::RasterizerState{
//						.FillMode{ D3D12_FILL_MODE_SOLID },
//						.CullMode{ D3D12_CULL_MODE_NONE },
//					},
//					Lumina::DX12::DepthStencilState{
//						.DepthEnable{ false },
//						.StencilEnable{ false },
//					},
//					spriteInputLayout,
//					D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
//					{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM, },
//					Lumina::DX12::GraphicsPSO::DefaultDSVFormat
//					);
//
//
//				DXContext_->Compile(
//					PS_SpriteUI_,
//					L"Assets/Shaders/SpriteUI.PS.hlsl",
//					L"ps_6_6",
//					L"main",
//					"SpriteUI.PS"
//				);
//				PSO_SpriteUI_.Initialize(
//					device,
//					SpriteRenderer_->RootSignature(),
//					VS_Sprite_,
//					PS_SpriteUI_,
//					spriteBlendState,
//					Lumina::DX12::RasterizerState{
//						.FillMode{ D3D12_FILL_MODE_SOLID },
//						.CullMode{ D3D12_CULL_MODE_NONE },
//					},
//					Lumina::DX12::DepthStencilState{
//						.DepthEnable{ false },
//						.StencilEnable{ false },
//					},
//					spriteInputLayout,
//					D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
//					{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, },
//					Lumina::DX12::GraphicsPSO::DefaultDSVFormat
//					);
//			}
//
//			{
//				UI_Label_StageClear_.Translate({ 640.0f, 390.0f });
//				UI_Label_StageClear_.Scale({ 720.0f, 180.0f });
//				UI_Label_StageClear_.AnchorPoint({ 0.5f, 0.5f });
//				UI_Label_StageClear_.TextureID(IT_StageClear);
//				UI_Label_StageClear_.RGBA({ 1.0f, 1.0f, 1.0f, 0.75f });
//			}
//			{
//				UI_Label_PressSpaceKey_.Translate({ 640.0f, 510.0f });
//				UI_Label_PressSpaceKey_.Scale({ 720.0f, 180.0f });
//				UI_Label_PressSpaceKey_.AnchorPoint({ 0.5f, 0.5f });
//				UI_Label_PressSpaceKey_.TextureID(IT_PressSpaceKey);
//				UI_Label_PressSpaceKey_.RGBA({ 1.0f, 1.0f, 1.0f, 0.75f });
//			}
//		}
//
//		// Particles
//		{
//			auto config_ParticleSystem{
//				Lumina::Utils::LoadFromFile<nlohmann::json>(
//					"Assets/Configs/ParticleSystem.json"
//				)
//			};
//			RS_ParticleSystem_.Initialize(
//				device,
//				Lumina::DX12::LoadRootSignatureSetup(
//					config_ParticleSystem.at("Common RS")
//				)
//			);
//
//			dxContext_.Compile(
//				VS_BasicParticle_,
//				L"Assets/Shaders/BasicParticle.VS.hlsl",
//				L"vs_6_6",
//				L"main",
//				"BasicParticle.VS"
//			);
//			dxContext_.Compile(
//				PS_BasicParticle_,
//				L"Assets/Shaders/BasicParticle.PS.hlsl",
//				L"ps_6_6",
//				L"main",
//				"BasicParticle.PS"
//			);
//
//			Lumina::DX12::BlendState blendState_AdditiveMode{};
//			blendState_AdditiveMode.RenderTarget[0] = {
//				.BlendEnable{ true },
//				.SrcBlend{ D3D12_BLEND_SRC_ALPHA },
//				.DestBlend{ D3D12_BLEND_ONE },
//				.BlendOp{ D3D12_BLEND_OP_ADD },
//				.SrcBlendAlpha{ D3D12_BLEND_SRC_ALPHA },
//				.DestBlendAlpha{ D3D12_BLEND_ONE },
//				.BlendOpAlpha{ D3D12_BLEND_OP_ADD },
//				.RenderTargetWriteMask{ D3D12_COLOR_WRITE_ENABLE_ALL },
//			};
//
//			Lumina::DX12::GraphicsPSO::InputLayout inputLayout_Particle{};
//			inputLayout_Particle.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT);
//			inputLayout_Particle.Append("TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT);
//			GraphicsPSO_BasicParticle_AdditiveMode_.Initialize(
//				device,
//				RS_ParticleSystem_,
//				VS_BasicParticle_,
//				PS_BasicParticle_,
//				blendState_AdditiveMode,
//				Lumina::DX12::RasterizerState{
//					.FillMode{ D3D12_FILL_MODE_SOLID },
//					.CullMode{ D3D12_CULL_MODE_NONE },
//				},
//				Lumina::DX12::DepthStencilState{
//					.DepthEnable{ false },
//					.StencilEnable{ false },
//				},
//				inputLayout_Particle,
//				D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
//				{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, },
//				Lumina::DX12::GraphicsPSO::DefaultDSVFormat
//				);
//
//			CircularSparkles_.reset(new ParticleSystem<Particle>{});
//			CircularSparkles_->Initialize(dxContext_, 512U);
//
//			TitleCaptionEffect_.reset(new ParticleSystem<Particle>{});
//			TitleCaptionEffect_->Initialize(dxContext_, 1024U);
//
//			ButtonOnFocusEffect_.reset(new ParticleSystem<Particle>{});
//			ButtonOnFocusEffect_->Initialize(dxContext_, 512U);
//
//			ButtonOnUnfocusEffect_.reset(new ParticleSystem<Particle>{});
//			ButtonOnUnfocusEffect_->Initialize(dxContext_, 512U);
//
//			ButtonOnClickEffect_.reset(new ParticleSystem<Particle>{});
//			ButtonOnClickEffect_->Initialize(dxContext_, 512U);
//		}
//
//		// DeferredLighting
//		{
//			DeferredLighting_.reset(new Lumina::DeferredLighting{});
//			DeferredLighting_->Initialize(
//				dxContext_,
//				1280U, 720U,
//				L"Assets/Shaders/Lighting.VS.hlsl",
//				L"Assets/Shaders/Lighting2.PS.hlsl"
//			);
//
//			UB_ScreenToWorld_.Initialize(device, 256LLU);
//			LocalHeap_ScreenToWorld_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1U, false);
//			Lumina::DX12::CBV::Create(device, LocalHeap_ScreenToWorld_.CPUHandle(0U), UB_ScreenToWorld_);
//			Lumina::Mat4 screenToWorld{
//				1.0f / 640.0f, 0.0f, 0.0f, 0.0f,
//				0.0f, -1.0f / 360.0f, 0.0f, 0.0f,
//				0.0f, 0.0f, 1.0f, 0.0f,
//				-1.0f, 1.0f, 0.0f, 1.0f,
//			};
//			screenToWorld *= WorldToNDC_.Inv();
//			UB_ScreenToWorld_.Store(&screenToWorld, sizeof(Lumina::Mat4), 0LLU);
//
//			List_PointLight_.Initialize(2048U);
//			List_Matrix_World_LightSphere_.Initialize(2048U);
//		}
//
//		// Canvas::Geometry
//		{
//			Canvas_Geometry_.AllocateTextures(2U, true);
//			Canvas_Geometry_.RenderTexture(0U).Initialize(device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
//			Canvas_Geometry_.RenderTexture(1U).Initialize(device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM);
//			Canvas_Geometry_.DepthTexture().Initialize(device, 1280U, 720U);
//			Canvas_Geometry_.TransitionResourceStates(device, dxContext_.DirectQueue());
//			Canvas_Geometry_.CreateViews(device);
//			Canvas_Geometry_.Viewport(0U) = D3D12_VIEWPORT{
//				.TopLeftX{ 0.0f },
//				.TopLeftY{ 0.0f },
//				.Width{ 1280.0f },
//				.Height{ 720.0f },
//				.MinDepth{ 0.0f },
//				.MaxDepth{ 1.0f },
//			};
//			Canvas_Geometry_.ScissorRect(0U) = D3D12_RECT{
//				.left{ 0 },
//				.top{ 0 },
//				.right{ 1280 },
//				.bottom{ 720 },
//			};
//			Canvas_Geometry_.Viewport(1U) = D3D12_VIEWPORT{
//				.TopLeftX{ 0.0f },
//				.TopLeftY{ 0.0f },
//				.Width{ 0.0f },
//				.Height{ 0.0f },
//				.MinDepth{ 0.0f },
//				.MaxDepth{ 1.0f },
//			};
//			Canvas_Geometry_.ScissorRect(1U) = D3D12_RECT{
//				.left{ 640 },
//				.top{ 360 },
//				.right{ 1280 },
//				.bottom{ 720 },
//			};
//		}
//
//		// Canvas::BackgroundMerge
//		{
//			Canvas_Background_Merge_.AllocateTextures(1U, true);
//			Canvas_Background_Merge_.RenderTexture(0U).Initialize(
//				device,
//				1280U,
//				720U,
//				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
//			);
//			Canvas_Background_Merge_.DepthTexture().Initialize(device, 1280U, 720U);
//			Canvas_Background_Merge_.TransitionResourceStates(device, dxContext_.DirectQueue());
//			Canvas_Background_Merge_.CreateViews(device);
//			Canvas_Background_Merge_.Viewport(0U) = D3D12_VIEWPORT{
//				.TopLeftX{ 0.0f },
//				.TopLeftY{ 0.0f },
//				.Width{ 1280.0f },
//				.Height{ 720.0f },
//				.MinDepth{ 0.0f },
//				.MaxDepth{ 1.0f },
//			};
//			Canvas_Background_Merge_.ScissorRect(0U) = D3D12_RECT{
//				.left{ 0 },
//				.top{ 0 },
//				.right{ 1280 },
//				.bottom{ 720 },
//			};
//		}
//
//		// Canvas::PostProcessing
//		{
//			/*Canvas_PostProcessing_.AllocateTextures(1U, false);
//			Canvas_PostProcessing_.RenderTexture(0U).Initialize(
//				device,
//				1280U,
//				720U,
//				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
//			);
//			Canvas_PostProcessing_.TransitionResourceStates(device, dxContext_.DirectQueue());
//			Canvas_PostProcessing_.CreateViews(device);
//			Canvas_PostProcessing_.Viewport(0U) = D3D12_VIEWPORT{
//				.TopLeftX{ 0.0f },
//				.TopLeftY{ 0.0f },
//				.Width{ 1280.0f },
//				.Height{ 720.0f },
//				.MinDepth{ 0.0f },
//				.MaxDepth{ 1.0f },
//			};
//			Canvas_PostProcessing_.ScissorRect(0U) = D3D12_RECT{
//				.left{ 0 },
//				.top{ 0 },
//				.right{ 1280 },
//				.bottom{ 720 },
//			};*/
//		}
//
//		// Canvas SRV
//		{
//			GlobalTable_SRV_CanvasTexture_ = dxContext_.GlobalDescriptorHeap().Allocate(8U);
//			Lumina::DX12::SRV<void>::Create(
//				device,
//				GlobalTable_SRV_CanvasTexture_.CPUHandle(0U),
//				Canvas_Geometry_.RenderTexture(0U)
//			);
//			Lumina::DX12::SRV<void>::Create(
//				device,
//				GlobalTable_SRV_CanvasTexture_.CPUHandle(1U),
//				Canvas_Geometry_.RenderTexture(1U)
//			);
//			Lumina::DX12::SRV<void>::Create<DXGI_FORMAT_R24_UNORM_X8_TYPELESS>(
//				device,
//				GlobalTable_SRV_CanvasTexture_.CPUHandle(2U),
//				Canvas_Geometry_.DepthTexture()
//			);
//
//			Lumina::DX12::SRV<void>::Create(
//				device,
//				GlobalTable_SRV_CanvasTexture_.CPUHandle(3U),
//				DeferredLighting_->RenderTexture()
//			);
//
//			Lumina::DX12::SRV<void>::Create(
//				device,
//				GlobalTable_SRV_CanvasTexture_.CPUHandle(4U),
//				Canvas_Background_Merge_.RenderTexture(0U)
//			);
//		}
//
//		// DeferredGeometryPass
//		{
//			DeferredGeometryPass_.Initialize(2U, true);
//			DeferredGeometryPass_.RenderTarget(0).BeginningEvent().ClearTarget(
//				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
//				{ 0.0f, 0.0f, 0.0f, 0.0f }
//			);
//			DeferredGeometryPass_.RenderTarget(0).EndingEvent().Preserve();
//			DeferredGeometryPass_.RenderTarget(1).BeginningEvent().ClearTarget(
//				DXGI_FORMAT_R8G8B8A8_UNORM,
//				{ 0.0f, 0.0f, 0.0f, 0.0f }
//			);
//			DeferredGeometryPass_.RenderTarget(1).EndingEvent().Preserve();
//			DeferredGeometryPass_.DepthStencil().DepthBeginningEvent().ClearTarget(
//				DXGI_FORMAT_D24_UNORM_S8_UINT,
//				{ .Depth{ 1.0f }, }
//			);
//			DeferredGeometryPass_.DepthStencil().DepthEndingEvent().Preserve();
//			DeferredGeometryPass_.DepthStencil().StencilBeginningEvent().NoAccess();
//			DeferredGeometryPass_.DepthStencil().StencilEndingEvent().NoAccess();
//
//			for (uint32_t idx{ 0U }; idx < Canvas_Geometry_.Num_RenderTargets(); ++idx) {
//				DeferredGeometryPass_.RenderTarget(idx).View() = Canvas_Geometry_.RTV(idx);
//			}
//			DeferredGeometryPass_.DepthStencil().View() = Canvas_Geometry_.DSV();
//		}
//		// MergePass
//		{
//			MergePass_.Initialize(1U, true);
//			/*MergePass_.RenderTarget(0).BeginningEvent().ClearTarget(
//				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
//				{ 0.0f, 0.0f, 0.0f, 0.0f }
//			);*/
//			MergePass_.RenderTarget(0).BeginningEvent().Preserve();
//			MergePass_.RenderTarget(0).EndingEvent().Preserve();
//			MergePass_.DepthStencil().DepthBeginningEvent().ClearTarget(
//				DXGI_FORMAT_D24_UNORM_S8_UINT,
//				{ .Depth{ 1.0f }, }
//			);
//			MergePass_.DepthStencil().DepthEndingEvent().Preserve();
//			MergePass_.DepthStencil().StencilBeginningEvent().NoAccess();
//			MergePass_.DepthStencil().StencilEndingEvent().NoAccess();
//
//			MergePass_.RenderTarget(0U).View() = Canvas_Background_Merge_.RTV(0U);
//			MergePass_.DepthStencil().View() = Canvas_Background_Merge_.DSV();
//		}
//		// PostProcessingPass
//		{
//			PostProcessingPass_.Initialize(1U, true);
//			/*PostProcessingPass_.RenderTarget(0).BeginningEvent().ClearTarget(
//				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
//				{ 0.0f, 0.0f, 0.0f, 0.0f }
//			);*/
//			PostProcessingPass_.RenderTarget(0).BeginningEvent().Preserve();
//			PostProcessingPass_.RenderTarget(0).EndingEvent().Preserve();
//			/*PostProcessingPass_.DepthStencil().DepthBeginningEvent().ClearTarget(
//				DXGI_FORMAT_D24_UNORM_S8_UINT,
//				{ .Depth{ 1.0f }, }
//			);
//			PostProcessingPass_.DepthStencil().DepthEndingEvent().Preserve();
//			PostProcessingPass_.DepthStencil().StencilBeginningEvent().NoAccess();
//			PostProcessingPass_.DepthStencil().StencilEndingEvent().NoAccess();*/
//
//			//PostProcessingPass_.RenderTarget(0U).View() = Canvas_PostProcessing_.RTV(0U);
//		}
//
//		// PostProcessingConstants
//		{
//			UB_PostProcessingConstants_.Initialize(device, 256LLU);
//			GlobalTable_CBV_PostProcessing_ = dxContext_.GlobalDescriptorHeap().Allocate(1U);
//			Lumina::DX12::CBV::Create(
//				device,
//				GlobalTable_CBV_PostProcessing_.CPUHandle(0U),
//				UB_PostProcessingConstants_
//			);
//
//			ClearPostProcessingConstants_.IsFadingOut = 0U;
//			UB_PostProcessingConstants_.Store(
//				&ClearPostProcessingConstants_,
//				sizeof(ClearPostProcessingConstants),
//				0LLU
//			);
//		}
//
//		{
//			PrimitiveManager0_.reset(new Lumina::PrimitiveManager{});
//			PrimitiveManager0_->Initialize(dxContext_);
//			PrimitiveManager1_.reset(new Lumina::PrimitiveManager{});
//			PrimitiveManager1_->Initialize(
//				dxContext_,
//				L"Assets/Shaders/TitleBG.VS.hlsl",
//				L"Assets/Shaders/TitleBG.PS.hlsl",
//				true,
//				false
//			);
//		}
//
//		{
//			UB_Dummy_.Initialize(device, 256LLU, "Dummy");
//			LocalHeap_Dummy_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1U, false);
//			Lumina::DX12::CBV::Create(device, LocalHeap_Dummy_.CPUHandle(0U), UB_Dummy_);
//		}
//	}
//
//	constexpr ClearImpl::~ClearImpl() noexcept = default;
//
//	template<>
//	Clear::Clear(
//		Lumina::DX12::Context const& dxContext_,
//		Lumina::AssetManager const& assetMngr_,
//		Lumina::DX12::CommandList const& cmdList_Main_,
//		Lumina::WinApp::RawInput const& input_
//	) {
//		Impl_.reset(new ClearImpl{});
//		Impl_->Initialize(dxContext_, assetMngr_, cmdList_Main_, input_);
//	}
//
//	Clear::~Clear() {}
//}