export module Lumina.CG3D.Animation;

import <algorithm>;

import <d3d12.h>;

import <vector>;

import <span>;
import <ranges>;
import <optional>;

import <string>;
import <format>;

import Lumina.CG3D.ASSIMP;
import Lumina.CG3D.Struct;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Core.String;
import Lumina.Core.Debug;

import Lumina.D3D12;
import Lumina.D3D12.Aux.View;

/// TODO: study the structure of glTF

namespace Lumina::CG3D {
	namespace {

		/// Creates joints from ALL nodes regardless of being related to the animation.
		auto CreateJoint(
			std::vector<Joint>& joints_,
			std::optional<U32> const& id_Parent_,
			Node const& node_
		) -> U32 {
			auto& joint{ joints_.emplace_back() };
			{
				joint.Name = node_.Name;
				joint.Local = node_.Transform_Local;
				joint.SkeletonSpace = Math::F32x4x4<>::Identity;
				joint.Transform = node_.Transform;
				joint.ID = static_cast<U32>(joints_.size()) - 1U;
				joint.ID_Parent = id_Parent_;
				joint.IDs_Child = {};
			}

			U32 const id{ joint.ID };

			if (!node_.Children.empty()) {
				for (auto const& child : node_.Children) {
					U32 const id_Child{ CreateJoint(joints_, id, child) };
					joints_[id].IDs_Child.emplace_back(id_Child);
				}
			}

			return joint.ID;
		}
	}

	export auto CreateSkeleton(Node const& rootNode_) -> Skeleton {
		Skeleton ret{};
		for (auto const& child : rootNode_.Children) {
			ret.ID_Root = CreateJoint(ret.ARR_Joint, std::nullopt, child);
		}

		for (Joint const& joint : ret.ARR_Joint) {
			ret.IDX_Joint.emplace(joint.Name, joint.ID);
		}

		return ret;
	}
}

namespace Lumina::CG3D {
	namespace {
		void ProcessAnimation(
			MyAnimation& anim_OUT_,
			ASSIMP::Animation const& anim_IN_
		) {
			anim_OUT_.DurationInSeconds = static_cast<F32>(
				anim_IN_.mDuration /
				anim_IN_.mTicksPerSecond
			);
			for (
				auto const* nodeAnim_IN :
				std::span{ anim_IN_.mChannels, anim_IN_.mNumChannels }
			) {
				auto& nodeAnim_OUT{ anim_OUT_.Nodes[nodeAnim_IN->mNodeName.data] };
				for (
					auto const& keyframe_IN :
					std::span{ nodeAnim_IN->mPositionKeys, nodeAnim_IN->mNumPositionKeys }
				) {
					auto& keyframe_OUT{ nodeAnim_OUT.Translate.Keyframes.emplace_back() };
					auto const& value_IN{ keyframe_IN.mValue };
					keyframe_OUT.Value = { -value_IN.x, value_IN.y, value_IN.z };
					keyframe_OUT.TimepointInSecond = static_cast<F32>(
						keyframe_IN.mTime /
						anim_IN_.mTicksPerSecond
					);
				}
				for (
					auto const& keyframe_IN :
					std::span{ nodeAnim_IN->mRotationKeys, nodeAnim_IN->mNumRotationKeys }
				) {
					auto& keyframe_OUT{ nodeAnim_OUT.Rotate.Keyframes.emplace_back() };
					auto const& value_IN{ keyframe_IN.mValue };
					/// Right-hand system to left-hand system;
					/// the rotation orientation and the axis is thus reversed. 
					keyframe_OUT.Value = { value_IN.x, -value_IN.y, -value_IN.z, value_IN.w };
					keyframe_OUT.TimepointInSecond = static_cast<F32>(
						keyframe_IN.mTime /
						anim_IN_.mTicksPerSecond
					);
				}
			}
		}
	}

	export auto LoadAnimationFile(
		StringView fileName_,
		StringView directoryPath_
	) -> std::vector<MyAnimation> {
		std::string filePath{ directoryPath_.Data() };
		filePath += '/';
		filePath += fileName_.Data();

		ASSIMP::Importer importer{};
		ASSIMP::Scene const* scene{
			importer.ReadFile(
				filePath.data(),
				ASSIMP::PostProcessStep::FlipWindingOrder |
				ASSIMP::PostProcessStep::FlipUVs
			)
		};
		(scene->HasAnimations()) ||
			Debug::ThrowIfFalse<>{ "No animations in the scene!\n" };

		std::vector<MyAnimation> animations{};

		for (
			auto const* anim_IN :
			std::span{ scene->mAnimations, scene->mNumAnimations }
		) {
			auto& anim_OUT{ animations.emplace_back() };
			ProcessAnimation(anim_OUT, *anim_IN);
		}

		return animations;
	}

	namespace {
		auto SRT(TRANSFORM const& transform_) noexcept -> Math::F32x4x4<> {
			Math::F32x4x4<> ret{ Math::SE3{ *reinterpret_cast<Math::Versor const*>(&transform_.Rotate) } };
			ret[0] *= transform_.Scale.X;
			ret[1] *= transform_.Scale.Y;
			ret[2] *= transform_.Scale.Z;
			ret[3] = Math::F32x4{ transform_.Translate, 1.0f };
			return ret;
		}
	}

	export void Update(Skeleton& skeleton_) {
		for (auto& joint : skeleton_.ARR_Joint) {
			joint.Local = SRT(joint.Transform);
			if (joint.ID_Parent) {
				auto const& jointParent{ skeleton_.ARR_Joint[*joint.ID_Parent] };
				joint.SkeletonSpace = joint.Local * jointParent.SkeletonSpace;
			}
			else {
				joint.SkeletonSpace = joint.Local;
			}
		}
	}

	namespace {
		void Calculate(
			Math::F32x3& out_,
			Animation::Curve<Math::F32x3> const& in_,
			F32 time_
		) {
			if (in_.Keyframes.size() == 1 || time_ <= in_.Keyframes[0].TimepointInSecond) {
				out_ = in_.Keyframes[0].Value;
			}
			else {
				for (auto it{ in_.Keyframes.cbegin() }; it != in_.Keyframes.cend() - 1; ++it) {
					auto const& cur{ *it };
					auto const& next{ *(it + 1) };
					if (cur.TimepointInSecond <= time_ && time_ <= next.TimepointInSecond) {
						F32 const t{
							(time_ - cur.TimepointInSecond) /
							(next.TimepointInSecond - cur.TimepointInSecond)
						};

						out_ = Math::LERP{
							static_cast<Math::F32x3>(cur.Value),
							static_cast<Math::F32x3>(next.Value)
						}(t);
						return;
					}
				}
			}
		}
		void Calculate(
			F32x4& out_,
			Animation::Curve<F32x4> const& in_,
			F32 time_
		) {
			if (in_.Keyframes.size() == 1 || time_ <= in_.Keyframes[0].TimepointInSecond) {
				out_ = in_.Keyframes[0].Value;
			}
			else {
				for (auto it{ in_.Keyframes.cbegin() }; it != in_.Keyframes.cend() - 1; ++it) {
					auto const& cur{ *it };
					auto const& next{ *(it + 1) };
					if (cur.TimepointInSecond <= time_ && time_ <= next.TimepointInSecond) {
						F32 const t{
							(time_ - cur.TimepointInSecond) /
							(next.TimepointInSecond - cur.TimepointInSecond)
						};
						/*auto rotate = Math::SLERP{
							Math::Quaternion{ cur.Value.X, cur.Value.Y, cur.Value.Z, cur.Value.W },
							Math::Quaternion{ next.Value.X, next.Value.Y, next.Value.Z, next.Value.W }
						}(t);*/
						auto rotate = Math::LERP{
							Math::F32x4{ &cur.Value.X },
							Math::F32x4{ &next.Value.X }
						}(t);
						out_ = *reinterpret_cast<F32x4 const*>(&rotate);
						return;
					}
				}
			}
		}
		void Calculate(
			TRANSFORM& out_,
			MyAnimation::Node const& in_,
			F32 time_
		) {
			Calculate(out_.Rotate, in_.Rotate, time_);
			Calculate(out_.Translate, in_.Translate, time_);
		}
	}

	export void ApplyAnimation(Skeleton& skeleton_, MyAnimation const& anim_, F32 time_) {
		for (auto& joint : skeleton_.ARR_Joint) {
			auto it{ anim_.Nodes.find(joint.Name) };
			if (it != anim_.Nodes.cend()) {
				auto const& animNode_Root{ it->second };
				Calculate(joint.Transform, animNode_Root, time_);
			}
		}
	}
}

namespace Lumina::CG3D {
	namespace {
		constexpr auto IndexRange(U32 n_) {
			return std::ranges::iota_view{ 0U, n_ };
		}
	}

	export auto CreateSkinCluster(
		SkinCluster& skinCluster_,
		D3D12::GraphicsDevice const& d3d12Device_,
		D3D12::DescriptorHeap const& globalHeap_,
		Skeleton const& skeleton_,
		Mesh const& mesh_
	) -> void {
		skinCluster_.PaletteResource.Initialize(
			d3d12Device_,
			sizeof(WellForGPU) * skeleton_.ARR_Joint.size()
		);
		skinCluster_.MappedPalette = {
			reinterpret_cast<WellForGPU*>(skinCluster_.PaletteResource()),
			skeleton_.ARR_Joint.size()
		};

		auto globalTable{ globalHeap_.Allocate(1U) };
		skinCluster_.PaletteSRVHandle.first = globalTable.CPUHandle(0U);
		skinCluster_.PaletteSRVHandle.second = globalTable.GPUHandle(0U);
		D3D12::SRV<WellForGPU>::Create(
			d3d12Device_,
			skinCluster_.PaletteSRVHandle.first,
			skinCluster_.PaletteResource
		);

		skinCluster_.InfluenceResource.Initialize(
			d3d12Device_,
			sizeof(VertexInfluence) * mesh_.Vertices.size()
		);
		skinCluster_.MappedInfluence = {
			reinterpret_cast<VertexInfluence*>(skinCluster_.InfluenceResource()),
			mesh_.Vertices.size()
		};
		auto influenceBufferView{ D3D12::VBV::Create<VertexInfluence>(skinCluster_.InfluenceResource) };
		skinCluster_.InfluenceBufferView = *reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW*>(&influenceBufferView);

		skinCluster_.ARR_INV_BindPose.resize(skeleton_.ARR_Joint.size());
		std::generate(
			skinCluster_.ARR_INV_BindPose.begin(),
			skinCluster_.ARR_INV_BindPose.end(),
			[]() { return Math::F32x4x4<>::Identity; }
		);

		for (auto const& jointWeight : mesh_.SkinClusterData) {
			auto it{ skeleton_.IDX_Joint.find(jointWeight.first) };
			if (it == skeleton_.IDX_Joint.cend()) { continue; }

			skinCluster_.ARR_INV_BindPose[it->second] = jointWeight.second.INV_BindPose;
			for (auto const& vertexWeight : jointWeight.second.VertexWeights) {
				auto& influence{ skinCluster_.MappedInfluence[vertexWeight.VertexID] };

				for (U32 idx : IndexRange(VertexInfluence::MAXNUM_Influences)) {
					if (influence.ARR_Weight[idx] == 0.0f) {
						influence.ARR_Weight[idx] = vertexWeight.Weight;
						influence.ARR_JointID[idx] = it->second;
						break;
					}
				}
			}
		}
	}

	export void Update(SkinCluster& skinCluster_, Skeleton const& skeleton_) {
		for (U32 jointID{ 0 }; jointID < skeleton_.ARR_Joint.size(); ++jointID) {
			skinCluster_.MappedPalette[jointID].SkeletonSpace =
				skinCluster_.ARR_INV_BindPose[jointID] *
				skeleton_.ARR_Joint[jointID].SkeletonSpace;
			skinCluster_.MappedPalette[jointID].TR_INV_SkeletonSpace =
				skinCluster_.MappedPalette[jointID].SkeletonSpace.Inverse().Transpose();
		}
	}

	export void Update(
		SkinCluster& skinCluster_,
		Skeleton& skeleton_,
		MyAnimation const& anim_,
		F32 time_
	) {
		ApplyAnimation(skeleton_, anim_, time_);
		Update(skeleton_);
		Update(skinCluster_, skeleton_);
	}
}