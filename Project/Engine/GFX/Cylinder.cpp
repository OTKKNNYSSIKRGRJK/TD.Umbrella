module Lumina.Cylinder;

import <random>;
import <vector>;
import <cstdint>;
import <d3d12.h>;

import Lumina.Core.Common;
import Lumina.Core.Math;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;
import Lumina.Utils.Data;

import Lumina.Main;

namespace Lumina {
	Cylinder::Properties Cylinder::Reset(Properties const& props_) {
		Properties props{};
		{
			props.NUM_Division = std::clamp<uint32_t>(
				props_.NUM_Division,
				NUM_Division_MIN,
				NUM_Division_MAX
			);
			props.Height = std::clamp<float>(
				props_.Height,
				0.001f,
				10.0f
			);
			props.Radius_Top = std::clamp<float>(
				props_.Radius_Top,
				0.001f,
				10.0f
			);
			props.Radius_Bottom = std::clamp<float>(
				props_.Radius_Bottom,
				0.001f,
				10.0f
			);
		}

		float const inv_NUM_DIV{ 1.0f / static_cast<float>(props.NUM_Division) };
		float const radianPerDivision{ Math::Constant::Pi * 2.0f * inv_NUM_DIV };

		std::vector<std::pair<float, float>> lookup_SINCOSs;
		{
			lookup_SINCOSs.resize(props.NUM_Division + 1U);

			lookup_SINCOSs[0] = { 0.0f, 1.0f };
			lookup_SINCOSs[props.NUM_Division] = { 0.0f, 1.0f };
			if ((props.NUM_Division & 1U) == 0U) {
				lookup_SINCOSs[props.NUM_Division >> 1U] = { 0.0f, -1.0f };
			}

			for (uint32_t idx{ 1U }; idx < ((props.NUM_Division + 1U) >> 1U); ++idx) {
				lookup_SINCOSs[idx] = {
					std::sin(idx * radianPerDivision),
					std::cos(idx * radianPerDivision)
				};
				lookup_SINCOSs[props.NUM_Division - idx] = {
					-lookup_SINCOSs[idx].first,
					lookup_SINCOSs[idx].second,
				};
			}
		}

		Vertices_.clear();
		Vertices_.resize(props.NUM_Division * 4U);
		Indices_.clear();
		Indices_.resize(props.NUM_Division * 6U);
		for (uint32_t idx_DIV{ 0U }; idx_DIV < props.NUM_Division; ++idx_DIV) {
			auto const& sinCos_CUR{ lookup_SINCOSs[idx_DIV] };
			auto const& sinCos_NEXT{ lookup_SINCOSs[idx_DIV + 1U] };

			float const texCoordU_CUR{ static_cast<float>(idx_DIV) * inv_NUM_DIV };
			float const texCoordU_NEXT{ static_cast<float>(idx_DIV + 1U) * inv_NUM_DIV };

			Vertices_[idx_DIV * 4U + 0U] = {
				F32x4{
					-sinCos_CUR.first * props.Radius_Top,
					props.Height,
					sinCos_CUR.second * props.Radius_Top,
					1.0f
				},
				F32x2{ texCoordU_CUR, 1.0f },
				F32x3{ -sinCos_CUR.first, 0.0f, sinCos_CUR.second }
			};
			Vertices_[idx_DIV * 4U + 1U] = {
				F32x4{
					-sinCos_NEXT.first * props.Radius_Top,
					props.Height,
					sinCos_NEXT.second * props.Radius_Top,
					1.0f
				},
				F32x2{ texCoordU_NEXT, 1.0f },
				F32x3{ -sinCos_NEXT.first, 0.0f, sinCos_NEXT.second }
			};
			Vertices_[idx_DIV * 4U + 2U] = {
				F32x4{
					-sinCos_CUR.first * props.Radius_Bottom,
					0.0f,
					sinCos_CUR.second * props.Radius_Bottom,
					1.0f
				},
				F32x2{ texCoordU_CUR, 0.0f },
				F32x3{ -sinCos_CUR.first, 0.0f, sinCos_CUR.second }
			};
			Vertices_[idx_DIV * 4U + 3U] = {
				F32x4{
					-sinCos_NEXT.first * props.Radius_Bottom,
					0.0f,
					sinCos_NEXT.second * props.Radius_Bottom,
					1.0f
				},
				F32x2{ texCoordU_NEXT, 0.0f },
				F32x3{ -sinCos_NEXT.first, 0.0f, sinCos_NEXT.second }
			};

			Indices_[idx_DIV * 6U + 0U] = idx_DIV * 4U + 0U;
			Indices_[idx_DIV * 6U + 1U] = idx_DIV * 4U + 1U;
			Indices_[idx_DIV * 6U + 2U] = idx_DIV * 4U + 2U;
			Indices_[idx_DIV * 6U + 3U] = idx_DIV * 4U + 1U;
			Indices_[idx_DIV * 6U + 4U] = idx_DIV * 4U + 3U;
			Indices_[idx_DIV * 6U + 5U] = idx_DIV * 4U + 2U;
		}

		VertexBuffer_.Store(Vertices_.data(), sizeof(Vertex) * Vertices_.size(), 0LLU);
		IndexBuffer_.Store(Indices_.data(), sizeof(uint32_t) * Indices_.size(), 0LLU);

		CylinderProperties_ = props;
		return props;
	}
	
	void Cylinder::Render(
		D3D12::CommandList const& cmdList_,
		D3D12::RootSignature const& rs_,
		D3D12::GraphicsPSO const& graphicsPSO_,
		Math::F32x4x4<> const& localToWorld_,
		Math::F32x4x4<> const& worldToProjective_
	) {
		Time_ += 0.0166667f;

		UB_Constants_.Store(
			&localToWorld_,
			sizeof(Math::F32x4x4<>),
			0LLU
		);
		UB_Constants_.Store(
			&worldToProjective_,
			sizeof(Math::F32x4x4<>),
			sizeof(Math::F32x4x4<>)
		);
		UB_Constants_.Store(
			&Time_,
			sizeof(F32),
			sizeof(Math::F32x4x4<>) * 2LLU
		);

		cmdList_->SetGraphicsRootSignature(rs_.Get());
		cmdList_->SetGraphicsRootDescriptorTable(0U, CBV_Constants_.GPUHandle(0U));
		cmdList_->SetGraphicsRootDescriptorTable(1U, SRV_Textures_.GPUHandle(0U));
		cmdList_->SetPipelineState(graphicsPSO_.Get());
		cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList_->IASetVertexBuffers(0U, 1U, &VBV_);
		cmdList_->IASetIndexBuffer(&IBV_);
		cmdList_->DrawIndexedInstanced(CylinderProperties_.NUM_Division * 6U, 1U, 0U, 0U, 0U);
	}

	void Cylinder::Initialize(
		D3D12::Context const& dxContext_,
		D3D12::GraphicsDevice const& device_,
		std::string_view filePath_
	) {
		auto& context{ Lumina::Context::Instance() };
		auto& resMngr{ context.ResourceContext() };

		SRV_Textures_ = dxContext_.GlobalDescriptorHeap().Allocate(1U);
		std::vector<uint32_t> texIDs{};
		resMngr.Graphics().LoadImageTextures(
			texIDs,
			{
				{ filePath_.data(), filePath_.data() },
			}
		);
		for (uint32_t idx{ 0U }; idx < static_cast<uint32_t>(texIDs.size()); ++idx) {
			device_->CopyDescriptorsSimple(
				1U,
				SRV_Textures_.CPUHandle(idx),
				resMngr.Graphics().CPUHandle(texIDs.at(idx)),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		VertexBuffer_.Initialize(device_, sizeof(Vertex) * NUM_Division_MAX * 4U);
		IndexBuffer_.Initialize(device_, sizeof(uint32_t) * NUM_Division_MAX * 6U);

		auto vbv{ D3D12::VBV::Create<Vertex>(VertexBuffer_) };
		VBV_ = *static_cast<D3D12_VERTEX_BUFFER_VIEW*>(reinterpret_cast<void*>(&vbv));
		auto ibv{ D3D12::IBV::Create(IndexBuffer_) };
		IBV_ = *static_cast<D3D12_INDEX_BUFFER_VIEW*>(reinterpret_cast<void*>(&ibv));

		UB_Constants_.Initialize(device_, 256LLU);
		CBV_Constants_ = dxContext_.GlobalDescriptorHeap().Allocate(1U);
		D3D12::CBV::Create(device_, CBV_Constants_.CPUHandle(0U), UB_Constants_);

		Time_ = 0.0f;
	}
}