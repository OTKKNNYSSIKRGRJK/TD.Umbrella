export module ParticleSystem;

import <cstdint>;
import <type_traits>;

import Lumina.Core.Common;
import Lumina.Core.Math;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import Lumina.MeshManager;

namespace Lumina {
	namespace {
		inline void StageRenderCommands(
			Lumina::D3D12::CommandList const& cmdList_,
			Lumina::D3D12::RootSignature const& rs_,
			Lumina::D3D12::GraphicsPSO const& graphicsPSO_,
			D3D12_GPU_DESCRIPTOR_HANDLE srv_ParticleRenderData_,
			D3D12_GPU_DESCRIPTOR_HANDLE cbv_SceneVars_,
			D3D12_GPU_DESCRIPTOR_HANDLE cbv_VP_,
			Lumina::D3D12::DescriptorTable const& table_Textures_,
			Lumina::D3D12::DescriptorTable const& table_Textures2_,
			D3D12_VERTEX_BUFFER_VIEW const& particleMeshVBV_,
			D3D12_INDEX_BUFFER_VIEW const& particleMeshIBV_,
			uint32_t num_Inst_
		) {
			cmdList_->SetGraphicsRootSignature(rs_.Get());
			cmdList_->SetPipelineState(graphicsPSO_.Get());
			// Array of render data
			cmdList_->SetGraphicsRootDescriptorTable(0U, srv_ParticleRenderData_);
			cmdList_->SetGraphicsRootDescriptorTable(1U, cbv_SceneVars_);
			cmdList_->SetGraphicsRootDescriptorTable(2U, cbv_VP_);
			cmdList_->SetGraphicsRootDescriptorTable(3U, table_Textures_.GPUHandle(0U));
			cmdList_->SetGraphicsRootDescriptorTable(4U, table_Textures2_.GPUHandle(0U));
			cmdList_->IASetVertexBuffers(0U, 1U, &particleMeshVBV_);
			cmdList_->IASetIndexBuffer(&particleMeshIBV_);
			cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			if (num_Inst_ > 0U) {
				cmdList_->DrawIndexedInstanced(6U, num_Inst_, 0U, 0U, 0U);
			}
		}

		inline auto SRT(
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

			srt[0] = srt[0] * scale_.X;
			srt[1] = srt[1] * scale_.Y;
			srt[2] = srt[2] * scale_.Z;

			return srt;
		}

		struct ParticleSpriteVertex {
			Lumina::F32x4 Position;
			Lumina::F32x2 TexCoord;
		};
	}

	export struct Particle {
		Lumina::F32x3 Scale{ 1.0f, 1.0f, 1.0f };
		Lumina::F32x3 Rotate{ 0.0f, 0.0f, 0.0f };
		Lumina::F32x3 Translate{ 0.0f, 0.0f, 0.0f };

		Lumina::F32x3 Velocity;

		float Life{ 10.0f };

		struct RenderDataCollection {
			Lumina::F32x4 RGBA{ 1.0f, 1.0f, 1.0f, 1.0f };
			uint32_t DiffuseID;
			uint32_t DiffuseAtlasID;
		};

		RenderDataCollection RenderData;
	};

	template<typename T>
	concept Concept_Particle = std::is_base_of_v<Particle, T>;

	export template<Concept_Particle T>
	class ParticleSystem {
	public:
		constexpr static auto DefaultUpdateCallback{
			[] (T& p_, void const*) {
				p_.Translate.X += p_.Velocity.X;
				p_.Translate.Y += p_.Velocity.Y;
				p_.Translate.Z += p_.Velocity.Z;
				p_.Life -= 1.0f;
				return (p_.Life > 0.0f);
			}
		};

	public:
		typename Lumina::List<T> const& InstanceList() const noexcept;

	public:
		void Emit(T&& p_) {
			if (!Instances_.IsFull()) {
				auto& p{ Instances_.New() };
				p = std::move(p_);
			}
		}
		void Clear() {
			Instances_.Clear();
			Count_Alive_ = 0U;
		}

	public:
		template<typename UpdateCallback>
		void Update(
			Lumina::D3D12::CommandList const& cmdList_,
			Lumina::Math::F32x4x4<> const& viewToWorld_,
			UpdateCallback updateFunc_,
			void const* updateFuncParam_ = nullptr
		);

		void Render(
			Lumina::D3D12::CommandList const& cmdList_,
			Lumina::D3D12::RootSignature const& rs_,
			Lumina::D3D12::GraphicsPSO const& graphicsPSO_,
			D3D12_CPU_DESCRIPTOR_HANDLE localCBV_SceneVars_,
			D3D12_CPU_DESCRIPTOR_HANDLE localCBV_VP_,
			Lumina::D3D12::DescriptorTable const& globalTable_Textures_,
			Lumina::D3D12::DescriptorTable const& globalTable_Textures2_
		);

	public:
		void Initialize(
			Lumina::D3D12::Context const& d3d12Context_,
			uint32_t num_
		);

	private:
		Lumina::List<T> Instances_{};
		uint32_t Count_Alive_{ 0U };

	private:
		Lumina::D3D12::Context const* D3D12Context_{ nullptr };

		Lumina::D3D12::DefaultBuffer DB_Array_RenderData_{};
		Lumina::D3D12::UploadBuffer UB_Array_RenderData_{};

		Lumina::D3D12::DescriptorTable GlobalTable_{};

		Lumina::D3D12::DefaultBuffer QuadVertexBuffer_{};
		Lumina::D3D12::DefaultBuffer QuadIndexBuffer_{};
		D3D12_VERTEX_BUFFER_VIEW QuadVBV_{};
		D3D12_INDEX_BUFFER_VIEW QuadIBV_{};
	};

	template<Concept_Particle T>
	typename Lumina::List<T> const& ParticleSystem<T>::InstanceList() const noexcept {
		return Instances_;
	}

	template<Concept_Particle T>
	template<typename UpdateCallback>
	void ParticleSystem<T>::Update(
		Lumina::D3D12::CommandList const& cmdList_,
		Lumina::Math::F32x4x4<> const& viewToWorld_,
		UpdateCallback updateFunc_,
		void const* updateFuncParam_
	) {
		Count_Alive_ = 0U;

		typename decltype(Instances_)::Iterator it{ Instances_ };
		for (it.Begin(); !it.End(); it.Next()) {
			auto& particle{ (*it) };
			int32_t isAlive{ 1 };

			if (!updateFunc_(particle, updateFuncParam_)) {
				Instances_.Delete(it);
				isAlive = 0;
			}

			if (isAlive) {
				auto&& transform{
					SRT(
						Lumina::Math::F32x3{ &particle.Scale.X },
						Lumina::Math::F32x3{ &particle.Rotate.X },
						Lumina::Math::F32x3{ &particle.Translate.X }
					)
				};

				Lumina::Math::F32x4x4<>::Multiply(transform, transform, viewToWorld_);
				transform[3] = {
					particle.Translate.X,
					particle.Translate.Y,
					particle.Translate.Z,
					1.0f
				};

				// Transform
				UB_Array_RenderData_.Store(
					&transform,
					sizeof(Lumina::Math::F32x4x4<>),
					(sizeof(Lumina::Math::F32x4x4<>) + sizeof(typename T::RenderDataCollection)) *
					Count_Alive_ + 0LLU
				);
				// RGBA, TextureID, AtlasID
				UB_Array_RenderData_.Store(
					&particle.RenderData,
					sizeof(typename T::RenderDataCollection),
					(sizeof(Lumina::Math::F32x4x4<>) + sizeof(typename T::RenderDataCollection)) *
					Count_Alive_ + sizeof(Lumina::Math::F32x4x4<>)
				);

				++Count_Alive_;
			}
		}

		D3D12_RESOURCE_BARRIER const barriers_BeforeCopy[]{
			Lumina::D3D12::Barrier::Transition(
				DB_Array_RenderData_,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_DEST
			),
		};
		D3D12_RESOURCE_BARRIER const barriers_AfterCopy[]{
			Lumina::D3D12::Barrier::Transition(
				DB_Array_RenderData_,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
		};
		cmdList_->ResourceBarrier(1U, barriers_BeforeCopy);
		cmdList_->CopyBufferRegion(
			DB_Array_RenderData_.Get(),
			0LLU,
			UB_Array_RenderData_.Get(),
			0LLU,
			(sizeof(Lumina::Math::F32x4x4<>) + sizeof(T::RenderDataCollection)) * Count_Alive_
		);
		cmdList_->ResourceBarrier(1U, barriers_AfterCopy);
	}

	template<Concept_Particle T>
	void ParticleSystem<T>::Render(
		Lumina::D3D12::CommandList const& cmdList_,
		Lumina::D3D12::RootSignature const& rs_,
		Lumina::D3D12::GraphicsPSO const& graphicsPSO_,
		D3D12_CPU_DESCRIPTOR_HANDLE localCBV_SceneVars_,
		D3D12_CPU_DESCRIPTOR_HANDLE localCBV_VP_,
		Lumina::D3D12::DescriptorTable const& globalTable_Textures_,
		Lumina::D3D12::DescriptorTable const& globalTable_Textures2_
	) {
		D3D12Context_->Device()->CopyDescriptorsSimple(
			1U,
			GlobalTable_.CPUHandle(1U),
			localCBV_SceneVars_,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
		D3D12Context_->Device()->CopyDescriptorsSimple(
			1U,
			GlobalTable_.CPUHandle(2U),
			localCBV_VP_,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		StageRenderCommands(
			cmdList_,
			rs_,
			graphicsPSO_,
			GlobalTable_.GPUHandle(0U),
			GlobalTable_.GPUHandle(1U),
			GlobalTable_.GPUHandle(2U),
			globalTable_Textures_,
			globalTable_Textures2_,
			QuadVBV_,
			QuadIBV_,
			Count_Alive_
		);
	}

	template<Concept_Particle T>
	void ParticleSystem<T>::Initialize(
		Lumina::D3D12::Context const& d3d12Context_,
		uint32_t num_
	) {
		Instances_.Initialize(num_);

		D3D12Context_ = &d3d12Context_;
		auto const& device{ d3d12Context_.Device() };

		struct RenderDataType {
			F32 Transform[4][4];
			typename T::RenderDataCollection RenderData;
		};
		DB_Array_RenderData_.Initialize(
			device,
			sizeof(RenderDataType) * num_
		);
		UB_Array_RenderData_.Initialize(
			device,
			DB_Array_RenderData_.SizeInBytes()
		);

		d3d12Context_.GlobalDescriptorHeap().Allocate(GlobalTable_, 3U);
		Lumina::D3D12::SRV<RenderDataType>::Create(
			device,
			GlobalTable_.CPUHandle(0U),
			DB_Array_RenderData_
		);

		QuadVertexBuffer_.Initialize(device, sizeof(ParticleSpriteVertex) * 4U);
		QuadVBV_ = Lumina::D3D12::VBV::Create<ParticleSpriteVertex>(QuadVertexBuffer_);
		float halfSide{ 1.0f / 2.0f };
		ParticleSpriteVertex quadVerts[4]{
			{.Position{ -halfSide, halfSide, 0.0f, 1.0f }, .TexCoord{ 0.0f, 0.0f }, },
			{.Position{ halfSide, halfSide, 0.0f, 1.0f }, .TexCoord{ 1.0f, 0.0f }, },
			{.Position{ -halfSide, -halfSide, 0.0f, 1.0f }, .TexCoord{ 0.0f, 1.0f }, },
			{.Position{ halfSide, -halfSide, 0.0f, 1.0f }, .TexCoord{ 1.0f, 1.0f }, },
		};
		Lumina::D3D12::UploadBuffer vbTmp{};
		vbTmp.Initialize(device, sizeof(ParticleSpriteVertex) * 4U);
		vbTmp.Store(quadVerts, QuadVertexBuffer_.SizeInBytes(), 0LLU);

		QuadIndexBuffer_.Initialize(device, sizeof(uint32_t) * 6U);
		QuadIBV_ = Lumina::D3D12::IBV::Create(QuadIndexBuffer_);
		uint32_t quadIndices[6]{ 0U, 1U, 2U, 1U, 3U, 2U, };
		Lumina::D3D12::UploadBuffer ibTmp{};
		ibTmp.Initialize(device, sizeof(uint32_t) * 6U);
		ibTmp.Store(quadIndices, QuadIndexBuffer_.SizeInBytes(), 0LLU);

		Lumina::D3D12::CommandAllocator cmdAllocator{};
		cmdAllocator.Initialize(device);
		Lumina::D3D12::CommandList cmdList{};
		cmdList.Initialize(device, cmdAllocator);
		cmdList->CopyResource(QuadVertexBuffer_.Get(), vbTmp.Get());
		cmdList->CopyResource(QuadIndexBuffer_.Get(), ibTmp.Get());
		d3d12Context_.DirectQueue() << cmdList;
		d3d12Context_.DirectQueue().CPUWait(d3d12Context_.DirectQueue().ExecuteBatchedCommandLists());
	}
}