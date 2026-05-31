module Game.Scene.Title;

import : Impl;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.Main;
import Game.MathUtils;

namespace {
	constexpr D3D12_VIEWPORT DefaultViewport{
		.TopLeftX{ 0.0f },
		.TopLeftY{ 0.0f },
		.Width{ 1280.0f },
		.Height{ 720.0f },
		.MinDepth{ 0.0f },
		.MaxDepth{ 1.0f },
	};

	constexpr D3D12_RECT DefaultScissorRect{
		.left{ 0 },
		.top{ 0 },
		.right{ 1280 },
		.bottom{ 720 },
	};
}

//////	//////	//////	//////	//////	//////	//////	//////	//////
//////	Deferred.Geometry										//////
//////	//////	//////	//////	//////	//////	//////	//////	//////

namespace Game::Scene::Impl {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Deferred.Geometry.PreDraw								:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	template<>
	auto Title::Render_<"Deferred.Geometry.PreDraw.ResourceBarrier">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		std::vector<D3D12_RESOURCE_BARRIER> const barriers_PrePass{
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.RenderTexture(0U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.RenderTexture(1U),
				 D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.RenderTexture(2U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.DepthTexture(),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_DEPTH_WRITE
			 ),
		};
		cmdList_->ResourceBarrier(
			static_cast<Lumina::U32>(barriers_PrePass.size()),
			barriers_PrePass.data()
		);
	}

	template<>
	auto Title::Render_<"Deferred.Geometry.PreDraw.MeshBatch">(
		Lumina::MeshManager& meshMngr_
	) -> void {
		meshMngr_.BatchBegin();

		/*
		meshMngr_.Batch(
			MeshShaderAssets_[メッシュ番号],
			1U,
			LocalHeap_Materials_.CPUHandle(マテリアル番号),
			ワールド行列
		);
		*/

		meshMngr_.Batch(
			MeshShaderAssets_[0],
			1U,
			LocalHeap_Materials_.CPUHandle(0U),
			*UmbrellaRootWorld_
		);
		meshMngr_.Batch(
			MeshShaderAssets_[1],
			1U,
			LocalHeap_Materials_.CPUHandle(0U),
			*UmbrellaTipWorld_
		);

		meshMngr_.BatchEnd();
	}

	template<>
	auto Title::Render_<"Deferred.Geometry.PreDraw">(
		Lumina::D3D12::CommandList const& cmdList_,
		Lumina::MeshManager& meshMngr_
	) -> void {
		Render_<"Deferred.Geometry.PreDraw.ResourceBarrier">(cmdList_);
		Render_<"Deferred.Geometry.PreDraw.MeshBatch">(meshMngr_);
	}

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Deferred.Geometry.Draw									:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	template<>
	auto Title::Render_<"Grassland">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		auto const& context{ Lumina::Context::Instance() };
		Grassland_->Render(
			context.D3D12Context(),
			cmdList_,
			*WorldToHomogeneous_,
			{}, 0,
			{}, 0
		);
	}

	template<>
	auto Title::Render_<"Deferred.Geometry.Draw">(
		Lumina::D3D12::CommandList const& cmdList_,
		Lumina::MeshManager& meshMngr_
	) -> void {
		GeometryPass_.Begin(cmdList_);

		cmdList_->RSSetViewports(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.Viewports().data()
		);
		cmdList_->RSSetScissorRects(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.ScissorRects().data()
		);

		meshMngr_.Render(
			GraphicsPSO_MeshDeferredGeometry_,
			GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
			LocalHeap_Scene_.CPUHandle(0U)
		);

		Render_<"Grassland">(cmdList_);

		GeometryPass_.End();
	}

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	Deferred.Geometry.PostDraw								:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	template<>
	auto Title::Render_<"Deferred.Geometry.PostDraw.ResourceBarrier">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		std::vector<D3D12_RESOURCE_BARRIER> const barriers_PostPass{
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(1U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.RenderTexture(2U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			 ),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.DepthTexture(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
		};
		cmdList_->ResourceBarrier(
			static_cast<Lumina::U32>(barriers_PostPass.size()),
			barriers_PostPass.data()
		);
	}

	template<>
	auto Title::Render_<"Deferred.Geometry.PostDraw">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		Render_<"Deferred.Geometry.PostDraw.ResourceBarrier">(cmdList_);
	}

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	template<>
	auto Title::Render_<"Deferred.Geometry">() -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		auto& meshMngr{ Lumina::Context::Instance().MeshContext() };

		meshMngr.Begin(cmdList);

		Render_<"Deferred.Geometry.PreDraw">(cmdList, meshMngr);
		Render_<"Deferred.Geometry.Draw">(cmdList, meshMngr);

		Render_<"Deferred.Geometry.PostDraw">(cmdList);

		meshMngr.End();
	}
}

//////	//////	//////	//////	//////	//////	//////	//////	//////
//////	Deferred.Lighting										//////
//////	//////	//////	//////	//////	//////	//////	//////	//////

namespace Game::Scene::Impl {
	template<>
	auto Title::Render_<"Deferred.Lighting">(
		Lumina::D3D12::GraphicsDevice const& d3d12Device_,
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		DeferredLighting_->Render(
			d3d12Device_,
			cmdList_,
			GlobalTable_SRV_CanvasTexture_,
			// * WorldToProjective
			LocalHeap_Scene_.CPUHandle(0U),
			// * ScreenToWorld
			LocalHeap_Scene_.CPUHandle(1U)
		);
	}
}


//////	//////	//////	//////	//////	//////	//////	//////	//////
//////	IlluminatingObjects										//////
//////	//////	//////	//////	//////	//////	//////	//////	//////

namespace Game::Scene::Impl {

	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://
	//::::	ParticleEffects											:::://
	//::::	::::::	::::::	::::::	::::::	::::::	::::::	::::::	:::://

	template<>
	auto Title::Render_<"Merge.IlluminatingObjects.ParticleEffects">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		UmbrellaEffects_->Render(
			cmdList_,
			RS_ParticleSystem_,
			GraphicsPSO_BasicParticle_AdditiveMode_,
			LocalHeap_Scene_.CPUHandle(0U),
			LocalHeap_Scene_.CPUHandle(0U),
			GlobalTable_SRV_ImageTexture_,
			GlobalTable_SRV_CanvasTexture_
		);

		Raindrops_->Render(
			cmdList_,
			RS_ParticleSystem_,
			GraphicsPSO_BasicParticle_AdditiveMode_,
			LocalHeap_Scene_.CPUHandle(0U),
			LocalHeap_Scene_.CPUHandle(0U),
			GlobalTable_SRV_ImageTexture_,
			GlobalTable_SRV_CanvasTexture_
		);
		AmbientSparkles_->Render(
			cmdList_,
			RS_ParticleSystem_,
			GraphicsPSO_BasicParticle_AdditiveMode_,
			LocalHeap_Scene_.CPUHandle(0U),
			LocalHeap_Scene_.CPUHandle(0U),
			GlobalTable_SRV_ImageTexture_,
			GlobalTable_SRV_CanvasTexture_
		);
	}


	template<>
	auto Title::Render_<"Merge.IlluminatingObjects">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		auto rtv{ Canvas_Merge_.RTV(0) };
		cmdList_->OMSetRenderTargets(1U, &rtv, false, nullptr);

		Render_<"Merge.IlluminatingObjects.ParticleEffects">(cmdList_);
	}
}

namespace Game::Scene::Impl {
	template<>
	auto Title::Render_<"Skybox">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		auto const& swapChain{ Lumina::Context::Instance().D3D12Context().SwapChain() };
		auto rtv{ swapChain.BackBufferRTVCPUHandle() };
		cmdList_->OMSetRenderTargets(1U, &rtv, false, nullptr);
		Skybox_->Render(cmdList_, LocalHeap_Scene_.CPUHandle(0U));
	}
}

namespace Game::Scene::Impl {
	template<>
	auto Title::Render_<"Merge.PreDraw">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		std::vector<D3D12_RESOURCE_BARRIER> const barriers_PrePass{
			Lumina::D3D12::Barrier::Transition(
				Canvas_Merge_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			),
		};
		cmdList_->ResourceBarrier(
			static_cast<Lumina::U32>(barriers_PrePass.size()),
			barriers_PrePass.data()
		);
	}

	template<>
	auto Title::Render_<"Merge.PostDraw">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		std::vector<D3D12_RESOURCE_BARRIER> const barriers_PostPass{
			Lumina::D3D12::Barrier::Transition(
				Canvas_Merge_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
		};
		cmdList_->ResourceBarrier(
			static_cast<Lumina::U32>(barriers_PostPass.size()),
			barriers_PostPass.data()
		);
	}

	template<>
	auto Title::Render_<"Merge.UI">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		SpriteRenderer_->BatchBegin();
		{
			Lumina::Sprite caption{ TitleCaption_ };
			caption.Translate.X -= 5.0f;
			caption.Translate.Y -= 5.0f;
			caption.Scale.X *= 1.02f;
			caption.Scale.Y *= 1.02f;
			caption.RGBA.Z *= 0.5f;
			caption.RGBA.W *= 0.125f;
			SpriteRenderer_->Batch(caption);
		}
		{
			auto button{ UI_StartButton_ };
			button.Translate.X += 5.0f;
			button.Translate.Y += 5.0f;
			button.RGBA.X *= 0.25f;
			button.RGBA.Y *= 0.25f;
			button.RGBA.W *= 0.125f;
			SpriteRenderer_->Batch(button);
		}
		{
			auto button{ UI_ExitButton_ };
			button.Translate.X += 5.0f;
			button.Translate.Y += 5.0f;
			button.RGBA.X *= 0.25f;
			button.RGBA.Y *= 0.25f;
			button.RGBA.W *= 0.125f;
			SpriteRenderer_->Batch(button);
		}
		SpriteRenderer_->BatchEnd();

		auto rtv{ Canvas_Merge_.RTV(0) };
		cmdList_->OMSetRenderTargets(1U, &rtv, false, nullptr);
		SpriteRenderer_->Render(
			PSO_SpriteUI_,
			GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
			LocalHeap_OrthoProj_.CPUHandle(0U)
		);
	}

	template<>
	auto Title::Render_<"Merge">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		Render_<"Merge.PreDraw">(cmdList_);

		cmdList_->RSSetViewports(1U, &DefaultViewport);
		cmdList_->RSSetScissorRects(1U, &DefaultScissorRect);

		PrimitiveManager_->Begin(cmdList_);
		PrimitiveManager_->BatchTriangle(
			{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, 0U },
			{ { 3.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 2.0f, 0.0f }, 0U },
			{ { -1.0f, -3.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 2.0f }, 0U }
		);
		PrimitiveManager_->End(cmdList_);

		auto const& swapChain{ Lumina::Context::Instance().D3D12Context().SwapChain() };
		//MergePass_.RenderTarget(0).View() = swapChain.BackBufferRTVCPUHandle();
		MergePass_.DepthStencil().View() = swapChain.DSVCPUHandle();
		MergePass_.Begin(cmdList_);
		PrimitiveManager_->Render(
			cmdList_,
			GlobalTable_SRV_LightingResultTexture_,
			Lumina::Math::F32x4x4<>::Identity,
			1
		);
		MergePass_.End();

		Render_<"Merge.IlluminatingObjects">(cmdList_);
		Render_<"Merge.UI">(cmdList_);

		Render_<"Merge.PostDraw">(cmdList_);
	}
}

//////	//////	//////	//////	//////	//////	//////	//////	//////
//////	PrepareData												//////
//////	//////	//////	//////	//////	//////	//////	//////	//////

namespace Game::Scene::Impl {
	template<>
	auto Title::Render_<"PrepareData.Particle">() -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		UmbrellaEffects_->Update(
			cmdList,
			Lumina::Math::F32x4x4<>::Identity,
			[this] (Lumina::Particle& p_, void const*) -> bool {
				this->Update_<"UmbrellaEffectParticle">(p_);
				return (p_.Life > 0.0f);
			}
		);
		Raindrops_->Update(
			cmdList,
			Lumina::Math::F32x4x4<>::Identity,
			[this] (Lumina::Particle& p_, void const*) -> bool {
				this->Update_<"RaindropParticle">(p_);
				return (p_.Life > 0.0f);
			}
		);
		AmbientSparkles_->Update(
			cmdList,
			Lumina::Math::F32x4x4<>::Identity,
			[this] (Lumina::Particle& p_, void const*) -> bool {
				this->Update_<"AmbientSparkleParticle">(p_);
				return (p_.Life > 0.0f);
			}
		);
	}

	template<>
	auto Title::Render_<"PrepareData">() -> void {
		UB_WorldToProjective_.Store(WorldToHomogeneous_.get(), sizeof(Lumina::Math::F32x4x4<>), 0LLU);
		UB_ScreenToWorld_.Store(ScreenToWorld_.get(), sizeof(Lumina::Math::F32x4x4<>), 0LLU);

		Render_<"PrepareData.Particle">();
	}
}

namespace Game::Scene::Impl {
	template<>
	void Title::Render_<"Watercolor">() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		static auto const& tex_SubstrateAlbedo{
			*static_cast<Lumina::D3D12::ImageTexture const*>(
				Lumina::Context::Instance().ResourceContext().
				Graphics().GetResource("SubstrateAlbedo")
			)
		};
		static auto const& tex_SubstrateNormal{
			*static_cast<Lumina::D3D12::ImageTexture const*>(
				Lumina::Context::Instance().ResourceContext().
				Graphics().GetResource("SubstrateNormal")
			)
		};

		D3D12_RESOURCE_BARRIER const barriers_PreWatercolor[]{
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(2U),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.DepthTexture(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				tex_SubstrateAlbedo,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				tex_SubstrateNormal,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
		};
		cmdList->ResourceBarrier(5U, barriers_PreWatercolor);

		Watercolor_->Render(GlobalTable_SRV_GBufferForWaterColor_);

		D3D12_RESOURCE_BARRIER const barriers_PostWatercolor[]{
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(2U),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.DepthTexture(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				tex_SubstrateAlbedo,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				tex_SubstrateNormal,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
		};
		cmdList->ResourceBarrier(5U, barriers_PostWatercolor);

		cmdList->RSSetViewports(1U, &DefaultViewport);
		cmdList->RSSetScissorRects(1U, &DefaultScissorRect);

		PrimitiveManager2_->Begin(cmdList);
		PrimitiveManager2_->BatchTriangle(
			{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, 9U },
			{ { 3.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 2.0f, 0.0f }, 9U },
			{ { -1.0f, -3.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 2.0f }, 9U }
		);
		PrimitiveManager2_->End(cmdList);

		auto const& swapChain{ Lumina::Context::Instance().D3D12Context().SwapChain() };
		auto rtv = swapChain.BackBufferRTVCPUHandle();
		auto dsv = swapChain.DSVCPUHandle();
		cmdList->OMSetRenderTargets(1U, &rtv, false, &dsv);

		PrimitiveManager2_->Render(
			cmdList,
			Watercolor_->GlobalTable(),
			Lumina::Math::F32x4x4<>::Identity,
			1
		);
	}

	template<>
	auto Title::Render_<"UI">(
		Lumina::D3D12::CommandList const& cmdList_
	) -> void {
		SpriteRenderer_->BatchBegin();
		{
			SpriteRenderer_->Batch(TitleCaption_);
			SpriteRenderer_->Batch(UI_StartButton_);
			SpriteRenderer_->Batch(UI_ExitButton_);
		}
		SpriteRenderer_->BatchEnd();

		auto const& swapChain{ Lumina::Context::Instance().D3D12Context().SwapChain() };
		auto rtv = swapChain.BackBufferRTVCPUHandle();
		cmdList_->OMSetRenderTargets(1U, &rtv, false, nullptr);
		SpriteRenderer_->Render(
			PSO_SpriteUI_,
			GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
			LocalHeap_OrthoProj_.CPUHandle(0U)
		);
	}

	void Title::Render() {
		auto const& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		ID3D12DescriptorHeap* descriptorHeaps[]{
			Lumina::Context::Instance().D3D12Context().GlobalDescriptorHeap().Get(),
		};
		cmdList->SetDescriptorHeaps(1U, descriptorHeaps);

		SpriteRenderer_->Begin(cmdList);

		Lumina::Math::F32x4x4<> meshWorld{ Game::MathUtils::SRT(MeshScale_, MeshRotate_, MeshTranslate_) };
		Lumina::Math::F32x4x4<> tr_INV_MeshWorld{ meshWorld.Inverse().Transpose() };
		Lumina::Math::F32x4x4<> wvp{ meshWorld * (*WorldToHomogeneous_) };
		UB_Transforms_.Store(&wvp, sizeof(Lumina::Math::F32x4x4<>), 0LLU);
		UB_Transforms_.Store(&meshWorld, sizeof(Lumina::Math::F32x4x4<>), sizeof(Lumina::Math::F32x4x4<>));
		UB_Transforms_.Store(&tr_INV_MeshWorld, sizeof(Lumina::Math::F32x4x4<>), sizeof(Lumina::Math::F32x4x4<>) * 2);

		UB_WorldToProjective_.Store(WorldToHomogeneous_.get(), sizeof(Lumina::Math::F32x4x4<>), 0LLU);
		
		Render_<"PrepareData">();

		Render_<"Deferred.Geometry">();
		Render_<"Deferred.Lighting">(d3d12Device, cmdList);

		//TerrainRenderer_->DebugRenderCollidersBatch(*Terrain_);
		//TerrainRenderer_->DebugRenderColliders(GlobalTable_SRV_ImageTexture_, *WorldToHomogeneous_);

		//Render_<"Skybox">(cmdList);

		Render_<"Merge">(cmdList);
		Render_<"Watercolor">();
		Render_<"UI">(cmdList);

		SpriteRenderer_->End();
	}
}

namespace Game::Scene {
	void Title::Render() {
		Impl_->Render();
	}
}