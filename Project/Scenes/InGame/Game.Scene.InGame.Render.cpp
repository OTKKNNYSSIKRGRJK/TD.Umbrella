module Game.Scene.InGame;

import : Impl;

import Lumina.Main;
import Lumina.D3D12;
import Lumina.MeshManager;
import Lumina.Primitive;

namespace Game::Scene::Impl {
	void InGame::Render_Geometry() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		auto& meshMngr{ Lumina::Context::Instance().MeshContext() };

		meshMngr.Begin(cmdList);

		meshMngr.BatchBegin();

		D3D12_RESOURCE_BARRIER const barriers_PreGeometryPass[]{
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.RenderTexture(0U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.RenderTexture(1U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_GeometryPass_.DepthTexture(),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_DEPTH_WRITE
			 ),
		};
		cmdList->ResourceBarrier(3U, barriers_PreGeometryPass);

		/*D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2]{ Canvas_.RTV(0U), Canvas_.RTV(1U) };
		auto dsv{ Canvas_.DSV() };
		cmdList->OMSetRenderTargets(2U, rtvs, false, &dsv);*/

		cmdList->RSSetViewports(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.Viewports().data()
		);
		cmdList->RSSetScissorRects(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.ScissorRects().data()
		);

		// メッシュバッチ処理
		/*
		MeshManager_->Batch(
			MeshShaderAssets_[メッシュ番号],
			1U,
			LocalHeap_Materials_.CPUHandle(マテリアル番号),
			ワールド行列
		);
		*/

		meshMngr.Batch(
			MeshShaderAssets_[0],
			1U,
			LocalHeap_Materials_.CPUHandle(0U),
			Lumina::Math::F32x4x4<>::Identity
		);

		meshMngr.BatchEnd();

		//auto rtv{ DXContext_->SwapChain().BackBufferRTVCPUHandle() };
		//auto dsv{ DXContext_->SwapChain().DSVCPUHandle() };
		GeometryPass_.Begin(cmdList);
		meshMngr.Render(
			GraphicsPSO_MeshDeferredGeometry_,
			GlobalTable_SRV_ImageTexture_.GPUHandle(0U),
			LocalHeap_Scene_.CPUHandle(0U)
		);
		GeometryPass_.End();

		D3D12_RESOURCE_BARRIER const barriers_PostGeometryPass[]{
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.RenderTexture(1U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_GeometryPass_.DepthTexture(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
		};
		cmdList->ResourceBarrier(3U, barriers_PostGeometryPass);

		meshMngr.End();
	}

	void InGame::Render_Merge() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		cmdList->RSSetViewports(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.Viewports().data()
		);
		cmdList->RSSetScissorRects(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.ScissorRects().data()
		);

		PrimitiveManager_->Begin(cmdList);
		PrimitiveManager_->BatchTriangle(
			{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, 0U },
			{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, 0U },
			{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, 0U }
		);
		PrimitiveManager_->BatchTriangle(
			{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, 0U },
			{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }, 0U },
			{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, 0U }
		);
		PrimitiveManager_->End(cmdList);

		auto const& swapChain{ Lumina::Context::Instance().D3D12Context().SwapChain() };
		MergePass_.RenderTarget(0).View() = swapChain.BackBufferRTVCPUHandle();
		MergePass_.DepthStencil().View() = swapChain.DSVCPUHandle();
		MergePass_.Begin(cmdList);
		PrimitiveManager_->Render(cmdList, GlobalTable_SRV_CanvasTexture_, {}, 1);
		MergePass_.End();
	}

	void InGame::Render() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		ID3D12DescriptorHeap* descriptorHeaps[]{
			Lumina::Context::Instance().D3D12Context().GlobalDescriptorHeap().Get(),
		};
		cmdList->SetDescriptorHeaps(1U, descriptorHeaps);

		Render_Geometry();
		Render_Merge();
	}
}

namespace Game::Scene {
	void InGame::Render() {
		Impl_->Render();
	}
}