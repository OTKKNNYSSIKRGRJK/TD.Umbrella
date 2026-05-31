//module Game.Scene.Clear;
//
//import : Impl;
//
//namespace Game::Scene {
//	namespace {
//		constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };
//
//		Lumina::Vec2 Delta_Center{};
//
//		bool UpdateCircularSparkle(Particle& p_, void const*) {
//			p_.Translate.x += p_.Velocity.x + Delta_Center.x;
//			p_.Translate.y += p_.Velocity.y + Delta_Center.y;
//			p_.Scale.x *= 0.97f;
//			p_.Scale.y *= 0.98f;
//			p_.Rotate.z = std::atan2(p_.Velocity.y, p_.Velocity.x) - std::numbers::pi_v<float> * 0.5f;
//			p_.Velocity.y += 0.05f;
//			p_.RenderData.RGBA.w = std::min<float>(p_.RenderData.RGBA.w + 0.015f, 0.15f);
//			p_.Life -= 1.0f;
//			return (p_.Life > 0.0f);
//		}
//	}
//
//	void ClearImpl::Render() {
//		ID3D12DescriptorHeap* descriptorHeaps[]{ DXContext_->GlobalDescriptorHeap().Get() };
//		CmdList_->SetDescriptorHeaps(1U, descriptorHeaps);
//
//		Delta_Center = BackgroundCenter_ - BackgroundCenter_Prev_;
//
//		SpriteRenderer_->Begin(CmdList_);
//
//		CircularSparkles_->Update(CmdList_, UpdateCircularSparkle);
//
//		// Geometry pass
//		{
//			D3D12_RESOURCE_BARRIER const barriers_PreGeometryPass[]{
//				 Lumina::DX12::Barrier::Transition(
//					 Canvas_Geometry_.RenderTexture(0U),
//					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
//					 D3D12_RESOURCE_STATE_RENDER_TARGET
//				 ),
//				 Lumina::DX12::Barrier::Transition(
//					 Canvas_Geometry_.RenderTexture(1U),
//					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
//					 D3D12_RESOURCE_STATE_RENDER_TARGET
//				 ),
//				 Lumina::DX12::Barrier::Transition(
//					 Canvas_Geometry_.DepthTexture(),
//					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
//					 D3D12_RESOURCE_STATE_DEPTH_WRITE
//				 ),
//			};
//			CmdList_->ResourceBarrier(3U, barriers_PreGeometryPass);
//
//			CmdList_->RSSetViewports(
//				Canvas_Geometry_.Num_RenderTargets(),
//				Canvas_Geometry_.Viewports().data()
//			);
//			CmdList_->RSSetScissorRects(
//				Canvas_Geometry_.Num_RenderTargets(),
//				Canvas_Geometry_.ScissorRects().data()
//			);
//
//			MeshManager_->Begin(CmdList_);
//			MeshManager_->BatchBegin();
//			for (auto const& localToWorld : Arr_MeshLocalToWorld_) {
//				MeshManager_->Batch(
//					MeshShaderAssets_[MSA_Box],
//					1U,
//					LocalHeap_Arr_CBV_MeshMaterial_.CPUHandle(MM_Box),
//					localToWorld
//				);
//			}
//			MeshManager_->BatchEnd();
//
//			SpriteRenderer_->BatchBegin(); 
//			{
//				auto label{ UI_Label_StageClear_ };
//				label.Scale_.x *= 1.05f;
//				label.Scale_.y *= 1.05f;
//				SpriteRenderer_->Batch(label);
//			}
//			{
//				auto label{ UI_Label_PressSpaceKey_ };
//				label.Scale_.x *= 1.05f;
//				label.Scale_.y *= 1.05f;
//				SpriteRenderer_->Batch(label);
//			}
//			SpriteRenderer_->BatchEnd();
//
//			DeferredGeometryPass_.Begin(CmdList_);
//			{
//				MeshManager_->Render(
//					GraphicsPSO_MeshDeferredGeometry_,
//					GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
//					LocalHeap_OrthoProj_.CPUHandle(0U)
//				);
//				SpriteRenderer_->Render(
//					PSO_Sprite_,
//					GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
//					LocalHeap_OrthoProj_.CPUHandle(0U)
//				);
//			}
//			DeferredGeometryPass_.End();
//
//			MeshManager_->End();
//
//			D3D12_RESOURCE_BARRIER const barriers_PostGeometryPass[]{
//				Lumina::DX12::Barrier::Transition(
//					Canvas_Geometry_.RenderTexture(0U),
//					D3D12_RESOURCE_STATE_RENDER_TARGET,
//					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
//				),
//				Lumina::DX12::Barrier::Transition(
//					Canvas_Geometry_.RenderTexture(1U),
//					D3D12_RESOURCE_STATE_RENDER_TARGET,
//					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
//				),
//				Lumina::DX12::Barrier::Transition(
//					Canvas_Geometry_.DepthTexture(),
//					D3D12_RESOURCE_STATE_DEPTH_WRITE,
//					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
//				),
//			};
//			CmdList_->ResourceBarrier(3U, barriers_PostGeometryPass);
//		}
//
//		// Lighting pass
//		{
//			DeferredLighting_->Render(
//				DXContext_->Device(),
//				CmdList_,
//				GlobalTable_SRV_CanvasTexture_,
//				LocalHeap_OrthoProj_.CPUHandle(0U),
//				LocalHeap_OrthoProj_.CPUHandle(0U)
//			);
//		}
//
//		// Merge pass
//		{
//			D3D12_RESOURCE_BARRIER const barriers_PreMergePass[]{
//				 Lumina::DX12::Barrier::Transition(
//					 Canvas_Background_Merge_.RenderTexture(0U),
//					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
//					 D3D12_RESOURCE_STATE_RENDER_TARGET
//				 ),
//				 Lumina::DX12::Barrier::Transition(
//					 Canvas_Background_Merge_.DepthTexture(),
//					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
//					 D3D12_RESOURCE_STATE_DEPTH_WRITE
//				 ),
//			};
//			CmdList_->ResourceBarrier(2U, barriers_PreMergePass);
//
//			PrimitiveManager0_->Begin(CmdList_);
//			PrimitiveManager0_->BatchTriangle(
//				{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 0.2f, 0.4f, 0.5f, 0.125f }, { 0.015625f, 0.015625f }, 3U },
//				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.3f, 0.4f, 0.5f, 0.125f }, { 0.984375f, 0.015625f }, 3U },
//				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.3f, 0.3f, 0.5f, 0.125f }, { 0.015625f, 0.984375f }, 3U }
//			);
//			PrimitiveManager0_->BatchTriangle(
//				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.3f, 0.4f, 0.5f, 0.125f }, { 0.984375f, 0.015625f }, 3U },
//				{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 0.25f, 0.5f, 0.5f, 0.125f }, { 0.984375f, 0.984375f }, 3U },
//				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.3f, 0.3f, 0.5f, 0.125f }, { 0.015625f, 0.984375f }, 3U }
//			);
//			PrimitiveManager0_->BatchTriangle(
//				{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 0.0f, 0.0f }, 3U },
//				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 1.0f, 0.0f }, 3U },
//				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 0.0f, 1.0f }, 3U }
//			);
//			PrimitiveManager0_->BatchTriangle(
//				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 1.0f, 0.0f }, 3U },
//				{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 1.0f, 1.0f }, 3U },
//				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 0.0f, 1.0f }, 3U }
//			);
//			PrimitiveManager0_->End(CmdList_);
//			
//			SpriteRenderer_->BatchBegin();
//			SpriteRenderer_->Batch(UI_Label_StageClear_);
//			SpriteRenderer_->Batch(UI_Label_PressSpaceKey_);
//			SpriteRenderer_->BatchEnd();
//
//			MergePass_.Begin(CmdList_);
//			{
//				PrimitiveManager0_->Render(CmdList_, GlobalTable_SRV_CanvasTexture_, {}, 1);
//
//				CircularSparkles_->Render(
//					CmdList_,
//					RS_ParticleSystem_,
//					GraphicsPSO_BasicParticle_AdditiveMode_,
//					LocalHeap_Dummy_.CPUHandle(0U),
//					LocalHeap_OrthoProj_.CPUHandle(0U),
//					GlobalTable_SRV_ImageTexture_
//				);
//
//				SpriteRenderer_->Render(
//					PSO_SpriteUI_,
//					GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
//					LocalHeap_OrthoProj_.CPUHandle(0U)
//				);
//			}
//			MergePass_.End();
//
//			D3D12_RESOURCE_BARRIER const barriers_PostMergePass[]{
//				 Lumina::DX12::Barrier::Transition(
//					 Canvas_Background_Merge_.RenderTexture(0U),
//					 D3D12_RESOURCE_STATE_RENDER_TARGET,
//					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
//				 ),
//				 Lumina::DX12::Barrier::Transition(
//					 Canvas_Background_Merge_.DepthTexture(),
//					 D3D12_RESOURCE_STATE_DEPTH_WRITE,
//					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
//				 ),
//			};
//			CmdList_->ResourceBarrier(2U, barriers_PostMergePass);
//		}
//
//		SpriteRenderer_->End();
//
//		Lumina::DX12::CommandQueue& directQueue{ DXContext_->DirectQueue() };
//		directQueue.BatchCommandList(CmdList_);
//		directQueue.CPUWait(directQueue.ExecuteBatchedCommandLists());
//		CmdList_.Reset(CmdAllocator_);
//
//		// Post-processing pass
//		{
//			float alpha = 0.75f;
//			if (Count_FadeIn_ > -1) {
//				float const t{ static_cast<float>(72 - Count_FadeIn_) / 72.0f };
//				alpha = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f) * 0.75f;
//			}
//			else if (Count_FadeOut_ > -1) {
//				float const t{ static_cast<float>(72 - Count_FadeOut_) / 72.0f };
//				alpha = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f) * 0.75f;
//			}
//			PrimitiveManager1_->Begin(*CmdList_Main_);
//			PrimitiveManager1_->BatchTriangle(
//				{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 0.0f, 0.0f }, 4U },
//				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 1.0f, 0.0f }, 4U },
//				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 0.0f, 1.0f }, 4U }
//			);
//			PrimitiveManager1_->BatchTriangle(
//				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 1.0f, 0.0f }, 4U },
//				{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 1.0f, 1.0f }, 4U },
//				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 0.0f, 1.0f }, 4U }
//			);
//			PrimitiveManager1_->End(*CmdList_Main_);
//
//			PostProcessingPass_.RenderTarget(0U).View() = DXContext_->SwapChain().BackBufferRTVCPUHandle();
//			PostProcessingPass_.DepthStencil().View() = DXContext_->SwapChain().DSVCPUHandle();
//			PostProcessingPass_.Begin(*CmdList_Main_);
//			{
//				PrimitiveManager1_->Render(
//					*CmdList_Main_,
//					GlobalTable_SRV_CanvasTexture_,
//					{},
//					1,
//					GlobalTable_CBV_PostProcessing_.GPUHandle(0U)
//				);
//			}
//			PostProcessingPass_.End();
//		}
//	}
//
//	void Clear::Render() {
//		Impl_->Render();
//	}
//}