module;

#include<d3d12.h>

module Lumina.ProceduralMap;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import Lumina.Utils.Data;

import Lumina.Core.Common;
import Lumina.Core.Math;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace Lumina {
	template<>
	auto ProceduralMap::Render(
		D3D12::RootSignature const& rs_,
		D3D12::ComputePSO const& pso_
	) -> void {
		ComputeList_->SetComputeRootSignature(rs_.Get());
		ComputeList_->SetPipelineState(pso_.Get());

		ComputeList_->Dispatch(Width_, Height_, 1U);
	}

	template<>
	auto ProceduralMap::Initialize<"Resource">(
		D3D12::GraphicsDevice const& d3d12Device_,
		U32 const& width_,
		U32 const& height_,
		DXGI_FORMAT const& format_
	) -> void {
		Texture_.Initialize(d3d12Device_, width_, height_, format_);
	}

	template<>
	auto ProceduralMap::Initialize<"Descriptors">(
		D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		LocalHeap_.Initialize(
			d3d12Device_,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			2U,
			false
		);

		D3D12::UAV<void>::Create(d3d12Device_, LocalHeap_.CPUHandle(0U), Texture_);
		D3D12::SRV<void>::Create(d3d12Device_, LocalHeap_.CPUHandle(1U), Texture_);
	}

	template<>
	auto ProceduralMap::Initialize<"CommandLists">(
		D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		ComputeAllocator_.Initialize(
			d3d12Device_,
			D3D12_COMMAND_LIST_TYPE_COMPUTE
		);
		ComputeList_.Initialize(
			d3d12Device_,
			ComputeAllocator_
		);

		DirectAllocator_.Initialize(
			d3d12Device_,
			D3D12_COMMAND_LIST_TYPE_DIRECT
		);
		DirectList_.Initialize(
			d3d12Device_,
			DirectAllocator_
		);
	}

	auto ProceduralMap::Initialize(
		D3D12::Context const& d3d12Context_,
		U32 const width_,
		U32 const height_,
		DXGI_FORMAT const format_
	) -> void {
		auto const& d3d12Device{ d3d12Context_.Device() };

		Initialize<"Resource">(d3d12Device, width_, height_, format_);
		Initialize<"Descriptors">(d3d12Device);
		Initialize<"CommandLists">(d3d12Device);
	}

	//void ProceduralMap::UpdateElevation(D3D12::CommandQueue& directQueue_) {
	//	const Math::PerlinNoise noiseGen{
	//		ElevationNoiseParam_.Frequency,
	//		ElevationNoiseParam_.Num_Octaves,
	//		ElevationNoiseParam_.Persistance,
	//		ElevationNoiseParam_.Offset
	//	};

	//	constexpr float div{ 1.0f / 32.0f };
	//	const float inv_Width = 1.0f / static_cast<float>(Width_);
	//	const float inv_Height = 1.0f / static_cast<float>(Height_);
	//	float inv_TerraceFactor_{ 1.0f / TerraceFactor_ };

	//	for (uint32_t v = 0; v < Height_; ++v) {
	//		for (uint32_t u = 0; u < Width_; ++u) {
	//			const float noise{ noiseGen(u * div, v * div, 0.0f) };

	//			const float nu = 2.0f * u * inv_Width - 1.0f;
	//			const float nv = 2.0f * v * inv_Height - 1.0f;
	//			const float island = (1.0f - nu * nu) * (1.0f - nv * nv);
	//			const float val = noise * (1.0f - Insulation_) + island * Insulation_;

	//			auto& surflet{ MapSurflets_[v * Width_ + u] };

	//			surflet.Elevation = std::pow(val, ElevationNoiseParam_.Redist);
	//			surflet.Elevation *= 2.0f;
	//			if (IsFormingTerraces_) {
	//				surflet.Elevation *= TerraceFactor_;
	//				surflet.Elevation = std::round(surflet.Elevation);
	//				surflet.Elevation *= inv_TerraceFactor_;
	//			}
	//		}
	//	}

	//	for (uint32_t v = 0; v < Height_; ++v) {
	//		uint32_t v0 = (v == 0) ? (v) : (v - 1);
	//		uint32_t v1 = (v == Height_ - 1) ? (v) : (v + 1);

	//		for (uint32_t u = 0; u < Width_; ++u) {
	//			uint32_t u0 = (u == 0) ? (u) : (u - 1);
	//			uint32_t u1 = (u == Width_ - 1) ? (u) : (u + 1);

	//			Vec3 df_du{ (u1 - u0) * div, 0.0f, MapSurflets_[v * Width_ + u1].Elevation - MapSurflets_[v * Width_ + u0].Elevation };
	//			Vec3 df_dv{ 0.0f, (v1 - v0) * div, MapSurflets_[v1 * Width_ + u].Elevation - MapSurflets_[v0 * Width_ + u].Elevation };
	//			Vec3 unit{ Vec3::Cross(df_du, df_dv) };
	//			unit = unit.Unit();
	//			MapSurflets_[v * Width_ + u].Normal = { unit.x, unit.y, unit.z };
	//		}
	//	}

	//	MapSurfletUpload_.Store(MapSurflets_, MapSurfletUpload_.SizeInBytes(), 0U);
	//	DirectList_->CopyResource(MapSurfletBuffer_.Get(), MapSurfletUpload_.Get());
	//	directQueue_ << DirectList_;
	//	directQueue_.CPUWait(directQueue_.ExecuteBatchedCommandLists());
	//	DirectList_.Reset(DirectAllocator_);
	//}

	//void ProceduralMap::UpdateTemperature(D3D12::CommandQueue& directQueue_) {
	//	Math::PerlinNoise noiseGen{
	//		TemperatureNoiseParam_.Frequency,
	//		TemperatureNoiseParam_.Num_Octaves,
	//		TemperatureNoiseParam_.Persistance,
	//		TemperatureNoiseParam_.Offset
	//	};

	//	constexpr float div{ 1.0f / 32.0f };
	//	const float inv_Height = 1.0f / static_cast<float>(Height_);

	//	for (uint32_t v = 0; v < Height_; ++v) {
	//		float latFactor{
	//			(1.0f - std::abs(static_cast<int32_t>(v << 1U) - static_cast<int32_t>(Height_)) * inv_Height)
	//		};
	//		for (uint32_t u = 0; u < Width_; ++u) {
	//			const float noise{ noiseGen(u * div, v * div, 0.0f) };

	//			float elvFactor{
	//				1.0f - MapSurflets_[v * Width_ + u].Elevation * 0.5f
	//			};

	//			auto& climate{ ClimateData_[v * Width_ + u] };
	//			climate.Temperature = std::pow(noise, TemperatureNoiseParam_.Redist);
	//			climate.Temperature =
	//				climate.Temperature * 0.65f +
	//				std::pow(latFactor, 1.5f) * 0.35f;
	//			climate.Temperature *= elvFactor;
	//		}
	//	}

	//	ClimateUpload_.Store(ClimateData_, ClimateUpload_.SizeInBytes(), 0U);
	//	DirectList_->CopyResource(ClimateBuffer_.Get(), ClimateUpload_.Get());
	//	directQueue_ << DirectList_;
	//	directQueue_.CPUWait(directQueue_.ExecuteBatchedCommandLists());
	//	DirectList_.Reset(DirectAllocator_);
	//}

	//void ProceduralMap::UpdatePrecipitation(D3D12::CommandQueue& directQueue_) {
	//	Math::PerlinNoise noiseGen{
	//		PrecipitationNoiseParam_.Frequency,
	//		PrecipitationNoiseParam_.Num_Octaves,
	//		PrecipitationNoiseParam_.Persistance,
	//		PrecipitationNoiseParam_.Offset
	//	};

	//	constexpr float div{ 1.0f / 32.0f };

	//	for (uint32_t v = 0; v < Height_; ++v) {
	//		for (uint32_t u = 0; u < Width_; ++u) {
	//			const float noise{ noiseGen(u * div, v * div, 0.0f) };

	//			auto& climate{ ClimateData_[v * Width_ + u] };
	//			climate.Precipitation = std::pow(noise, PrecipitationNoiseParam_.Redist);
	//		}
	//	}

	//	ClimateUpload_.Store(ClimateData_, ClimateUpload_.SizeInBytes(), 0U);
	//	DirectList_->CopyResource(ClimateBuffer_.Get(), ClimateUpload_.Get());
	//	directQueue_ << DirectList_;
	//	directQueue_.CPUWait(directQueue_.ExecuteBatchedCommandLists());
	//	DirectList_.Reset(DirectAllocator_);
	//}

	//void ProceduralMap::Update(D3D12::CommandQueue& directQueue_) {
	//	#if defined(_DEBUG)
	//	ImGui::Begin("Procedural ProceduralMap");

	//	ImGui::SeparatorText("Elevation");
	//	ImGui::DragFloat("Frequency##TerElv", &ElevationNoiseParam_.Frequency, 0.01f);
	//	ImGui::DragFloat("Redistribution##TerElv", &ElevationNoiseParam_.Redist, 0.01f);
	//	ImGui::DragFloat3("Offset##TerElv", reinterpret_cast<float*>(&ElevationNoiseParam_.Offset), 0.01f);
	//	ImGui::DragInt("Octaves##TerElv", reinterpret_cast<int*>(&ElevationNoiseParam_.Num_Octaves), 0.1f, 1, 32);
	//	ImGui::DragFloat("Persistance##TerElv", &ElevationNoiseParam_.Persistance, 0.01f, 0.0f, 0.875f);
	//	ImGui::DragFloat("Insulation##TerElv", &Insulation_, 0.01f, 0.0f, 1.0f);
	//	ImGui::Checkbox("Is Forming Terraces##TerElv", reinterpret_cast<bool*>(&IsFormingTerraces_));
	//	ImGui::DragFloat("Terrace Factor##TerElv", &TerraceFactor_, 0.01f, 1.0f, 32.0f);
	//	if (ImGui::Button("Update##TerElv")) {
	//		UpdateElevation(directQueue_);
	//	}

	//	ImGui::SeparatorText("Temperature");
	//	ImGui::DragFloat("Frequency##TerTmp", &TemperatureNoiseParam_.Frequency, 0.01f);
	//	ImGui::DragFloat("Redistribution##TerTmp", &TemperatureNoiseParam_.Redist, 0.01f);
	//	ImGui::DragF32x3("Offset##TerTmp", reinterpret_cast<float*>(&TemperatureNoiseParam_.Offset), 0.01f);
	//	ImGui::DragInt("Octaves##TerTmp", reinterpret_cast<int*>(&TemperatureNoiseParam_.Num_Octaves), 0.1f, 1, 32);
	//	ImGui::DragFloat("Persistance##TerTmp", &TemperatureNoiseParam_.Persistance, 0.01f, 0.0f, 0.875f);
	//	if (ImGui::Button("Update##TerTmp")) {
	//		UpdateTemperature(directQueue_);
	//	}

	//	ImGui::SeparatorText("Precipitation");
	//	ImGui::DragFloat("Frequency##TerPcp", &PrecipitationNoiseParam_.Frequency, 0.01f);
	//	ImGui::DragFloat("Redistribution##TerPcp", &PrecipitationNoiseParam_.Redist, 0.01f);
	//	ImGui::DragF32x3("Offset##TerPcp", reinterpret_cast<float*>(&PrecipitationNoiseParam_.Offset), 0.01f);
	//	ImGui::DragInt("Octaves##TerPcp", reinterpret_cast<int*>(&PrecipitationNoiseParam_.Num_Octaves), 0.1f, 1, 32);
	//	ImGui::DragFloat("Persistance##TerPcp", &PrecipitationNoiseParam_.Persistance, 0.01f, 0.0f, 0.875f);
	//	if (ImGui::Button("Update##TerPcp")) {
	//		UpdatePrecipitation(directQueue_);
	//	}

	//	ImGui::End();
	//	#endif
	//}

	//void ProceduralMap::Render(
	//	D3D12::CommandList& directList_,
	//	const D3D12::DescriptorHeap& cbvsrvuavHeap_,
	//	const D3D12::DescriptorTable& vpCBVTable_,
	//	const D3D12::DescriptorTable& texSRVTable_
	//) {
	//	ID3D12DescriptorHeap* descriptorHeaps[]{ cbvsrvuavHeap_.Get() };
	//	directList_->SetDescriptorHeaps(1U, descriptorHeaps);
	//	directList_->SetPipelineState(RenderPSO_.Get());
	//	directList_->SetGraphicsRootSignature(RenderRS_.Get());
	//	directList_->SetGraphicsRootDescriptorTable(0U, CSUTable_.GPUHandle(0U));
	//	directList_->SetGraphicsRootDescriptorTable(1U, vpCBVTable_.GPUHandle(0U));
	//	directList_->SetGraphicsRootDescriptorTable(2U, texSRVTable_.GPUHandle(0U));
	//	directList_->IASetVertexBuffers(0U, 1U, &QuadVBV_);
	//	directList_->IASetIndexBuffer(&QuadIBV_);
	//	directList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	directList_->DrawIndexedInstanced(6U, Width_ * Height_, 0U, 0U, 0U);
	//}

	ProceduralMap::ProceduralMap() {}
	ProceduralMap::~ProceduralMap() {}
}