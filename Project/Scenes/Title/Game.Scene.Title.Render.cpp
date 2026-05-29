module Game.Scene.Title;

import : Impl;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.Main;
import Game.MathUtils;

namespace Game::Scene::Impl {
	template<>
	void Title::Render_<"Grassland">() {
		auto const& context{ Lumina::Context::Instance() };
		auto const& cmdList{ context.MainCommandList() };
		Grassland_->Render(
			context.D3D12Context(),
			cmdList,
			*WorldToHomogeneous_,
			{}, 0,
			{}, 0
		);
	}
	template<>
	void Title::Render_<"SceneParticles">() {
		auto const& context{ Lumina::Context::Instance() };
		auto const& cmdList{ context.MainCommandList() };

		// * 違うレンダラでもパイプラインが同じなら引数はそのままで大丈夫
		Raindrops_->Render(
			cmdList,
			// * ルートシグネチャ
			RS_ParticleSystem_,
			// * パイプラインステートオブジェクト
			GraphicsPSO_BasicParticle_AdditiveMode_,
			// * 定数バッファ
			LocalHeap_CBV_.CPUHandle(0U),
			// * 定数バッファ
			LocalHeap_CBV_.CPUHandle(0U),
			// * パーティクル画像
			GlobalTable_SRV_ImageTexture_,
			// * オフスクリーンバッファ
			GlobalTable_SRV_CanvasTexture_
		);
	}

	template<>
	void Title::Render_<"Geometry">() {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		//auto& meshMngr{ Lumina::Context::Instance().MeshContext() };

		Raindrops_->Update(
			cmdList,
			Lumina::Math::F32x4x4<>::Identity,
			[] (Lumina::Particle& p_, void const*) {
				p_.Translate.X += p_.Velocity.X;
				p_.Translate.Y += p_.Velocity.Y;
				p_.Translate.Z += p_.Velocity.Z;
				p_.Life -= 1.0f;
				return (p_.Life > 0.0f);
			}
		);

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
		cmdList->ResourceBarrier(4U, barriers_PreGeometryPass);

		cmdList->RSSetViewports(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.Viewports().data()
		);
		cmdList->RSSetScissorRects(
			Canvas_GeometryPass_.Num_RenderTargets(),
			Canvas_GeometryPass_.ScissorRects().data()
		);

		/*auto rtv{ Canvas_GeometryPass_.RTV(0U) };
		auto dsv{ Canvas_GeometryPass_.DSV() };
		cmdList->OMSetRenderTargets(1U, &rtv, false, &dsv);*/

		GeometryPass_.Begin(cmdList);

		cmdList->SetGraphicsRootSignature(RS_Skinning_.Get());
		cmdList->SetPipelineState(GraphicsPSO_SkinnedMeshDeferredGeometry_.Get());
		cmdList->SetGraphicsRootDescriptorTable(0U, GlobalTable_CBV_Scene_.GPUHandle(0U));
		cmdList->SetGraphicsRootDescriptorTable(1U, SkinCluster_.PaletteSRVHandle.second);
		cmdList->SetGraphicsRootDescriptorTable(2U, GlobalTable_Materials_.GPUHandle(0U));
		cmdList->SetGraphicsRootDescriptorTable(3U, GlobalTable_SRV_ImageTexture_.GPUHandle(0U));
		cmdList->SetGraphicsRootDescriptorTable(5U, Skybox_->GlobalTable().GPUHandle(0U));
		auto const& cameraPos{ Camera_->WorldPosition() };
		cmdList->SetGraphicsRoot32BitConstants(6U, 3U, &cameraPos, 0U);

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		D3D12_VERTEX_BUFFER_VIEW const vbvs[2]{
			reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW&>(VBV_),
			reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW&>(SkinCluster_.InfluenceBufferView)
		};
		cmdList->IASetVertexBuffers(0, 2, vbvs);
		cmdList->IASetIndexBuffer(reinterpret_cast<D3D12_INDEX_BUFFER_VIEW const*>(&IBV_));
		cmdList->DrawIndexedInstanced(
			static_cast<Lumina::U32>(Collection_.Meshes[0].Indices.size()),
			1U, 0U, 0U, 0U
		);

		// * `GeometryPass_.Begin(cmdList);`と`GeometryPass_.End();`の間に書かないとダメ
		Render_<"Grassland">();
		// * `GeometryPass_.Begin(cmdList);`と`GeometryPass_.End();`の間に書かないとダメ
		Render_<"SceneParticles">();

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
		cmdList->ResourceBarrier(4U, barriers_PostGeometryPass);
	}

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
	}

	template<>
	void Title::Render_<"Merge">() {
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
		auto idx{ static_cast<Lumina::U32>(Lumina::Watercolor::VIEW_NAME::SRV_COMPOSITE) };
		PrimitiveManager_->BatchTriangle(
			{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, idx },
			{ { 3.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 2.0f, 0.0f }, idx },
			{ { -1.0f, -3.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 2.0f }, idx }
		);
		PrimitiveManager_->End(cmdList);

		auto const& swapChain{ Lumina::Context::Instance().D3D12Context().SwapChain() };
		MergePass_.RenderTarget(0).View() = swapChain.BackBufferRTVCPUHandle();
		MergePass_.DepthStencil().View() = swapChain.DSVCPUHandle();
		MergePass_.Begin(cmdList);
		PrimitiveManager_->Render(cmdList, Watercolor_->GlobalTable(), Lumina::Math::F32x4x4<>::Identity, 1);
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

		UB_WorldToProjective_.Store(WorldToHomogeneous_.get(), sizeof(Lumina::Math::F32x4x4<>), 0LLU);

		Render_<"Geometry">();
		Render_<"Watercolor">();
		Render_<"Merge">();
	}
}

namespace Game::Scene {
	void Title::Render() {
		Impl_->Render();
	}
}