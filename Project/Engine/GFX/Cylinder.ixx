export module Lumina.Cylinder;

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
	export class Cylinder {
	public:
		struct Properties {
			U32 NUM_Division;
			F32 Height;
			F32 Radius_Top;
			F32 Radius_Bottom;
		};

	public:
		Properties Reset(Properties const& props_);

	public:
		void Render(
			D3D12::CommandList const& cmdList_,
			D3D12::RootSignature const& rs_,
			D3D12::GraphicsPSO const& graphicsPSO_,
			Math::F32x4x4<> const& localToWorld_,
			Math::F32x4x4<> const& worldToProjective_
		);

		void Initialize(
			D3D12::Context const& dxContext_,
			D3D12::GraphicsDevice const& device_,
			std::string_view filePath_ = "Assets/Img/gradationLine.png"
		);

	private:
		struct Vertex {
			F32x4 Position;
			F32x2 TexCoord;
			F32x3 Normal;
		};

		std::vector<Vertex> Vertices_;
		std::vector<uint32_t> Indices_;

		D3D12::UploadBuffer VertexBuffer_;
		D3D12::UploadBuffer IndexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW VBV_;
		D3D12_INDEX_BUFFER_VIEW IBV_;

		D3D12::UploadBuffer UB_Constants_;
		D3D12::DescriptorTable CBV_Constants_;
		D3D12::DescriptorTable SRV_Textures_;

	public:
		constexpr static uint32_t NUM_Division_MIN{ 3U };
		constexpr static uint32_t NUM_Division_MAX{ 128U };

	private:
		Properties CylinderProperties_;
		F32 Time_;
	};
}