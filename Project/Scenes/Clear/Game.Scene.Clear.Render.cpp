module Game.Scene.Clear;

import : Impl;

import Lumina.Main;
import Lumina.D3D12;
import Lumina.Core.Math;

namespace Game::Scene::Impl {
	namespace {
		constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };

		Lumina::Math::F32x2 Delta_Center{};

		auto UpdateCircularSparkle(
			Lumina::Particle& p_,
			[[maybe_unused]] void const* dummy_
		) -> bool {
			p_.Translate.X += p_.Velocity.X + Delta_Center.X;
			p_.Translate.Y += p_.Velocity.Y + Delta_Center.Y;
			p_.Scale.X *= 0.97f;
			p_.Scale.Y *= 0.98f;
			p_.Rotate.Z = std::atan2(p_.Velocity.Y, p_.Velocity.X) - Lumina::Math::Constant::Pi * 0.5f;
			p_.Velocity.Y += 0.05f;
			p_.RenderData.RGBA.W = std::min<float>(p_.RenderData.RGBA.W + 0.015f, 0.15f);
			p_.Life -= 1.0f;

			if (p_.Life > 0.0f) { return true; }
			return false;
		}
	}

	void Clear::Render() {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		//auto const& d3d12Device{ d3d12Context.Device() };
		auto const& cmdList{ context.MainCommandList() };

		ID3D12DescriptorHeap* descriptorHeaps[]{ d3d12Context.GlobalDescriptorHeap().Get() };
		CmdList_->SetDescriptorHeaps(1U, descriptorHeaps);

		Delta_Center = BackgroundCenter_ - BackgroundCenter_Prev_;

		SpriteRenderer_->Begin(CmdList_);

		CircularSparkles_->Update(
			CmdList_,
			Lumina::Math::F32x4x4<>::Identity,
			UpdateCircularSparkle
		);

		// Geometry pass
		{
			D3D12_RESOURCE_BARRIER const barriers_PreGeometryPass[]{
				 Lumina::D3D12::Barrier::Transition(
					 Canvas_Geometry_.RenderTexture(0U),
					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					 D3D12_RESOURCE_STATE_RENDER_TARGET
				 ),
				 Lumina::D3D12::Barrier::Transition(
					 Canvas_Geometry_.RenderTexture(1U),
					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					 D3D12_RESOURCE_STATE_RENDER_TARGET
				 ),
				 Lumina::D3D12::Barrier::Transition(
					 Canvas_Geometry_.DepthTexture(),
					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					 D3D12_RESOURCE_STATE_DEPTH_WRITE
				 ),
			};
			CmdList_->ResourceBarrier(3U, barriers_PreGeometryPass);

			CmdList_->RSSetViewports(
				Canvas_Geometry_.Num_RenderTargets(),
				Canvas_Geometry_.Viewports().data()
			);
			CmdList_->RSSetScissorRects(
				Canvas_Geometry_.Num_RenderTargets(),
				Canvas_Geometry_.ScissorRects().data()
			);

			SpriteRenderer_->BatchBegin();
			{
				auto label{ UI_Label_StageClear_ };
				label.Scale.X *= 1.05f;
				label.Scale.Y *= 1.05f;
				SpriteRenderer_->Batch(label);
			}
			{
				auto label{ UI_Label_PressSpaceKey_ };
				label.Scale.X *= 1.05f;
				label.Scale.Y *= 1.05f;
				SpriteRenderer_->Batch(label);
			}
			SpriteRenderer_->BatchEnd();

			DeferredGeometryPass_.Begin(CmdList_);
			{
				SpriteRenderer_->Render(
					PSO_Sprite_,
					GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
					LocalHeap_OrthoProj_.CPUHandle(0U)
				);
			}
			DeferredGeometryPass_.End();

			D3D12_RESOURCE_BARRIER const barriers_PostGeometryPass[]{
				Lumina::D3D12::Barrier::Transition(
					Canvas_Geometry_.RenderTexture(0U),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				),
				Lumina::D3D12::Barrier::Transition(
					Canvas_Geometry_.RenderTexture(1U),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				),
				Lumina::D3D12::Barrier::Transition(
					Canvas_Geometry_.DepthTexture(),
					D3D12_RESOURCE_STATE_DEPTH_WRITE,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				),
			};
			CmdList_->ResourceBarrier(3U, barriers_PostGeometryPass);
		}

		// Lighting pass
		{
			DeferredLighting_->Render(
				d3d12Context.Device(),
				CmdList_,
				GlobalTable_SRV_CanvasTexture_,
				LocalHeap_OrthoProj_.CPUHandle(0U),
				LocalHeap_OrthoProj_.CPUHandle(0U)
			);
		}

		// Merge pass
		{
			D3D12_RESOURCE_BARRIER const barriers_PreMergePass[]{
				 Lumina::D3D12::Barrier::Transition(
					 Canvas_Background_Merge_.RenderTexture(0U),
					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					 D3D12_RESOURCE_STATE_RENDER_TARGET
				 ),
				 Lumina::D3D12::Barrier::Transition(
					 Canvas_Background_Merge_.DepthTexture(),
					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					 D3D12_RESOURCE_STATE_DEPTH_WRITE
				 ),
			};
			CmdList_->ResourceBarrier(2U, barriers_PreMergePass);

			PrimitiveManager0_->Begin(CmdList_);
			PrimitiveManager0_->BatchTriangle(
				{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 0.2f, 0.4f, 0.5f, 0.125f }, { 0.015625f, 0.015625f }, 4U },
				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.3f, 0.4f, 0.5f, 0.125f }, { 0.984375f, 0.015625f }, 4U },
				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.3f, 0.3f, 0.5f, 0.125f }, { 0.015625f, 0.984375f }, 4U }
			);
			PrimitiveManager0_->BatchTriangle(
				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.3f, 0.4f, 0.5f, 0.125f }, { 0.984375f, 0.015625f }, 4U },
				{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 0.25f, 0.5f, 0.5f, 0.125f }, { 0.984375f, 0.984375f }, 4U },
				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.3f, 0.3f, 0.5f, 0.125f }, { 0.015625f, 0.984375f }, 4U }
			);
			PrimitiveManager0_->BatchTriangle(
				{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 0.0f, 0.0f }, 4U },
				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 1.0f, 0.0f }, 4U },
				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 0.0f, 1.0f }, 4U }
			);
			PrimitiveManager0_->BatchTriangle(
				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 1.0f, 0.0f }, 4U },
				{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 1.0f, 1.0f }, 4U },
				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.1875f }, { 0.0f, 1.0f }, 4U }
			);
			PrimitiveManager0_->End(CmdList_);

			SpriteRenderer_->BatchBegin();
			SpriteRenderer_->Batch(UI_Label_StageClear_);
			SpriteRenderer_->Batch(UI_Label_PressSpaceKey_);
			SpriteRenderer_->BatchEnd();

			MergePass_.Begin(CmdList_);
			{
				PrimitiveManager0_->Render(CmdList_, GlobalTable_SRV_CanvasTexture_, {}, 1);

				CircularSparkles_->Render(
					CmdList_,
					RS_ParticleSystem_,
					GraphicsPSO_BasicParticle_AdditiveMode_,
					LocalHeap_Dummy_.CPUHandle(0U),
					LocalHeap_OrthoProj_.CPUHandle(0U),
					GlobalTable_SRV_ImageTexture_,
					GlobalTable_SRV_ImageTexture_
				);

				SpriteRenderer_->Render(
					PSO_SpriteUI_,
					GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
					LocalHeap_OrthoProj_.CPUHandle(0U)
				);
			}
			MergePass_.End();

			D3D12_RESOURCE_BARRIER const barriers_PostMergePass[]{
				 Lumina::D3D12::Barrier::Transition(
					 Canvas_Background_Merge_.RenderTexture(0U),
					 D3D12_RESOURCE_STATE_RENDER_TARGET,
					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				 ),
				 Lumina::D3D12::Barrier::Transition(
					 Canvas_Background_Merge_.DepthTexture(),
					 D3D12_RESOURCE_STATE_DEPTH_WRITE,
					 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				 ),
			};
			CmdList_->ResourceBarrier(2U, barriers_PostMergePass);
		}

		SpriteRenderer_->End();

		Lumina::D3D12::CommandQueue& directQueue{ d3d12Context.DirectQueue() };
		directQueue.BatchCommandList(CmdList_);
		directQueue.CPUWait(directQueue.ExecuteBatchedCommandLists());
		CmdList_.Reset(CmdAllocator_);

		// Post-processing pass
		{
			float alpha = 0.75f;
			if (Count_FadeIn_ > -1) {
				float const t{ static_cast<float>(72 - Count_FadeIn_) / 72.0f };
				alpha = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f) * 0.75f;
			}
			else if (Count_FadeOut_ > -1) {
				float const t{ static_cast<float>(72 - Count_FadeOut_) / 72.0f };
				alpha = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f) * 0.75f;
			}
			PrimitiveManager1_->Begin(cmdList);
			PrimitiveManager1_->BatchTriangle(
				{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 0.0f, 0.0f }, 5U },
				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 1.0f, 0.0f }, 5U },
				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 0.0f, 1.0f }, 5U }
			);
			PrimitiveManager1_->BatchTriangle(
				{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 1.0f, 0.0f }, 5U },
				{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 1.0f, 1.0f }, 5U },
				{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, alpha }, { 0.0f, 1.0f }, 5U }
			);
			PrimitiveManager1_->End(cmdList);

			PostProcessingPass_.RenderTarget(0U).View() = d3d12Context.SwapChain().BackBufferRTVCPUHandle();
			PostProcessingPass_.DepthStencil().View() = d3d12Context.SwapChain().DSVCPUHandle();
			PostProcessingPass_.Begin(cmdList);
			{
				PrimitiveManager1_->Render(
					cmdList,
					GlobalTable_SRV_CanvasTexture_,
					Lumina::Math::F32x4x4<>::Identity,
					1,
					GlobalTable_CBV_PostProcessing_.GPUHandle(0U)
				);
			}
			PostProcessingPass_.End();
		}
	}
}

namespace Game::Scene {
	void Clear::Render() {
		Impl_->Render();
	}
}