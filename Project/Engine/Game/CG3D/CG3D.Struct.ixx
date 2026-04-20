module;

#include<array>
#include<vector>
#include<map>

export module Lumina.CG3D.Struct;

import <algorithm>;

import <d3d12.h>;

import <span>;
import <ranges>;
import <optional>;

import <string>;
import <format>;

import Lumina.CG3D.ASSIMP;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Core.String;
import Lumina.Core.Debug;

import Lumina.D3D12;

namespace Lumina::CG3D {
	export class TRANSFORM {
	public:
		Math::F32x3 Scale;
		Math::Versor Rotate;
		Math::F32x3 Translate;
	};

	export class Animation {
	public:
		template<typename _ATTR>
		struct Keyframe {
			_ATTR Value;
			F32 TimepointInSecond;
		};

		template<typename _ATTR>
		struct Curve {
			std::vector<Keyframe<_ATTR>> Keyframes;
		};
	};

	export struct MyAnimation {
		struct Node {
			Animation::Curve<Math::F32x3> Scale;
			Animation::Curve<Math::Versor> Rotate;
			Animation::Curve<Math::F32x3> Translate;
		};

		std::map<std::string, Node> Nodes;
		F32 DurationInSeconds;
	};

	export class Node {
	public:
		Math::F32x4x4<> Transform_Local;
		TRANSFORM Transform;
		std::string Name;
		std::vector<U32> Indices_Mesh;
		std::vector<Node> Children;
	};

	export struct Joint {
		Math::F32x4x4<> Local;
		Math::F32x4x4<> SkeletonSpace;
		TRANSFORM Transform;
		std::string Name;
		U32 ID;
		/// ID of the parent Joint, std::nullopt if this Joint is the root
		std::optional<U32> ID_Parent;
		std::vector<U32> IDs_Child;
	};

	export struct Skeleton {
		U32 ID_Root;
		std::map<std::string, U32> IDX_Joint;
		std::vector<Joint> ARR_Joint;
	};

	export class Material {
	public:
		std::string FilePath_Diffuse;
	};

	export class VertexWeightData {
	public:
		F32 Weight;
		U32 VertexID;
	};

	export class JointWeightData {
	public:
		Math::F32x4x4<> INV_BindPose;
		std::vector<VertexWeightData> VertexWeights;
	};

	export class VertexInfluence {
	public:
		constexpr static U32 MAXNUM_Influences{ 4U };

	public:
		std::array<F32, MAXNUM_Influences> ARR_Weight;
		std::array<U32, MAXNUM_Influences> ARR_JointID;
	};

	export class WellForGPU {
	public:
		Math::F32x4x4<> SkeletonSpace;
		Math::F32x4x4<> TR_INV_SkeletonSpace;
	};

	export class SkinCluster {
	public:
		std::vector<Math::F32x4x4<>> ARR_INV_BindPose;

		D3D12::UploadBuffer InfluenceResource;
		D3D12_VERTEX_BUFFER_VIEW InfluenceBufferView;
		std::span<VertexInfluence> MappedInfluence;

		D3D12::UploadBuffer PaletteResource;
		std::span<WellForGPU> MappedPalette;
		Pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> PaletteSRVHandle;
	};

	export class Mesh {
	public:
		struct Vertex {
			Math::F32x3 Position;
			Math::F32x2 TexCoord;
			Math::F32x3 Normal;
		};
	public:
		std::string Name;

		std::vector<Vertex> Vertices;

		std::map<std::string, JointWeightData> SkinClusterData;

		U32 Index_Material;
	};

	export class Collection {
	public:
		class Importer;

	public:
		Node Root;
		std::vector<Mesh> Meshes;
		std::vector<Material> Materials;
	};

	class Collection::Importer {
	public:
		auto ReadFromFile(
			Collection& out_,
			std::string_view fileName_,
			std::string_view dirPath_
		) const -> void;
	};
}