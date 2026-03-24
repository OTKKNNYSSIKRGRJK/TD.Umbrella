module Game.Scene.InGame;

import : Impl;

import Lumina;

namespace Game::Scene::Impl {
	void InGame::Render() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		D3D12_RESOURCE_BARRIER const barriers_PreGeometryPass[]{
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_.RenderTexture(0U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_.RenderTexture(1U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_.DepthTexture(),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_DEPTH_WRITE
			 ),
		};
		cmdList->ResourceBarrier(3U, barriers_PreGeometryPass);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2]{ Canvas_.RTV(0U), Canvas_.RTV(1U) };
		auto dsv{ Canvas_.DSV() };
		cmdList->OMSetRenderTargets(2U, rtvs, false, &dsv);

		cmdList->RSSetViewports(
			Canvas_.Num_RenderTargets(),
			Canvas_.Viewports().data()
		);
		cmdList->RSSetScissorRects(
			Canvas_.Num_RenderTargets(),
			Canvas_.ScissorRects().data()
		);

		Test_->Render();

		D3D12_RESOURCE_BARRIER const barriers_PostGeometryPass[]{
			Lumina::D3D12::Barrier::Transition(
				Canvas_.RenderTexture(0U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_.RenderTexture(1U),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			Lumina::D3D12::Barrier::Transition(
				Canvas_.DepthTexture(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
		};
		cmdList->ResourceBarrier(3U, barriers_PostGeometryPass);
	}
}

namespace Game::Scene {
	void InGame::Render() {
		Impl_->Render();
	}
}