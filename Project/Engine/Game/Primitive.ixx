export module Lumina.Primitive;

import <memory>;

import <d3d12.h>;

import nlohmann.json;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Core.String;

import Lumina.Utils.Data;

namespace Lumina {
	export struct PrimitiveVertex {
		F32x4 Position;
		F32x4 Color;
		F32x2 TexCoord;
		U32 TexID;
	};

	export class PrimitiveManager {
	public:
		void Initialize(
			D3D12::Context const& D3D12Context_,
			WStringView filePath_VS_ = L"Assets/Shaders/Primitive.VS.hlsl",
			WStringView filePath_PS_ = L"Assets/Shaders/Primitive.PS.hlsl",
			bool isAdditive_ = false,
			bool depthEnabled_ = true,
			int capacity_ = 0
		);

		void Begin(
			D3D12::CommandList const& cmdList_
		);

		void End(
			D3D12::CommandList const& cmdList_,
			D3D12::DescriptorTable const& texTable_,
			Math::F32x4x4<> const& vp_,
			I32 flag_SRGB_ = 1
		);

		void End(
			D3D12::CommandList const& cmdList_
		);

		void Render(
			D3D12::CommandList const& cmdList_,
			D3D12::DescriptorTable const& texTable_,
			Math::F32x4x4<> const& vp_,
			I32 flag_SRGB_ = 1,
			D3D12_GPU_DESCRIPTOR_HANDLE cbv_ = { .ptr{ 0LLU } }
		);

		void BatchLine(
			PrimitiveVertex const& vert0_,
			PrimitiveVertex const& vert1_
		);

		void BatchTriangle(
			PrimitiveVertex const& vert0_,
			PrimitiveVertex const& vert1_,
			PrimitiveVertex const& vert2_
		);

	public:
		PrimitiveManager();
		~PrimitiveManager();

	private:
		std::unique_ptr<class LineManager> LineManager_;
		std::unique_ptr<class TriangleManager> TriangleManager_;

		D3D12::RootSignature RS_;
		D3D12::Shader VS_;
		D3D12::Shader PS_;

		D3D12::UploadBuffer UB_VP_;
	};
}