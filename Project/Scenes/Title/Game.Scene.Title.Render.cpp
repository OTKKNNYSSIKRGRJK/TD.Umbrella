module Game.Scene.Title;

import : Impl;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.Main;
import Game.MathUtils;

namespace Game::Scene::Impl {
	void Title::Render_Geometry() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		//auto& meshMngr{ Lumina::Context::Instance().MeshContext() };

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

		cmdList->RSSetViewports(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.Viewports().data()
		);
		cmdList->RSSetScissorRects(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.ScissorRects().data()
		);

		cmdList->SetGraphicsRootSignature(RS_Skinning_.Get());
		cmdList->SetPipelineState(GraphicsPSO_SkinnedMeshDeferredGeometry_.Get());
		cmdList->SetGraphicsRootDescriptorTable(0U, GlobalTable_CBV_Scene_.GPUHandle(0U));
		cmdList->SetGraphicsRootDescriptorTable(1U, SkinCluster_.PaletteSRVHandle.second);
		cmdList->SetGraphicsRootDescriptorTable(2U, GlobalTable_Materials_.GPUHandle(0U));
		cmdList->SetGraphicsRootDescriptorTable(3U, GlobalTable_SRV_ImageTexture_.GPUHandle(0U));

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		D3D12_VERTEX_BUFFER_VIEW const vbvs[2]{
			reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW&>(VBV_),
			reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW&>(SkinCluster_.InfluenceBufferView)
		};
		cmdList->IASetVertexBuffers(0, 2, vbvs);
		cmdList->DrawInstanced(
			static_cast<Lumina::U32>(Collection_.Meshes[0].Vertices.size()),
			1U, 0U, 0U
		);

		GeometryPass_.Begin(cmdList);
		GeometryPass_.End();

		auto rtv{ Canvas_GeometryPass_.RTV(0U) };
		auto dsv{ Canvas_GeometryPass_.DSV() };
		cmdList->OMSetRenderTargets(1U, &rtv, false, &dsv);

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
	}

	void Title::Render_Merge() {
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
		PrimitiveManager_->Render(cmdList, GlobalTable_SRV_CanvasTexture_, Lumina::Math::F32x4x4<>::Identity, 1);
		MergePass_.End();
	}

	void Title::Render() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		ID3D12DescriptorHeap* descriptorHeaps[]{
			Lumina::Context::Instance().D3D12Context().GlobalDescriptorHeap().Get(),
		};
		cmdList->SetDescriptorHeaps(1U, descriptorHeaps);

		Lumina::Math::F32x4x4<> meshWorld{ Game::MathUtils::SRT(MeshScale_, MeshRotate_, MeshTranslate_) };
		Lumina::Math::F32x4x4<> tr_INV_MeshWorld{ meshWorld.Inverse().Transpose() };
		Lumina::Math::F32x4x4<> wvp{ meshWorld * (*WorldToHomogeneous_) };
		UB_Transforms_.Store(&wvp, sizeof(Lumina::Math::F32x4x4<>), 0LLU);
		UB_Transforms_.Store(&meshWorld, sizeof(Lumina::Math::F32x4x4<>), sizeof(Lumina::Math::F32x4x4<>));
		UB_Transforms_.Store(&tr_INV_MeshWorld, sizeof(Lumina::Math::F32x4x4<>), sizeof(Lumina::Math::F32x4x4<>) * 2);


		Render_Geometry();
		Render_Merge();
	}
}

namespace Game::Scene {
	void Title::Render() {
		Impl_->Render();
	}
}