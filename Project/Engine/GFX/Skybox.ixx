export module Lumina.Skybox;

import <string>;
import <array>;
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

namespace Lumina {
	export class Skybox {
	public:
		auto GlobalTable() const noexcept -> D3D12::DescriptorTable const& { return SRV_Textures_; }

	public:
		void Render(
			D3D12::CommandList const& cmdList_,
			D3D12_CPU_DESCRIPTOR_HANDLE cbv_WorldToProjective_
		);

		void Initialize(
			D3D12::Context const& d3d12Context_,
			D3D12::GraphicsDevice const& d3d12Device_,
			std::string_view texFilePath_
		);

	private:
		struct Vertex {
			F32x4 Position;
		};

		std::array<Vertex, 24U> Vertices_;
		std::array<uint32_t, 36U> Indices_;

		D3D12::RootSignature RS_;
		D3D12::Shader VS_;
		D3D12::Shader PS_;
		D3D12::GraphicsPSO PSO_;

		D3D12::DescriptorTable CBV_;
		D3D12::DescriptorTable SRV_Textures_;

		D3D12::UploadBuffer VertexBuffer_;
		D3D12::UploadBuffer IndexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW VBV_;
		D3D12_INDEX_BUFFER_VIEW IBV_;

		D3D12::ImageTexture const* Texture_{ nullptr };
	};
}