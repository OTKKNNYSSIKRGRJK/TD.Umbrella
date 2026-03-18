export module Lumina.Utils.Data.Mesh;

import <vector>;

import <string>;

import Lumina.Core.Common;
import Lumina.Core.Math;

import Lumina.Utils.Data;

namespace Lumina::Utils {
	export class Mesh {
	public:
		struct Vertex {
			U32 Index_Position;
			U32 Index_TexCoord;
			U32 Index_Normal;
			U32 Index_Tangent;
		};

	public:
		static Math::F32x3 CalculateTangent(
			Math::F32x3 const& pos0_, Math::F32x2 const& uv0_,
			Math::F32x3 const& pos1_, Math::F32x2 const& uv1_,
			Math::F32x3 const& pos2_, Math::F32x2 const& uv2_
		) {
			Math::F32x3 const dPos01{ pos1_ - pos0_ };
			Math::F32x3 const dPos02{ pos2_ - pos0_ };
			Math::F32x2 const dUV01{ uv1_ - uv0_ };
			Math::F32x2 const dUV02{ uv2_ - uv0_ };

			float const inv_Det{ 1.0f / Math::F32x2::Cross(dUV01, dUV02) };

			return inv_Det * (dUV02.Y * dPos01 - dUV01.Y * dPos02);
		}

	public:
		static auto Load(WavefrontOBJ const& obj_) -> std::vector<Mesh> {
			std::vector<Mesh> meshes{};
			meshes.resize(obj_.Num_Objects());

			auto const& names{ obj_.ObjectNames() };
			auto const& offsets{ obj_.ObjectOffsets() };

			for (U32 idx_Mesh{ 0U }; idx_Mesh < obj_.Num_Objects(); ++idx_Mesh) {
				Mesh& mesh{ meshes[idx_Mesh] };

				mesh.Name = names[idx_Mesh];

				// Positions
				{
					U32 const idx_Position_Begin{ offsets[idx_Mesh].Index_Position };
					U32 const idx_Position_End{ offsets[idx_Mesh + 1U].Index_Position };
					U32 const num_Positions{ idx_Position_End - idx_Position_Begin };
					mesh.Positions.resize(num_Positions);
					mesh.Positions.assign(
						obj_.Positions().cbegin() + idx_Position_Begin,
						obj_.Positions().cbegin() + idx_Position_End
					);
					for (auto& pos : mesh.Positions) {
						pos.Z = -pos.Z;
					}
				}

				// TexCoords
				{
					U32 const idx_TexCoord_Begin{ offsets[idx_Mesh].Index_TexCoord };
					U32 const idx_TexCoord_End{ offsets[idx_Mesh + 1U].Index_TexCoord };
					U32 const num_TexCoords{ idx_TexCoord_End - idx_TexCoord_Begin };
					mesh.TexCoords.resize(num_TexCoords);
					mesh.TexCoords.assign(
						obj_.TexCoords().cbegin() + idx_TexCoord_Begin,
						obj_.TexCoords().cbegin() + idx_TexCoord_End
					);
					for (auto& texCoord : mesh.TexCoords) {
						texCoord.Y = 1.0f - texCoord.Y;
					}
				}

				// Normals
				{
					U32 const idx_Normal_Begin{ offsets[idx_Mesh].Index_Normal };
					U32 const idx_Normal_End{ offsets[idx_Mesh + 1U].Index_Normal };
					U32 const num_Normals{ idx_Normal_End - idx_Normal_Begin };
					mesh.Normals.resize(num_Normals);
					mesh.Normals.assign(
						obj_.Normals().cbegin() + idx_Normal_Begin,
						obj_.Normals().cbegin() + idx_Normal_End
					);
					for (auto& norm : mesh.Normals) {
						norm.Z = -norm.Z;
					}
				}

				// Vertices & tangents
				{
					auto const& faces{ obj_.Faces().data() };
					auto const& verts{ obj_.Vertices().data() };

					auto const& positions{ obj_.Positions().data() };
					auto const& texCoords{ obj_.TexCoords().data() };

					U32 const idx_Face_Begin{ offsets[idx_Mesh].Index_Face };
					U32 const idx_Face_End{ offsets[idx_Mesh + 1U].Index_Face };

					U32 const idxOffset_Position{ offsets[idx_Mesh].Index_Position };
					U32 const idxOffset_TexCoord{ offsets[idx_Mesh].Index_TexCoord };
					U32 const idxOffset_Normal{ offsets[idx_Mesh].Index_Normal };

					for (
						U32 idx_Face{ idx_Face_Begin };
						idx_Face < idx_Face_End;
						++idx_Face
					) {
						auto const& face{ faces[idx_Face] };

						I32 idx_Verts[3]{
							static_cast<I32>(face.Index_Vertex_Last),
							static_cast<I32>(face.Index_Vertex_Last) - 1,
							static_cast<I32>(face.Index_Vertex_Last) - 2,
						};

						U32 const idx_Tangent{ idx_Face - idx_Face_Begin };
						Math::F32x3&& tangent{
							CalculateTangent(
								positions[verts[idx_Verts[0]].Index_Position],
								texCoords[verts[idx_Verts[0]].Index_TexCoord],
								positions[verts[idx_Verts[1]].Index_Position],
								texCoords[verts[idx_Verts[1]].Index_TexCoord],
								positions[verts[idx_Verts[2]].Index_Position],
								texCoords[verts[idx_Verts[2]].Index_TexCoord]
							)
						};
						mesh.Tangents.emplace_back(tangent);

						do {
							for (I32 idx_Vert : idx_Verts) {
								auto const& vert{ verts[idx_Vert] };
								mesh.Vertices.emplace_back(
									vert.Index_Position - idxOffset_Position,
									vert.Index_TexCoord - idxOffset_TexCoord,
									vert.Index_Normal - idxOffset_Normal,
									idx_Tangent
								);
							}

							--idx_Verts[1];
							--idx_Verts[2];
						} while (idx_Verts[2] >= static_cast<I32>(face.Index_Vertex_First));
					}
				}
			}

			return meshes;
		}

	public:
		std::string Name;

		std::vector<Math::F32x3> Positions;
		std::vector<Math::F32x2> TexCoords;
		std::vector<Math::F32x3> Normals;
		std::vector<Math::F32x3> Tangents;

		std::vector<Vertex> Vertices;
	};
}