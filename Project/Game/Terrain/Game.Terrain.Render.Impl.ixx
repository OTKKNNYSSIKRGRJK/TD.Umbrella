export module Game.Terrain : Render.Impl;

import <vector>;
import <memory>;
import <format>;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Core.String;
import Lumina.D3D12;
import Lumina.D3D12.Aux.View;
import Lumina.Main;
import : Shape;
import Lumina.MeshManager;

namespace Game::Impl {
	export class TerrainRenderer {
		struct Material {
			// * Texture where material IDs are written
			Lumina::U32 MaterialMap;
			// * Texture of gradient (vector field) of a scalar noise map
			Lumina::U32 BlendMap;
		};

	public:
		auto PrepareMesh(TerrainShapeCollection const& shapeCollection_) -> void;

	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Update(_ARGs&&...args_) -> void;
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Render(_ARGs&&...args_) -> void;

	public:
		auto Update() -> void;
		template<typename..._ARGs>
		auto Render(_ARGs&&...args_) -> void;

	private:
		template<Lumina::StringLiteral _Name, typename..._ARGs>
		auto Initialize(_ARGs&&...args_) -> void;

	public:
		auto Initialize() -> void;

	private:
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_ImageTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_Noise_;

	private:
		std::vector<Lumina::MeshShaderAsset> MeshShaderAssets_;

		Lumina::D3D12::UploadBuffer VertexBuffer_XY_;
		Lumina::D3D12::UploadBuffer VertexBuffer_Z_;
		Lumina::D3D12::VertexBufferView VBV_;

		constexpr static Lumina::U32 MaxNum_Vertices_{ 1024U };

		std::vector<Material> Materials_;
		std::vector<std::unique_ptr<Lumina::D3D12::UploadBuffer>> UB_Materials_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Materials_;

		struct Vertex {
			Lumina::F32x4 Position;
			Lumina::F32x2 TexCoord;
			Lumina::F32x3 Normal;
			Lumina::F32x3 Tangent;
		};

	private:
		Lumina::D3D12::RootSignature RS_Geometry_;
		Lumina::D3D12::Shader VS_Geometry_;
		Lumina::D3D12::Shader PS_Geometry_;
		Lumina::D3D12::GraphicsPSO PSO_Geometry_;
	};
}

namespace Game::Impl {
	auto TerrainRenderer::PrepareMesh(TerrainShapeCollection const& shapeCollection_) -> void {
		Lumina::Utils::Mesh mesh_Nonground{};

		auto const& polygons{ shapeCollection_.PolygonsData() };
		for (auto const& polygon : polygons) {
			if (polygon.Vertices.size() > 2LLU) {

				for (auto const& vert : polygon.Vertices) {
					mesh_Nonground.Positions.emplace_back(
						vert.Pos.X,
						vert.Pos.Y,
						vert.Pos.Z - 0.05f
					);
				}

				for (auto const& vert : polygon.Vertices) {
					mesh_Nonground.TexCoords.emplace_back(
						vert.Pos.X * 0.5f,
						vert.Pos.Y * 0.5f
					);
				}

				mesh_Nonground.Normals.emplace_back(Lumina::Math::F32x3{ 0.0f, 0.0f, -1.0f });
				mesh_Nonground.Tangents.emplace_back(Lumina::Math::F32x3{ 0.0f, -1.0f, 0.0f });

				for (Lumina::U32 i{ 2U }; i < static_cast<Lumina::U32>(polygon.Vertices.size()); ++i) {
					mesh_Nonground.Vertices.emplace_back(0, 0, 0, 0);
					mesh_Nonground.Vertices.emplace_back(i - 1, i - 1, 0, 0);
					mesh_Nonground.Vertices.emplace_back(i, i, 0, 0);
				}
			}
		}

		Lumina::Utils::Mesh mesh_Ground{};

		mesh_Ground.Normals.emplace_back(Lumina::Math::F32x3{ 0.0f, 0.0f, -1.0f });
		mesh_Ground.Tangents.emplace_back(Lumina::Math::F32x3{ 0.0f, -1.0f, 0.0f });

		auto const& ground{ shapeCollection_.GroundData() };
		for (auto const& collider : ground.Colliders) {
			auto const& verts{ collider->GetVertices() };

			for (auto const& vert : verts) {
				mesh_Ground.Positions.emplace_back(vert.X, vert.Y, vert.Z - 0.05f);
				mesh_Ground.TexCoords.emplace_back(
					vert.X * 0.5f,
					vert.Y * 0.5f
				);
			}

			Lumina::U32 offset{ static_cast<Lumina::U32>(mesh_Ground.Positions.size()) - 4U };
			mesh_Ground.Vertices.emplace_back(offset + 0, offset + 0, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset + 1, offset + 1, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset + 3, offset + 3, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset + 1, offset + 1, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset + 2, offset + 2, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset + 3, offset + 3, 0, 0);
		}

		auto const& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };

		Lumina::MeshUploader meshUploader{};
		meshUploader.Initialize(d3d12Context);
		meshUploader.Begin();
		meshUploader.Batch(mesh_Nonground);
		meshUploader.Batch(mesh_Ground);
		meshUploader.End(MeshShaderAssets_);
	}
}

namespace Game::Impl {
	template<>
	auto TerrainRenderer::Render<"Batch">(
		Lumina::Math::F32x4x4<> const& world_
	) -> void {
		auto& meshMngr{ Lumina::Context::Instance().MeshContext() };
		meshMngr.Batch(
			MeshShaderAssets_[0],
			1U,
			LocalHeap_Materials_.CPUHandle(0U),
			world_
		);
		meshMngr.Batch(
			MeshShaderAssets_[1],
			1U,
			LocalHeap_Materials_.CPUHandle(1U),
			world_
		);
		/*meshMngr.Batch(
			MeshShaderAssets_[2],
			1U,
			LocalHeap_Materials_.CPUHandle(2U),
			world_
		);*/
	}

	template<>
	auto TerrainRenderer::Render(
		Lumina::Math::F32x4x4<> const& world_
	) -> void {
		Render<"Batch">(world_);
	}
}

namespace Game::Impl {
	template<>
	auto TerrainRenderer::Initialize<"Materials : ImageTextures">() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto& resMngr{ context.ResourceContext() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		auto entryOfImageToLoad{
			[] (
				std::string_view name_,
				std::string_view type_
			) -> std::pair<std::string, std::string> {
				std::string name{ std::format("{}.{}", name_, type_) };
				std::string filePath{ std::format("Assets/Img/Terrain/{}/{}.png", name_, type_) };
				return std::pair{ std::move(name), std::move(filePath) };
			}
		};

		auto appendListOfImageToLoad{
			[&] (
				std::vector<std::pair<std::string, std::string>> list_,
				std::string_view name_
			) -> void {
				list_.emplace_back(entryOfImageToLoad(name_, "Albedo"));
				list_.emplace_back(entryOfImageToLoad(name_, "Normal"));
				list_.emplace_back(entryOfImageToLoad(name_, "Height"));
			}
		};

		std::vector<std::pair<std::string, std::string>> imageTexturesToLoad{};
		appendListOfImageToLoad(imageTexturesToLoad, "Soil0");
		appendListOfImageToLoad(imageTexturesToLoad, "Soil1");
		appendListOfImageToLoad(imageTexturesToLoad, "Moss0");
		appendListOfImageToLoad(imageTexturesToLoad, "Moss1");
		appendListOfImageToLoad(imageTexturesToLoad, "Rock0");
		appendListOfImageToLoad(imageTexturesToLoad, "Rock1");
		appendListOfImageToLoad(imageTexturesToLoad, "Bricks0");
		appendListOfImageToLoad(imageTexturesToLoad, "Bricks1");

		std::vector<uint32_t> texIDs{};
		resMngr.Graphics().LoadImageTextures(
			texIDs,
			imageTexturesToLoad
		);

		GlobalTable_SRV_ImageTexture_ = d3d12Context.GlobalDescriptorHeap().Allocate(
			static_cast<Lumina::U32>(imageTexturesToLoad.size())
		);
		for (Lumina::U32 idx{ 0U }; idx < static_cast<uint32_t>(texIDs.size()); ++idx) {
			d3d12Device->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_ImageTexture_.CPUHandle(idx),
				resMngr.Graphics().CPUHandle(texIDs.at(idx)),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}
	}

	template<>
	auto TerrainRenderer::Initialize<"Materials : Buffers & CBVs">() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		// * Upload Buffers for Stroing Materials
		UB_Materials_.resize(64U);
		for (auto& ub : UB_Materials_) {
			ub = std::make_unique<Lumina::D3D12::UploadBuffer>();
			ub->Initialize(d3d12Device, 256LLU);
		}

		// * Local Descriptor Heap for Materials
		LocalHeap_Materials_.Initialize(
			d3d12Device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			64U,
			false
		);

		// * CBV Creation
		for (Lumina::U32 idx{ 0U }; idx < 64U; ++idx) {
			Lumina::D3D12::CBV::Create(
				d3d12Device,
				LocalHeap_Materials_.CPUHandle(0U),
				*UB_Materials_[0]
			);
		}

		Materials_.resize(64U);
		Materials_[0].MaterialMap = 0U;
		Materials_[0].BlendMap = 0U;
		UB_Materials_[0]->Store(&Materials_[0], sizeof(Material), 0LLU);
	}

	template<>
	auto TerrainRenderer::Initialize<"Pipeline">() -> void {
	}

	auto TerrainRenderer::Initialize() -> void {
		Initialize<"Materials : ImageTextures">();
		Initialize<"Materials : Buffers & CBVs">();
		Initialize<"Pipeline">();
	}
}