module Game.Terrain;

import : Render;
import : Shape;

import : Render.Impl;

import Lumina.D3D12;
import Lumina.Main;
import Lumina.D3D12.Aux;
import Lumina.Utils.Data;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace Game::Impl {
	auto TerrainRenderer::PrepareMesh(TerrainShapeCollection const& shapeCollection_) -> void {
		auto const& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		Lumina::Utils::Mesh mesh_Nonground{};

		auto const& polygons{ shapeCollection_.PolygonsData() };
		for (auto const& polygon : polygons) {
			if (polygon.Vertices.size() > 2LLU) {

				mesh_Nonground.Normals.emplace_back(Lumina::Math::F32x3{ 0.0f, 0.0f, -1.0f });
				mesh_Nonground.Tangents.emplace_back(Lumina::Math::F32x3{ 0.0f, -1.0f, 0.0f });

				// * XY

				for (auto const& vert : polygon.Vertices) {
					mesh_Nonground.Positions.emplace_back(
						vert.Pos.X,
						vert.Pos.Y,
						vert.Pos.Z - 2.5f
					);
				}

				for (auto const& vert : polygon.Vertices) {
					mesh_Nonground.TexCoords.emplace_back(
						vert.Pos.X * 0.03125f,
						vert.Pos.Y * 0.03125f
					);
				}

				for (Lumina::U32 i{ 2U }; i < static_cast<Lumina::U32>(polygon.Vertices.size()); ++i) {
					mesh_Nonground.Vertices.emplace_back(0, 0, 0, 0);
					mesh_Nonground.Vertices.emplace_back(i - 1, i - 1, 0, 0);
					mesh_Nonground.Vertices.emplace_back(i, i, 0, 0);
				}

				// * Z

				Lumina::U32 const num_Verts{ static_cast<Lumina::U32>(polygon.Vertices.size()) };

				for (auto const& vert : polygon.Vertices) {
					mesh_Nonground.Positions.emplace_back(
						vert.Pos.X,
						vert.Pos.Y,
						vert.Pos.Z + 2.5f
					);
				}

				for (auto const& vert : polygon.Vertices) {
					mesh_Nonground.TexCoords.emplace_back(
						vert.Pos.X * 0.03125f,
						vert.Pos.Y * 0.03125f
					);
				}

				for (Lumina::U32 i{ 0U }; i < static_cast<Lumina::U32>(polygon.Vertices.size()); ++i) {
					auto const& pos0{ polygon.Vertices[i].Pos };
					auto const& pos1{ polygon.Vertices[(i + 1) % num_Verts].Pos };
					auto dPos{ pos1 - pos0 };
					dPos = dPos.Unit();

					mesh_Nonground.Normals.emplace_back(
						-dPos.Y,
						dPos.X,
						0.0f
					);
				}

				mesh_Nonground.Tangents.emplace_back(Lumina::Math::F32x3{ 0.0f, 0.0f, 1.0f });

				for (Lumina::U32 i{ 0U }; i < static_cast<Lumina::U32>(polygon.Vertices.size()); ++i) {
					Lumina::U32 const indices[6]{
						i,
						i + num_Verts,
						(i + num_Verts + 1) % (num_Verts * 2),
						i,
						(i + num_Verts + 1) % (num_Verts * 2),
						i + 1,
					};
					mesh_Nonground.Vertices.emplace_back(
						indices[0],
						indices[0],
						i + 1,
						1
					);
					mesh_Nonground.Vertices.emplace_back(
						indices[1],
						indices[1],
						i + 1,
						1
					);
					mesh_Nonground.Vertices.emplace_back(
						indices[2],
						indices[2],
						i + 1,
						1
					);
					mesh_Nonground.Vertices.emplace_back(
						indices[3],
						indices[3],
						i + 1,
						1
					);
					mesh_Nonground.Vertices.emplace_back(
						indices[4],
						indices[4],
						i + 1,
						1
					);
					mesh_Nonground.Vertices.emplace_back(
						indices[5],
						indices[5],
						i + 1,
						1
					);
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
				mesh_Ground.Positions.emplace_back(vert.X, vert.Y, vert.Z - 2.5f);
				mesh_Ground.TexCoords.emplace_back(
					vert.X * 0.03125f,
					vert.Y * 0.03125f
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

		Lumina::U32 tmp{ 0U };

		for (auto const& collider : ground.Colliders) {
			auto const& verts{ collider->GetVertices() };

			mesh_Ground.Positions.emplace_back(verts[0].X, verts[0].Y, verts[0].Z + 2.5f);
			mesh_Ground.Positions.emplace_back(verts[1].X, verts[1].Y, verts[1].Z + 2.5f);
			mesh_Ground.TexCoords.emplace_back(
				verts[0].X * 0.03125f,
				verts[0].Y * 0.03125f
			);
			mesh_Ground.TexCoords.emplace_back(
				verts[1].X * 0.03125f,
				verts[1].Y * 0.03125f
			);

			Lumina::U32 offset{ static_cast<Lumina::U32>(mesh_Ground.Positions.size()) - 2U };
			Lumina::U32 offset2{ tmp * 4U };
			mesh_Ground.Vertices.emplace_back(offset2 + 0, offset2 + 0, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset + 0, offset + 0, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset + 1, offset + 1, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset2 + 0, offset2 + 0, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset + 1, offset + 1, 0, 0);
			mesh_Ground.Vertices.emplace_back(offset2 + 1, offset2 + 1, 0, 0);

			++tmp;
		}

		Lumina::MeshUploader meshUploader{};
		meshUploader.Initialize(d3d12Context);
		meshUploader.Begin();
		if (mesh_Nonground.Vertices.size() > 0LLU) {
			meshUploader.Batch(mesh_Nonground);
		}
		if (mesh_Ground.Vertices.size() > 0LLU) {
			meshUploader.Batch(mesh_Ground);
		}
		meshUploader.End(MeshShaderAssets_);

		if (!MeshShaderAssets_.empty()) {
			Lumina::D3D12::CommandAllocator cmdAllocator{};
			Lumina::D3D12::CommandList cmdList{};

			cmdAllocator.Initialize(d3d12Device);
			cmdList.Initialize(d3d12Device, cmdAllocator);

			std::vector<D3D12_RESOURCE_BARRIER> barriers{};
			for (auto const& meshShaderAsset : MeshShaderAssets_) {
				barriers.emplace_back(
					Lumina::D3D12::Barrier::Transition(
						meshShaderAsset.VertexBuffer(),
						D3D12_RESOURCE_STATE_COPY_SOURCE,
						D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
					)
				);
			};
			cmdList->ResourceBarrier(
				static_cast<Lumina::U32>(barriers.size()),
				barriers.data()
			);

			auto& cmdQueue{ d3d12Context.DirectQueue() };
			cmdQueue << cmdList;
			cmdQueue.CPUWait(cmdQueue.ExecuteBatchedCommandLists());

			if (MeshShaderAssets_.size() > 1) {
				VBVs_LowPoly_[0] =
					Lumina::D3D12::VBV::Create<Lumina::Utils::Mesh::Vertex>(
						MeshShaderAssets_[0].VertexBuffer()
					);
				VBVs_LowPoly_[1] =
					Lumina::D3D12::VBV::Create<Lumina::Utils::Mesh::Vertex>(
						MeshShaderAssets_[1].VertexBuffer()
					);
			}
			else {
				VBVs_LowPoly_[0] =
					Lumina::D3D12::VBV::Create<Lumina::Utils::Mesh::Vertex>(
						MeshShaderAssets_[0].VertexBuffer()
					);
			}
		}
	}
}

namespace Game::Impl {
	template<>
	auto TerrainRenderer::Render<"LowPoly">(
		Lumina::D3D12::GraphicsDevice const& d3d12Device_,
		Lumina::D3D12::CommandList const& cmdList_,
		Lumina::D3D12::Canvas const& canvas_,
		[[maybe_unused]] Lumina::Math::F32x4x4<> const& worldToProjective_
	) -> void {
		UB_WorldToProjective_.Store(&worldToProjective_, sizeof(Lumina::Math::F32x4x4<>), 0LLU);

		D3D12_RESOURCE_BARRIER const barriers_PrePass[]{
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_LowPoly_.RenderTexture(0U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_LowPoly_.RenderTexture(1U),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_LowPoly_.DepthTexture(),
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				 D3D12_RESOURCE_STATE_DEPTH_WRITE
			 ),
		};
		cmdList_->ResourceBarrier(3U, barriers_PrePass);

		cmdList_->SetGraphicsRootSignature(RS_LowPoly_.Get());
		cmdList_->SetPipelineState(PSO_LowPoly_.Get());

		/*cmdList_->RSSetViewports(
			Canvas_LowPoly_.Num_RenderTargets(),
			Canvas_LowPoly_.Viewports().data()
		);
		cmdList_->RSSetScissorRects(
			Canvas_LowPoly_.Num_RenderTargets(),
			Canvas_LowPoly_.ScissorRects().data()
		);

		RenderPass_LowPoly_.Begin(cmdList_);*/

		D3D12_CPU_DESCRIPTOR_HANDLE const rtvs[]{
			canvas_.RTV(0U),
			canvas_.RTV(1U)
		};
		D3D12_CPU_DESCRIPTOR_HANDLE const dsv{ canvas_.DSV() };
		cmdList_->OMSetRenderTargets(2U, rtvs, false, &dsv);

		Lumina::U32 idx{ 0U };
		for (auto const& mesh : MeshShaderAssets_) {
			auto const& localSRVs{ mesh.LocalDescriptors() };

			// * Positions
			d3d12Device_->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_VertexElementArray_.CPUHandle(idx * 4U + 0U),
				localSRVs.CPUHandle(0U),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
			// * UVs
			d3d12Device_->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_VertexElementArray_.CPUHandle(idx * 4U + 1U),
				localSRVs.CPUHandle(1U),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
			// * Normals
			d3d12Device_->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_VertexElementArray_.CPUHandle(idx * 4U + 2U),
				localSRVs.CPUHandle(2U),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
			// * Tangents
			d3d12Device_->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_VertexElementArray_.CPUHandle(idx * 4U + 3U),
				localSRVs.CPUHandle(3U),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);

			cmdList_->SetGraphicsRootDescriptorTable(
				0U,
				GlobalTable_SRV_VertexElementArray_.GPUHandle(idx * 4U)
			);
			cmdList_->SetGraphicsRootDescriptorTable(
				1U,
				GlobalTable_CBV_Transforms_.GPUHandle(0U)
			);
			cmdList_->SetGraphicsRootDescriptorTable(
				2U,
				GlobalTable_CBV_PSParameters_.GPUHandle(0U)
			);
			cmdList_->SetGraphicsRootDescriptorTable(
				3U,
				GlobalTable_SRV_TerrainMaterialDatabase_.GPUHandle(0U)
			);
			cmdList_->SetGraphicsRootDescriptorTable(
				4U,
				GlobalTable_Surface_.GPUHandle(7U)
			);
			cmdList_->SetGraphicsRootDescriptorTable(
				5U,
				GlobalTable_SRV_TerrainMaterialMap_Albedo_.GPUHandle(0U)
			);

			cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList_->IASetVertexBuffers(0U, 1U, &VBVs_LowPoly_[idx]);

			cmdList_->DrawInstanced(mesh.Num_Vertices(), 1U, 0U, 0U);

			++idx;
		}

		//RenderPass_LowPoly_.End();

		D3D12_RESOURCE_BARRIER const barriers_PostPass[]{
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_LowPoly_.RenderTexture(0U),
				 D3D12_RESOURCE_STATE_RENDER_TARGET,
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_LowPoly_.RenderTexture(1U),
				 D3D12_RESOURCE_STATE_RENDER_TARGET,
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Canvas_LowPoly_.DepthTexture(),
				 D3D12_RESOURCE_STATE_DEPTH_WRITE,
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			 ),
		};
		cmdList_->ResourceBarrier(3U, barriers_PostPass);

		#if defined(_DEBUG)
		ImGui::Begin("TerrainDebug");
		ImGui::Image(GlobalTable_Surface_.GPUHandle(7U).ptr, { 320.0f, 180.0f });
		ImGui::Image(GlobalTable_Surface_.GPUHandle(8U).ptr, { 320.0f, 180.0f });
		ImGui::Image(GlobalTable_Surface_.GPUHandle(9U).ptr, { 320.0f, 180.0f });
		ImGui::End();
		#endif
	}

	template<>
	auto TerrainRenderer::Render(
		Lumina::D3D12::Canvas const& canvas_,
		Lumina::Math::F32x4x4<> const& worldToProjective_
	) -> void {
		auto const& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };
		auto const& cmdList{ context.MainCommandList() };

		Render<"LowPoly">(d3d12Device, cmdList, canvas_, worldToProjective_);
	}
}

namespace Game::Impl {
	namespace {
		Lumina::U32 NUM_Materials;
	}

	template<>
	auto TerrainRenderer::Initialize<"Material.ImageTextures">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		auto& context{ Lumina::Context::Instance() };
		auto& resMngr{ context.ResourceContext() };

		auto entryOfImageToLoad{
			[] (
				std::string_view name_,
				std::string_view type_
			) -> std::pair<std::string, std::string> {
				std::string name{ std::format("{}.{}", name_, type_) };
				std::string filePath{ std::format("Assets/Terrain/Img/{}/{}.jpg", type_, name_) };
				return std::pair{ std::move(name), std::move(filePath) };
			}
		};

		auto appendListOfImageToLoad{
			[&] (
				std::vector<std::pair<std::string, std::string>>& list_,
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
		appendListOfImageToLoad(imageTexturesToLoad, "Rock0");
		appendListOfImageToLoad(imageTexturesToLoad, "Rock1");
		appendListOfImageToLoad(imageTexturesToLoad, "Moss0");
		appendListOfImageToLoad(imageTexturesToLoad, "Moss1");
		appendListOfImageToLoad(imageTexturesToLoad, "Brick0");
		appendListOfImageToLoad(imageTexturesToLoad, "Brick1");

		std::vector<uint32_t> texIDs{};
		resMngr.Graphics().LoadImageTextures(
			texIDs,
			imageTexturesToLoad
		);

		NUM_Materials = static_cast<Lumina::U32>(imageTexturesToLoad.size()) / 3U;
		GlobalTable_SRV_TerrainMaterialMap_Albedo_ =
			d3d12Context_.GlobalDescriptorHeap().Allocate(32U);
		GlobalTable_SRV_TerrainMaterialMap_Normal_ =
			d3d12Context_.GlobalDescriptorHeap().Allocate(32U);
		GlobalTable_SRV_TerrainMaterialMap_Height_ =
			d3d12Context_.GlobalDescriptorHeap().Allocate(32U);
		for (Lumina::U32 idx_Material{ 0U }; idx_Material < NUM_Materials; ++idx_Material) {
			d3d12Device_->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_TerrainMaterialMap_Albedo_.CPUHandle(idx_Material),
				resMngr.Graphics().CPUHandle(texIDs.at(idx_Material * 3U + 0U)),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
			d3d12Device_->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_TerrainMaterialMap_Normal_.CPUHandle(idx_Material),
				resMngr.Graphics().CPUHandle(texIDs.at(idx_Material * 3U + 1U)),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
			d3d12Device_->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_TerrainMaterialMap_Height_.CPUHandle(idx_Material),
				resMngr.Graphics().CPUHandle(texIDs.at(idx_Material * 3U + 2U)),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}
	}

	template<>
	auto TerrainRenderer::Initialize<"Material.Database">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		GlobalTable_SRV_TerrainMaterialDatabase_ = d3d12Context_.GlobalDescriptorHeap().Allocate(1U);

		UB_TerrainMaterialDatabase_.Initialize(
			d3d12Device_,
			sizeof(TerrainMaterial) * TerrainMaterialDatabase_.size()
		);

		Lumina::D3D12::SRV<TerrainMaterial>::Create(
			d3d12Device_,
			GlobalTable_SRV_TerrainMaterialDatabase_.CPUHandle(0U),
			UB_TerrainMaterialDatabase_
		);

		for (Lumina::U32 idx_Material{ 0U }; idx_Material < NUM_Materials; ++idx_Material) {
			TerrainMaterialDatabase_[idx_Material] = {
				idx_Material,
				idx_Material,
				idx_Material
			};
		}
		UB_TerrainMaterialDatabase_.Store(
			TerrainMaterialDatabase_.data(),
			sizeof(TerrainMaterial) * TerrainMaterialDatabase_.size(),
			0LLU
		);
	}

	template<>
	auto TerrainRenderer::Initialize<"Material">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		Initialize<"Material.ImageTextures">(d3d12Context_, d3d12Device_);
		Initialize<"Material.Database">(d3d12Context_, d3d12Device_);
	}

	template<>
	auto TerrainRenderer::Initialize<"LowPoly.Canvas">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		Canvas_LowPoly_.AllocateTextures(2U, true);
		
		// * Albedo
		Canvas_LowPoly_.RenderTexture(0U).Initialize(
			d3d12Device_,
			1280U, 720U,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		);
		// * Normal
		Canvas_LowPoly_.RenderTexture(1U).Initialize(
			d3d12Device_,
			1280U, 720U,
			DXGI_FORMAT_R8G8B8A8_UNORM
		);
		// * Depth
		Canvas_LowPoly_.DepthTexture().Initialize(
			d3d12Device_,
			1280U, 720U
		);

		Canvas_LowPoly_.TransitionResourceStates(
			d3d12Device_,
			d3d12Context_.DirectQueue()
		);
		Canvas_LowPoly_.CreateViews(d3d12Device_);
		Canvas_LowPoly_.Viewport(0U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 1280.0f },
			.Height{ 720.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_LowPoly_.ScissorRect(0U) = D3D12_RECT{
			.left{ 0 },
			.top{ 0 },
			.right{ 1280 },
			.bottom{ 720 },
		};
		Canvas_LowPoly_.Viewport(1U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 0.0f },
			.Height{ 0.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_LowPoly_.ScissorRect(1U) = D3D12_RECT{
			.left{ 640 },
			.top{ 360 },
			.right{ 1280 },
			.bottom{ 720 },
		};
	}

	template<>
	auto TerrainRenderer::Initialize<"LowPoly.RenderPass">() -> void {
		Lumina::F32 const clearColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
		RenderPass_LowPoly_.Initialize(2U, true);
		RenderPass_LowPoly_.RenderTarget(0).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			clearColor
		);
		RenderPass_LowPoly_.RenderTarget(0).EndingEvent().Preserve();
		RenderPass_LowPoly_.RenderTarget(1).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			clearColor
		);
		RenderPass_LowPoly_.RenderTarget(1).EndingEvent().Preserve();
		RenderPass_LowPoly_.DepthStencil().DepthBeginningEvent().ClearTarget(
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			{ .Depth{ 1.0f }, }
		);
		RenderPass_LowPoly_.DepthStencil().DepthEndingEvent().Preserve();
		RenderPass_LowPoly_.DepthStencil().StencilBeginningEvent().NoAccess();
		RenderPass_LowPoly_.DepthStencil().StencilEndingEvent().NoAccess();

		for (uint32_t idx{ 0U }; idx < Canvas_LowPoly_.Num_RenderTargets(); ++idx) {
			RenderPass_LowPoly_.RenderTarget(idx).View() = Canvas_LowPoly_.RTV(idx);
		}
		RenderPass_LowPoly_.DepthStencil().View() = Canvas_LowPoly_.DSV();
	}

	template<>
	auto TerrainRenderer::Initialize<"LowPoly.Pipeline">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_,
		nlohmann::json const& settings_
	) -> void {
		auto rsSetup{
			Lumina::D3D12::LoadSetup<Lumina::D3D12::RootSignature>(
				settings_.at("Low Poly RS")
			)
		};
		RS_LowPoly_.Initialize(d3d12Device_, rsSetup);

		d3d12Context_.Compile(
			VS_LowPoly_,
			L"Assets/Terrain/Shaders/LowPoly.VS.hlsl",
			L"vs_6_6",
			L"main",
			"LowPoly.VS"
		);
		d3d12Context_.Compile(
			PS_LowPoly_,
			L"Assets/Terrain/Shaders/LowPoly.PS.hlsl",
			L"ps_6_6",
			L"main",
			"LowPoly.PS"
		); 
		
		Lumina::D3D12::BlendState blendState_None{};
		// * Albedo
		blendState_None.RenderTarget[0].BlendEnable = false;
		blendState_None.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		// * Normal
		blendState_None.RenderTarget[1].BlendEnable = false;
		blendState_None.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		Lumina::D3D12::GraphicsPSO::InputLayout inputLayout{};
		inputLayout.Append("IDX_POSITION", 0U, DXGI_FORMAT_R32_UINT);
		inputLayout.Append("IDX_UV", 0U, DXGI_FORMAT_R32_UINT);
		inputLayout.Append("IDX_NORMAL", 0U, DXGI_FORMAT_R32_UINT);
		inputLayout.Append("IDX_TANGENT", 0U, DXGI_FORMAT_R32_UINT);

		PSO_LowPoly_.Initialize(
			d3d12Device_,
			RS_LowPoly_,
			VS_LowPoly_,
			PS_LowPoly_,
			blendState_None,
			Lumina::D3D12::RasterizerState{
				.FillMode{ D3D12_FILL_MODE_SOLID },
				.CullMode{ D3D12_CULL_MODE_NONE },
			},
			Lumina::D3D12::DepthStencilState{
				.DepthEnable{ true },
				.DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ALL },
				.DepthFunc{ D3D12_COMPARISON_FUNC_LESS_EQUAL },
				.StencilEnable{ false },
			},
			inputLayout,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			{
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				DXGI_FORMAT_R8G8B8A8_UNORM,
			},
			Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
		);
	}

	template<>
	auto TerrainRenderer::Initialize<"LowPoly.Resources">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		UB_WorldToProjective_.Initialize(
			d3d12Device_,
			256LLU
		);
		GlobalTable_CBV_Transforms_ = d3d12Context_.GlobalDescriptorHeap().Allocate(1U);
		Lumina::D3D12::CBV::Create(
			d3d12Device_,
			GlobalTable_CBV_Transforms_.CPUHandle(0U),
			UB_WorldToProjective_
		);

		// * Non-ground, Ground
		GlobalTable_SRV_VertexElementArray_ = d3d12Context_.GlobalDescriptorHeap().Allocate(4U * 2U);
	}

	template<>
	auto TerrainRenderer::Initialize<"LowPoly.PixelShaderResources">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		CT_HeightMap_LowPoly_.Initialize(
			d3d12Device_,
			1280U, 720U,
			DXGI_FORMAT_R16_FLOAT
		);

		GlobalTable_UAV_SRV_HeightMap_LowPoly_ = d3d12Context_.GlobalDescriptorHeap().Allocate(2U);
		Lumina::D3D12::UAV<void>::Create(
			d3d12Device_,
			GlobalTable_UAV_SRV_HeightMap_LowPoly_.CPUHandle(0U),
			CT_HeightMap_LowPoly_
		);
		Lumina::D3D12::SRV<void>::Create(
			d3d12Device_,
			GlobalTable_UAV_SRV_HeightMap_LowPoly_.CPUHandle(1U),
			CT_HeightMap_LowPoly_
		);

		UB_PSParameters_.Initialize(
			d3d12Device_,
			256LLU
		);
		GlobalTable_CBV_PSParameters_ = d3d12Context_.GlobalDescriptorHeap().Allocate(1U);
		Lumina::D3D12::CBV::Create(
			d3d12Device_,
			GlobalTable_CBV_PSParameters_.CPUHandle(0U),
			UB_PSParameters_
		);

		PSParameters_.Scale_SurfaceBlendUV = { 0.5f, 0.5f };
		PSParameters_.Scale_SurfaceNormal = 1.0f;
		PSParameters_.Scale_MaterialNormal = 0.25f;
		UB_PSParameters_.Store(&PSParameters_, sizeof(PSParameters), 0LLU);
	}

	template<>
	auto TerrainRenderer::Initialize<"LowPoly">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_,
		nlohmann::json const& settings_
	) -> void {
		Initialize<"LowPoly.Pipeline">(d3d12Context_, d3d12Device_, settings_);
		Initialize<"LowPoly.Canvas">(d3d12Context_, d3d12Device_);
		Initialize<"LowPoly.Resources">(d3d12Context_, d3d12Device_);
		Initialize<"LowPoly.PixelShaderResources">(d3d12Context_, d3d12Device_);
		Initialize<"LowPoly.RenderPass">();
	}

	template<>
	auto TerrainRenderer::Initialize<"Surface.ParameterBuffers">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_,
		Lumina::D3D12::UploadBuffer& ub_Parameter_Common_,
		Lumina::D3D12::UploadBuffer& ub_Parameter_Material_,
		Lumina::D3D12::UploadBuffer& ub_Parameter_Blend_,
		Lumina::D3D12::UploadBuffer& ub_Parameter_Height_,
		Lumina::D3D12::DescriptorTable& globalTable_
	) -> void {
		ub_Parameter_Common_.Initialize(d3d12Device_, 256LLU);
		ub_Parameter_Material_.Initialize(d3d12Device_, 256LLU);
		ub_Parameter_Blend_.Initialize(d3d12Device_, 256LLU);
		ub_Parameter_Height_.Initialize(d3d12Device_, 256LLU);

		globalTable_ = d3d12Context_.GlobalDescriptorHeap().Allocate(4U + 3U + 3U);
		Lumina::D3D12::CBV::Create(
			d3d12Device_,
			globalTable_.CPUHandle(0U),
			ub_Parameter_Common_
		);
		Lumina::D3D12::CBV::Create(
			d3d12Device_,
			globalTable_.CPUHandle(1U),
			ub_Parameter_Material_
		);
		Lumina::D3D12::CBV::Create(
			d3d12Device_,
			globalTable_.CPUHandle(2U),
			ub_Parameter_Blend_
		);
		Lumina::D3D12::CBV::Create(
			d3d12Device_,
			globalTable_.CPUHandle(3U),
			ub_Parameter_Height_
		);

		struct Parameter_Common {
			Lumina::U32 MapSize[2];
			Lumina::F32x2 TexelSize;
			Lumina::F32 INV_MAX;
		} param_Common{};

		Lumina::F32 const persistence{ 0.75f };
		param_Common.INV_MAX = 1.0f / (1.0f + (persistence * (1.0f + persistence * (1.0f + persistence))));
		param_Common.TexelSize = { 1.0f / 512.0f, 1.0f / 384.0f };
		param_Common.MapSize[0] = 512U;
		param_Common.MapSize[1] = 384U;
		ub_Parameter_Common_.Store(&param_Common, sizeof(Parameter_Common), 0LLU);

		struct Parameter_Material {
			Lumina::ProceduralMap::Parameter NoiseArguments;
			Lumina::U32 NUM_Materials;
		} param_Material{};
		param_Material.NoiseArguments = { { 1.0f, 2.0f, 3.0f }, 6.0f, persistence, 4U };
		param_Material.NUM_Materials = 8U;
		ub_Parameter_Material_.Store(&param_Material, sizeof(Parameter_Material), 0LLU);

		struct Parameter_Blend {
			Lumina::ProceduralMap::Parameter NoiseArguments;
		} param_Blend{};
		param_Blend.NoiseArguments = { { 5.5f, 5.6f, 5.7f }, 5.0f, persistence, 4U };
		ub_Parameter_Blend_.Store(&param_Blend, sizeof(Parameter_Blend), 0LLU);

		struct Parameter_Height {
			Lumina::ProceduralMap::Parameter NoiseArguments;
		} param_Height{};
		param_Height.NoiseArguments = { { 4.8f, 3.6f, 2.4f }, 7.0f, persistence, 4U };
		ub_Parameter_Height_.Store(&param_Height, sizeof(Parameter_Height), 0LLU);

		d3d12Device_->CopyDescriptorsSimple(
			1U,
			globalTable_.CPUHandle(4U),
			Surface_MaterialID_.LocalUAV(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
		d3d12Device_->CopyDescriptorsSimple(
			1U,
			globalTable_.CPUHandle(5U),
			Surface_BlendAndElevation_.LocalUAV(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
		d3d12Device_->CopyDescriptorsSimple(
			1U,
			globalTable_.CPUHandle(6U),
			Surface_Normal_.LocalUAV(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		d3d12Device_->CopyDescriptorsSimple(
			1U,
			globalTable_.CPUHandle(7U),
			Surface_MaterialID_.LocalSRV(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
		d3d12Device_->CopyDescriptorsSimple(
			1U,
			globalTable_.CPUHandle(8U),
			Surface_BlendAndElevation_.LocalSRV(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
		d3d12Device_->CopyDescriptorsSimple(
			1U,
			globalTable_.CPUHandle(9U),
			Surface_Normal_.LocalSRV(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
	}

	template<>
	auto TerrainRenderer::Initialize<"Surface.Pipeline">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_,
		Lumina::D3D12::RootSignature& rs_,
		Lumina::D3D12::ComputePSO& pso_,
		Lumina::D3D12::Shader& cs_,
		nlohmann::json const& settings_
	) -> void {
		auto rsSetup{
			 Lumina::D3D12::LoadSetup<Lumina::D3D12::RootSignature>(
				 settings_.at("Surface RS")
			 )
		};
		rs_.Initialize(d3d12Device_, rsSetup);

		d3d12Context_.Compile(
			cs_,
			L"Assets/Terrain/Shaders/Surface.CS.hlsl",
			L"cs_6_6",
			L"main",
			"Surface.CS"
		);

		pso_.Initialize(
			d3d12Device_,
			rs_,
			cs_
		);

		d3d12Context_.Compile(
			CS_Surface2_,
			L"Assets/Terrain/Shaders/Surface.CS.hlsl",
			L"cs_6_6",
			L"main2",
			"Surface2.CS"
		);

		PSO_Surface2_.Initialize(
			d3d12Device_,
			rs_,
			CS_Surface2_
		);
	}

	template<>
	auto TerrainRenderer::Initialize<"Surface.Dispatch">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_,
		Lumina::D3D12::CommandQueue& cmdQueue_
	) -> void {
		Lumina::D3D12::CommandAllocator cmdAllocator{};
		Lumina::D3D12::CommandList cmdList{};
		cmdAllocator.Initialize(d3d12Device_);
		cmdList.Initialize(d3d12Device_, cmdAllocator);

		ID3D12DescriptorHeap* descriptorHeaps[]{
			d3d12Context_.GlobalDescriptorHeap().Get(),
		};
		cmdList->SetDescriptorHeaps(1U, descriptorHeaps);

		cmdList->SetComputeRootSignature(RS_Surface_.Get());
		cmdList->SetComputeRootDescriptorTable(0U, GlobalTable_Surface_.GPUHandle(4U));
		cmdList->SetComputeRootDescriptorTable(1U, GlobalTable_Surface_.GPUHandle(0U));
		cmdList->SetPipelineState(PSO_Surface_.Get());
		cmdList->Dispatch(512U, 384U, 1U);
		cmdList->SetPipelineState(PSO_Surface2_.Get());
		cmdList->Dispatch(512U, 384U, 1U);

		D3D12_RESOURCE_BARRIER const barriers[]{
			 Lumina::D3D12::Barrier::Transition(
				 Surface_MaterialID_.Texture(),
				 D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Surface_BlendAndElevation_.Texture(),
				 D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			 ),
			 Lumina::D3D12::Barrier::Transition(
				 Surface_Normal_.Texture(),
				 D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			 ),
		};
		cmdList->ResourceBarrier(3U, barriers);

		cmdQueue_ << cmdList;
		cmdQueue_.CPUWait(cmdQueue_.ExecuteBatchedCommandLists());
	}

	template<>
	auto TerrainRenderer::Initialize<"Surface">(
		Lumina::D3D12::Context const& d3d12Context_,
		nlohmann::json const& settings_
	) -> void {
		Surface_MaterialID_.Initialize(d3d12Context_, 512U, 384U, DXGI_FORMAT_R8_UINT);
		Surface_BlendAndElevation_.Initialize(d3d12Context_, 512U, 384U, DXGI_FORMAT_R16G16B16A16_FLOAT);
		Surface_Normal_.Initialize(d3d12Context_, 512U, 384U, DXGI_FORMAT_R8G8B8A8_UNORM);

		Initialize<"Surface.ParameterBuffers">(
			d3d12Context_,
			d3d12Context_.Device(),
			UB_Parameter_Common_,
			UB_Parameter_Material_,
			UB_Parameter_Blend_,
			UB_Parameter_Height_,
			GlobalTable_Surface_
		);

		Initialize<"Surface.Pipeline">(
			d3d12Context_,
			d3d12Context_.Device(),
			RS_Surface_,
			PSO_Surface_,
			CS_Surface_,
			settings_
		);

		Initialize<"Surface.Dispatch">(
			d3d12Context_,
			d3d12Context_.Device(),
			d3d12Context_.DirectQueue()
		);
	}

	template<>
	auto TerrainRenderer::Initialize<"">() -> void {
	}

	auto TerrainRenderer::Initialize() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		auto const settings{
			Lumina::Utils::LoadFromFile<nlohmann::json>(
				"Assets/Terrain/Settings.json"
			)
		};
		
		Initialize<"Surface">(d3d12Context, settings);

		Initialize<"Material">(d3d12Context, d3d12Device);
		Initialize<"LowPoly">(d3d12Context, d3d12Device, settings);

		//Initialize<"MeshManager">(d3d12Context);
	}
}

namespace Game {
	auto TerrainRenderer::PrepareMesh(
		TerrainShapeCollection const& shapeCollection_
	) -> void { Impl_->PrepareMesh(shapeCollection_); }

	auto TerrainRenderer::DebugRenderCollidersBatch(
		TerrainShapeCollection const& shapeCollection_
	) -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };

		PrimitiveManager_->Begin(cmdList);

		auto const& polygons{ shapeCollection_.PolygonsData() };
		for (auto const& polygon : polygons) {
			if (polygon.Vertices.size() > 2) {
				auto const* verts{ polygon.Vertices.data() };
				for (size_t i = 2; i < polygon.Vertices.size(); ++i) {
					PrimitiveManager_->BatchTriangle(
						{ { verts[0].Pos.X, verts[0].Pos.Y, verts[0].Pos.Z, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
						{ { verts[i - 1].Pos.X, verts[i - 1].Pos.Y, verts[i - 1].Pos.Z, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
						{ { verts[i].Pos.X, verts[i].Pos.Y, verts[i].Pos.Z, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U }
					);
				}
			}
		}

		auto const& ground{ shapeCollection_.GroundData() };
		for (auto const& collider : ground.Colliders) {
			auto const& verts{ collider->GetVertices() };
			PrimitiveManager_->BatchTriangle(
				{ { verts[0].X, verts[0].Y, verts[0].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
				{ { verts[1].X, verts[1].Y, verts[1].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
				{ { verts[3].X, verts[3].Y, verts[3].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U }
			);
			PrimitiveManager_->BatchTriangle(
				{ { verts[1].X, verts[1].Y, verts[1].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
				{ { verts[2].X, verts[2].Y, verts[2].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U },
				{ { verts[3].X, verts[3].Y, verts[3].Z - 0.05f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }, 0U }
			);
		}

		PrimitiveManager_->End(cmdList);
	}

	auto TerrainRenderer::DebugRenderColliders(
		Lumina::D3D12::DescriptorTable const& srvTable_,
		Lumina::Math::F32x4x4<> const& vp_
	) -> void {
		auto const& cmdList{ Lumina::Context::Instance().MainCommandList() };
		PrimitiveManager_->Render(cmdList, srvTable_, vp_);
	}

	template<>
	auto TerrainRenderer::Render(
		Lumina::D3D12::Canvas const& canvas_,
		Lumina::Math::F32x4x4<> const& worldToProjective_
	) -> void {
		Impl_->Render(canvas_, worldToProjective_);
	}

	auto TerrainRenderer::Initialize() -> void {
		auto const& d3d12Context{ Lumina::Context::Instance().D3D12Context() };
		PrimitiveManager_ = std::make_unique<Lumina::PrimitiveManager>();
		PrimitiveManager_->Initialize(
			d3d12Context,
			L"Assets/Shaders/Terrain.Debug.VS.hlsl",
			L"Assets/Shaders/Terrain.Debug.PS.hlsl"
		);

		Impl_ = std::make_unique<Impl::TerrainRenderer>();
		Impl_->Initialize();
	}

	TerrainRenderer::TerrainRenderer() {}
	TerrainRenderer::~TerrainRenderer() {}
}