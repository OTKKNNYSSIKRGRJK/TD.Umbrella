module Game.Scene.Clear;

import <type_traits>;

import : Impl;

import Lumina.Main;
import Game.MathUtils;
import Lumina.Utils.Data;
import Lumina.D3D12.Aux;

namespace Game::Scene::Impl {
	template<>
	void Clear::Initialize(
	) {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };
		//auto const& cmdList{ context.MainCommandList() };
		auto& resMngr{ context.ResourceContext() };

		CmdAllocator_.Initialize(d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		CmdList_.Initialize(d3d12Device, CmdAllocator_);

		WorldToNDC_ = MathUtils::Orthographic(0.0f, 1280.0f, 0.0f, 720.0f, 0.0f, 1.0f);
		{
			UB_OrthoProj_.Initialize(d3d12Device, 256LLU, "OrthoProj");
			UB_OrthoProj_.Store(&WorldToNDC_, sizeof(Lumina::Math::F32x4x4<>), 0U);
			LocalHeap_OrthoProj_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1U, false);
			Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_OrthoProj_.CPUHandle(0U), UB_OrthoProj_);
		}

		BackgroundCenter_ = { 640.0f, 360.0f };

		IterationTime_ = 0;
		NextIterationTime_ = 0;

		KeyState_Space_ = 0U;

		Count_FadeOut_ = -1;

		// ImageTexture
		{
			std::vector<uint32_t> texIDs{};
			resMngr.Graphics().LoadImageTextures(
				texIDs,
				{
					{ "Clear.UI.StageClear", "Assets/Img/UI/Title.png" },
					{ "Clear.UI.PressSpaceKey", "Assets/Img/UI/returntotitle.png" },
					{ "Particles", "Assets/Img/Particles.png" },
				}
				);

			GlobalTable_SRV_ImageTexture_ =
				d3d12Context.GlobalDescriptorHeap().Allocate(
					static_cast<uint32_t>(texIDs.size())
				);
			for (uint32_t idx{ 0U }; idx < static_cast<uint32_t>(texIDs.size()); ++idx) {
				d3d12Device->CopyDescriptorsSimple(
					1U,
					GlobalTable_SRV_ImageTexture_.CPUHandle(idx),
					resMngr.Graphics().CPUHandle(texIDs.at(idx)),
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
				);
			}
		}

		SceneRotation_ = 0.0f;

		// Sprite
		{
			SpriteRenderer_.reset(new Lumina::SpriteRenderer{});
			SpriteRenderer_->Initialize(d3d12Context, 128U);

			// Pipeline
			{
				d3d12Context.Compile(
					VS_Sprite_,
					L"Assets/Shaders/Sprite2.VS.hlsl",
					L"vs_6_6",
					L"main",
					"Sprite2.VS"
				);
				d3d12Context.Compile(
					PS_Sprite_,
					L"Assets/Shaders/Sprite2.PS.hlsl",
					L"ps_6_6",
					L"main",
					"Sprite2.PS"
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
				PSO_Sprite_.Initialize(
					d3d12Device,
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
					{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM, },
					Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
					);


				d3d12Context.Compile(
					PS_SpriteUI_,
					L"Assets/Shaders/SpriteUI.PS.hlsl",
					L"ps_6_6",
					L"main",
					"SpriteUI.PS"
				);
				PSO_SpriteUI_.Initialize(
					d3d12Device,
					SpriteRenderer_->RootSignature(),
					VS_Sprite_,
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

			{
				UI_Label_StageClear_.Translate = { 640.0f, 390.0f };
				UI_Label_StageClear_.Scale = { 720.0f, 180.0f };
				UI_Label_StageClear_.AnchorPoint = { 0.5f, 0.5f };
				UI_Label_StageClear_.TextureID = IT_StageClear;
				UI_Label_StageClear_.RGBA = { 1.0f, 1.0f, 1.0f, 0.75f };
			}
			{
				UI_Label_PressSpaceKey_.Translate = { 640.0f, 510.0f };
				UI_Label_PressSpaceKey_.Scale = { 720.0f, 180.0f };
				UI_Label_PressSpaceKey_.AnchorPoint = { 0.5f, 0.5f };
				UI_Label_PressSpaceKey_.TextureID = IT_PressSpaceKey;
				UI_Label_PressSpaceKey_.RGBA = { 1.0f, 1.0f, 1.0f, 0.75f };
			}
		}

		// Particles
		{
			auto config_ParticleSystem{
				Lumina::Utils::LoadFromFile<nlohmann::json>(
					"Assets/Configs/ParticleSystem.json"
				)
			};
			RS_ParticleSystem_.Initialize(
				d3d12Device,
				Lumina::D3D12::LoadSetup<Lumina::D3D12::RootSignature>(
					config_ParticleSystem.at("Common RS")
				)
			);

			d3d12Context.Compile(
				VS_BasicParticle_,
				L"Assets/Shaders/BasicParticle.VS.hlsl",
				L"vs_6_6",
				L"main",
				"BasicParticle.VS"
			);
			d3d12Context.Compile(
				PS_BasicParticle_,
				L"Assets/Shaders/BasicParticle.PS.hlsl",
				L"ps_6_6",
				L"main",
				"BasicParticle.PS"
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
				d3d12Device,
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
					.StencilEnable{ false },
				},
				inputLayout_Particle,
				D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, },
				Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
				);

			CircularSparkles_.reset(new Lumina::ParticleSystem<Lumina::Particle>{});
			CircularSparkles_->Initialize(d3d12Context, 512U);
		}

		// DeferredLighting
		{
			DeferredLighting_.reset(new Lumina::DeferredLighting{});
			DeferredLighting_->Initialize(
				d3d12Context,
				1280U, 720U
			);

			UB_ScreenToWorld_.Initialize(d3d12Device, 256LLU);
			LocalHeap_ScreenToWorld_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1U, false);
			Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_ScreenToWorld_.CPUHandle(0U), UB_ScreenToWorld_);
			Lumina::Math::F32x4x4<> screenToWorld{
				1.0f / 640.0f, 0.0f, 0.0f, 0.0f,
				0.0f, -1.0f / 360.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				-1.0f, 1.0f, 0.0f, 1.0f,
			};
			screenToWorld = screenToWorld * WorldToNDC_.Inverse();
			UB_ScreenToWorld_.Store(&screenToWorld, sizeof(Lumina::Math::F32x4x4<>), 0LLU);

			List_PointLight_.Initialize(2048U);
			List_Matrix_World_LightSphere_.Initialize(2048U);
		}

		// Canvas::Geometry
		{
			Canvas_Geometry_.AllocateTextures(2U, true);
			Canvas_Geometry_.RenderTexture(0U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
			Canvas_Geometry_.RenderTexture(1U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM);
			Canvas_Geometry_.DepthTexture().Initialize(d3d12Device, 1280U, 720U);
			Canvas_Geometry_.TransitionResourceStates(d3d12Device, d3d12Context.DirectQueue());
			Canvas_Geometry_.CreateViews(d3d12Device);
			Canvas_Geometry_.Viewport(0U) = D3D12_VIEWPORT{
				.TopLeftX{ 0.0f },
				.TopLeftY{ 0.0f },
				.Width{ 1280.0f },
				.Height{ 720.0f },
				.MinDepth{ 0.0f },
				.MaxDepth{ 1.0f },
			};
			Canvas_Geometry_.ScissorRect(0U) = D3D12_RECT{
				.left{ 0 },
				.top{ 0 },
				.right{ 1280 },
				.bottom{ 720 },
			};
			Canvas_Geometry_.Viewport(1U) = D3D12_VIEWPORT{
				.TopLeftX{ 0.0f },
				.TopLeftY{ 0.0f },
				.Width{ 0.0f },
				.Height{ 0.0f },
				.MinDepth{ 0.0f },
				.MaxDepth{ 1.0f },
			};
			Canvas_Geometry_.ScissorRect(1U) = D3D12_RECT{
				.left{ 640 },
				.top{ 360 },
				.right{ 1280 },
				.bottom{ 720 },
			};
		}

		// Canvas::BackgroundMerge
		{
			Canvas_Background_Merge_.AllocateTextures(1U, true);
			Canvas_Background_Merge_.RenderTexture(0U).Initialize(
				d3d12Device,
				1280U,
				720U,
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
			);
			Canvas_Background_Merge_.DepthTexture().Initialize(d3d12Device, 1280U, 720U);
			Canvas_Background_Merge_.TransitionResourceStates(d3d12Device, d3d12Context.DirectQueue());
			Canvas_Background_Merge_.CreateViews(d3d12Device);
			Canvas_Background_Merge_.Viewport(0U) = D3D12_VIEWPORT{
				.TopLeftX{ 0.0f },
				.TopLeftY{ 0.0f },
				.Width{ 1280.0f },
				.Height{ 720.0f },
				.MinDepth{ 0.0f },
				.MaxDepth{ 1.0f },
			};
			Canvas_Background_Merge_.ScissorRect(0U) = D3D12_RECT{
				.left{ 0 },
				.top{ 0 },
				.right{ 1280 },
				.bottom{ 720 },
			};
		}

		// Canvas::PostProcessing
		{
			/*Canvas_PostProcessing_.AllocateTextures(1U, false);
			Canvas_PostProcessing_.RenderTexture(0U).Initialize(
				d3d12Device,
				1280U,
				720U,
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
			);
			Canvas_PostProcessing_.TransitionResourceStates(d3d12Device, d3d12Context.DirectQueue());
			Canvas_PostProcessing_.CreateViews(d3d12Device);
			Canvas_PostProcessing_.Viewport(0U) = D3D12_VIEWPORT{
				.TopLeftX{ 0.0f },
				.TopLeftY{ 0.0f },
				.Width{ 1280.0f },
				.Height{ 720.0f },
				.MinDepth{ 0.0f },
				.MaxDepth{ 1.0f },
			};
			Canvas_PostProcessing_.ScissorRect(0U) = D3D12_RECT{
				.left{ 0 },
				.top{ 0 },
				.right{ 1280 },
				.bottom{ 720 },
			};*/
		}

		// Canvas SRV
		{
			GlobalTable_SRV_CanvasTexture_ = d3d12Context.GlobalDescriptorHeap().Allocate(8U);
			Lumina::D3D12::SRV<void>::Create(
				d3d12Device,
				GlobalTable_SRV_CanvasTexture_.CPUHandle(0U),
				Canvas_Geometry_.RenderTexture(0U)
			);
			Lumina::D3D12::SRV<void>::Create(
				d3d12Device,
				GlobalTable_SRV_CanvasTexture_.CPUHandle(1U),
				Canvas_Geometry_.RenderTexture(1U)
			);
			Lumina::D3D12::SRV<void>::Create<DXGI_FORMAT_R24_UNORM_X8_TYPELESS>(
				d3d12Device,
				GlobalTable_SRV_CanvasTexture_.CPUHandle(3U),
				Canvas_Geometry_.DepthTexture()
			);

			Lumina::D3D12::SRV<void>::Create(
				d3d12Device,
				GlobalTable_SRV_CanvasTexture_.CPUHandle(4U),
				DeferredLighting_->RenderTexture()
			);

			Lumina::D3D12::SRV<void>::Create(
				d3d12Device,
				GlobalTable_SRV_CanvasTexture_.CPUHandle(5U),
				Canvas_Background_Merge_.RenderTexture(0U)
			);
		}

		// DeferredGeometryPass
		{
			float const clearColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

			DeferredGeometryPass_.Initialize(2U, true);
			DeferredGeometryPass_.RenderTarget(0).BeginningEvent().ClearTarget(
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				clearColor
			);
			DeferredGeometryPass_.RenderTarget(0).EndingEvent().Preserve();
			DeferredGeometryPass_.RenderTarget(1).BeginningEvent().ClearTarget(
				DXGI_FORMAT_R8G8B8A8_UNORM,
				clearColor
			);
			DeferredGeometryPass_.RenderTarget(1).EndingEvent().Preserve();
			DeferredGeometryPass_.DepthStencil().DepthBeginningEvent().ClearTarget(
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				{ .Depth{ 1.0f }, }
			);
			DeferredGeometryPass_.DepthStencil().DepthEndingEvent().Preserve();
			DeferredGeometryPass_.DepthStencil().StencilBeginningEvent().NoAccess();
			DeferredGeometryPass_.DepthStencil().StencilEndingEvent().NoAccess();

			for (uint32_t idx{ 0U }; idx < Canvas_Geometry_.Num_RenderTargets(); ++idx) {
				DeferredGeometryPass_.RenderTarget(idx).View() = Canvas_Geometry_.RTV(idx);
			}
			DeferredGeometryPass_.DepthStencil().View() = Canvas_Geometry_.DSV();
		}
		// MergePass
		{
			MergePass_.Initialize(1U, true);
			/*MergePass_.RenderTarget(0).BeginningEvent().ClearTarget(
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				{ 0.0f, 0.0f, 0.0f, 0.0f }
			);*/
			MergePass_.RenderTarget(0).BeginningEvent().Preserve();
			MergePass_.RenderTarget(0).EndingEvent().Preserve();
			MergePass_.DepthStencil().DepthBeginningEvent().ClearTarget(
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				{ .Depth{ 1.0f }, }
			);
			MergePass_.DepthStencil().DepthEndingEvent().Preserve();
			MergePass_.DepthStencil().StencilBeginningEvent().NoAccess();
			MergePass_.DepthStencil().StencilEndingEvent().NoAccess();

			MergePass_.RenderTarget(0U).View() = Canvas_Background_Merge_.RTV(0U);
			MergePass_.DepthStencil().View() = Canvas_Background_Merge_.DSV();
		}
		// PostProcessingPass
		{
			PostProcessingPass_.Initialize(1U, true);
			/*PostProcessingPass_.RenderTarget(0).BeginningEvent().ClearTarget(
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				{ 0.0f, 0.0f, 0.0f, 0.0f }
			);*/
			PostProcessingPass_.RenderTarget(0).BeginningEvent().Preserve();
			PostProcessingPass_.RenderTarget(0).EndingEvent().Preserve();
			/*PostProcessingPass_.DepthStencil().DepthBeginningEvent().ClearTarget(
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				{ .Depth{ 1.0f }, }
			);
			PostProcessingPass_.DepthStencil().DepthEndingEvent().Preserve();
			PostProcessingPass_.DepthStencil().StencilBeginningEvent().NoAccess();
			PostProcessingPass_.DepthStencil().StencilEndingEvent().NoAccess();*/

			//PostProcessingPass_.RenderTarget(0U).View() = Canvas_PostProcessing_.RTV(0U);
		}

		// PostProcessingConstants
		{
			UB_PostProcessingConstants_.Initialize(d3d12Device, 256LLU);
			GlobalTable_CBV_PostProcessing_ = d3d12Context.GlobalDescriptorHeap().Allocate(1U);
			Lumina::D3D12::CBV::Create(
				d3d12Device,
				GlobalTable_CBV_PostProcessing_.CPUHandle(0U),
				UB_PostProcessingConstants_
			);

			ClearPostProcessingConstants_.IsFadingOut = 0U;
			UB_PostProcessingConstants_.Store(
				&ClearPostProcessingConstants_,
				sizeof(ClearPostProcessingConstants),
				0LLU
			);
		}

		{
			PrimitiveManager0_.reset(new Lumina::PrimitiveManager{});
			PrimitiveManager0_->Initialize(d3d12Context);
			PrimitiveManager1_.reset(new Lumina::PrimitiveManager{});
			PrimitiveManager1_->Initialize(
				d3d12Context,
				L"Assets/Shaders/Primitive.VS.hlsl",
				L"Assets/Shaders/Primitive.PS.hlsl",
				true,
				false
			);
		}

		{
			UB_Dummy_.Initialize(d3d12Device, 256LLU, "Dummy");
			LocalHeap_Dummy_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1U, false);
			Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_Dummy_.CPUHandle(0U), UB_Dummy_);
		}
	}

	Clear::~Clear() noexcept {}
}

namespace Game::Scene {
	Clear::Clear() {
		Impl_.reset(new Impl::Clear{});
		Impl_->Initialize();
	}

	Clear::~Clear() {}
}