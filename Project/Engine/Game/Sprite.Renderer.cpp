module Lumina.Sprite;

import nlohmann.json;

import Lumina.Core.Common;
import Lumina.Core.Math;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import Lumina.Utils.Data;

namespace Lumina {
	namespace {
		auto SRT(
			Math::F32x3 const& scale_,
			Math::F32x3 const& rotate_,
			Math::F32x3 const& translate_
		) -> Math::F32x4x4<> {
			F32 const
				cosAlpha{ Math::COS(rotate_.X) },
				sinAlpha{ Math::SIN(rotate_.X) },
				cosBeta{ Math::COS(rotate_.Y) },
				sinBeta{ Math::SIN(rotate_.Y) },
				cosGamma{ Math::COS(rotate_.Z) },
				sinGamma{ Math::SIN(rotate_.Z) };

			Math::F32x4x4<> srt{
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
}

namespace Lumina {
	void SpriteRenderer::Initialize(
			D3D12::Context const& d3d12Context_,
			uint32_t maxNum_Batches_
	) {
		MaxNum_Batches_ = std::clamp(maxNum_Batches_, 256U, 4096U);

		D3D12Context_ = &d3d12Context_;

		auto const& device{ D3D12Context_->Device() };
		auto const& globalHeap{ D3D12Context_->GlobalDescriptorHeap() };

		globalHeap.Allocate(Table_Materials_, 1U);
		globalHeap.Allocate(Table_UVs_, 1U);
		globalHeap.Allocate(Table_VP_And_Worlds_, 2U);

		DB_VP_.Initialize(device, 256LLU);
		UB_VP_.Initialize(device, DB_VP_.SizeInBytes());
		D3D12::CBV::Create(device, Table_VP_And_Worlds_.CPUHandle(0U), DB_VP_);
		DB_Worlds_.Initialize(device, sizeof(Math::F32x4x4<>) * MaxNum_Batches_);
		UB_Worlds_.Initialize(device, DB_Worlds_.SizeInBytes());
		D3D12::SRV<Math::F32x4x4<>>::Create(device, Table_VP_And_Worlds_.CPUHandle(1U), DB_Worlds_);

		DB_UVs_.Initialize(device, sizeof(QuadUVs) * MaxNum_Batches_);
		UB_UVs_.Initialize(device, DB_UVs_.SizeInBytes());
		D3D12::SRV<QuadUVs>::Create(device, Table_UVs_.CPUHandle(0U), DB_UVs_);
		DB_Materials_.Initialize(device, (sizeof(U32) + sizeof(F32) * 4U) * MaxNum_Batches_);
		UB_Materials_.Initialize(device, DB_Materials_.SizeInBytes());
		D3D12::SRV<Material>::Create(device, Table_Materials_.CPUHandle(0U), DB_Materials_);

		auto config{ Utils::LoadFromFile<nlohmann::json>("Engine/Assets/Configs/Sprite.json") };
		auto&& rsSetup{ D3D12::LoadSetup<D3D12::RootSignature>(config.at("RS")) };
		RS_.Initialize(device, rsSetup);

		struct ParticleSpriteVertex {
			F32 Position[4];
		};

		QuadVertexBuffer_.Initialize(device, sizeof(ParticleSpriteVertex) * 4U);
		QuadVBV_ = D3D12::VBV::Create<ParticleSpriteVertex>(QuadVertexBuffer_);
		ParticleSpriteVertex quadVerts[4]{
			{.Position{ 0.0f, 0.0f, 0.0f, 1.0f }, },
			{.Position{ 1.0f, 0.0f, 0.0f, 1.0f }, },
			{.Position{ 0.0f, 1.0f, 0.0f, 1.0f }, },
			{.Position{ 1.0f, 1.0f, 0.0f, 1.0f }, },
		};
		D3D12::UploadBuffer vbTmp{};
		vbTmp.Initialize(device, sizeof(ParticleSpriteVertex) * 4U);
		vbTmp.Store(quadVerts, QuadVertexBuffer_.SizeInBytes(), 0LLU);

		QuadIndexBuffer_.Initialize(device, sizeof(uint32_t) * 6U);
		QuadIBV_ = Lumina::D3D12::IBV::Create(QuadIndexBuffer_);
		uint32_t quadIndices[6]{ 0U, 1U, 2U, 1U, 3U, 2U, };
		D3D12::UploadBuffer ibTmp{};
		ibTmp.Initialize(device, sizeof(uint32_t) * 6U);
		ibTmp.Store(quadIndices, QuadIndexBuffer_.SizeInBytes(), 0LLU);

		D3D12::CommandAllocator cmdAllocator{};
		cmdAllocator.Initialize(device);
		D3D12::CommandList cmdList{};
		cmdList.Initialize(device, cmdAllocator);
		cmdList->CopyResource(QuadVertexBuffer_.Get(), vbTmp.Get());
		cmdList->CopyResource(QuadIndexBuffer_.Get(), ibTmp.Get());

		D3D12Context_->DirectQueue() << cmdList;
		D3D12Context_->DirectQueue().CPUWait(
			D3D12Context_->DirectQueue().ExecuteBatchedCommandLists()
		);
	}

	void SpriteRenderer::Begin(D3D12::CommandList const& cmdList_) {
		CommandList_ = cmdList_.Get();
		Count_UnuploadedBatches_ = 0U;
		Count_UploadedBatches_ = 0U;
	}

	void SpriteRenderer::End() {
		Count_UnuploadedBatches_ = 0U;
		Count_UploadedBatches_ = 0U;
	}

	void SpriteRenderer::BatchBegin() {
		D3D12::Barrier const barriers[]{
			D3D12::Barrier::Transition(
				DB_Worlds_,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_DEST
			),
			D3D12::Barrier::Transition(
				DB_UVs_,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_DEST
			),
			D3D12::Barrier::Transition(
				DB_Materials_,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_DEST
			),
		};
		CommandList_->ResourceBarrier(3U, reinterpret_cast<D3D12_RESOURCE_BARRIER const*>(barriers));
	}

	void SpriteRenderer::Batch(
		Sprite const& sprite_
	) {
		uint32_t const idx{ Count_UnuploadedBatches_ };

		Math::F32x4x4<> world{ Math::F32x4x4<>::Identity };
		world[3].X(-sprite_.AnchorPoint.X);
		world[3].Y(-sprite_.AnchorPoint.Y);
		Math::F32x4x4<>::Multiply(
			world,
			world,
			SRT(
				{ sprite_.Scale },
				{ 0.0f, 0.0f, sprite_.Rotate },
				{ sprite_.Translate }
			)
		);
		UB_Worlds_.Store(
			&world,
			sizeof(Math::F32x4x4<>),
			sizeof(Math::F32x4x4<>) * (idx + Count_UploadedBatches_)
		);

		QuadUVs&& uvs{
			sprite_.UVs[0],
			sprite_.UVs[1],
			sprite_.UVs[2],
			sprite_.UVs[3],
		};
		UB_UVs_.Store(
			&uvs,
			sizeof(QuadUVs),
			sizeof(QuadUVs) * (idx + Count_UploadedBatches_)
		);

		Material material{
			.RGBA{ sprite_.RGBA.X, sprite_.RGBA.Y, sprite_.RGBA.Z, sprite_.RGBA.W, },
			.TextureID{ sprite_.TextureID },
		};
		UB_Materials_.Store(
			&material,
			sizeof(Material),
			sizeof(Material) * (idx + Count_UploadedBatches_)
		);

		++Count_UnuploadedBatches_;
	}

	void SpriteRenderer::BatchEnd() {
		if (Count_UnuploadedBatches_ == 0U) {
			return;
		}

		CommandList_->CopyBufferRegion(
			DB_Worlds_.Get(),
			0LLU,
			UB_Worlds_.Get(),
			sizeof(Math::F32x4x4<>) * Count_UploadedBatches_,
			sizeof(Math::F32x4x4<>) * Count_UnuploadedBatches_
		);
		CommandList_->CopyBufferRegion(
			DB_UVs_.Get(),
			0LLU,
			UB_UVs_.Get(),
			sizeof(QuadUVs) * Count_UploadedBatches_,
			sizeof(QuadUVs) * Count_UnuploadedBatches_
		);
		CommandList_->CopyBufferRegion(
			DB_Materials_.Get(),
			0LLU,
			UB_Materials_.Get(),
			sizeof(Material) * Count_UploadedBatches_,
			sizeof(Material) * Count_UnuploadedBatches_
		);

		D3D12::Barrier const barriers[]{
			D3D12::Barrier::Transition(
				DB_Worlds_,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				DB_UVs_,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				DB_Materials_,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
		};
		CommandList_->ResourceBarrier(3U, reinterpret_cast<D3D12_RESOURCE_BARRIER const*>(barriers));

		Count_UploadedBatches_ += Count_UnuploadedBatches_;
	}

	void SpriteRenderer::Render(
		D3D12::GraphicsPipelineState const& pso_,
		D3D12_GPU_DESCRIPTOR_HANDLE globalSRV_TextureStart_,
		D3D12_CPU_DESCRIPTOR_HANDLE localCBV_VP_
	) {
		D3D12Context_->Device()->CopyDescriptorsSimple(
			1U,
			Table_VP_And_Worlds_.CPUHandle(0U),
			localCBV_VP_,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		if (Count_UnuploadedBatches_ == 0U) {
			return;
		}

		CommandList_->SetGraphicsRootSignature(RS_.Get());
		CommandList_->SetPipelineState(pso_.Get());
		CommandList_->SetGraphicsRootDescriptorTable(
			0U,
			Table_UVs_.GPUHandle(0U)
		);
		CommandList_->SetGraphicsRootDescriptorTable(
			1U,
			Table_VP_And_Worlds_.GPUHandle(0U)
		);
		CommandList_->SetGraphicsRootDescriptorTable(
			2U,
			Table_Materials_.GPUHandle(0U)
		);
		CommandList_->SetGraphicsRootDescriptorTable(
			3U,
			globalSRV_TextureStart_
		);

		CommandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CommandList_->IASetVertexBuffers(0U, 1U, reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW const*>(&QuadVBV_));
		CommandList_->IASetIndexBuffer(reinterpret_cast<D3D12_INDEX_BUFFER_VIEW const*>(&QuadIBV_));
		CommandList_->DrawIndexedInstanced(
			6U,
			Count_UnuploadedBatches_,
			0U,
			0U,
			0U
		);
	}
}