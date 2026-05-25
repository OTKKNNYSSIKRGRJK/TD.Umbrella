module Lumina.Grassland;

import <memory>;

import <vector>;

import Lumina.D3D12;
import Lumina.D3D12.Context;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import Lumina.Main;

import Lumina.Utils.Data;
import Lumina.Utils.Data.Mesh;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import Lumina.Core.Common;
import Lumina.Core.Math;

namespace Lumina {
	void Grassland::Initialize(
		D3D12::Context const& dxContext_,
		U32 mapWidth_,
		U32 mapHeight_
	) {
		auto const& device{ dxContext_.Device() };
		auto const& resMngr{ Context::Instance().ResourceContext() };

		MapWidth_ = mapWidth_;
		MapHeight_ = mapHeight_;

		Num_Maps_ = 5U;

		Arr_MapResourceFormats_.resize(Num_Maps_);
		Arr_MapResourceFormats_[static_cast<U32>(MAP::BEND)] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		Arr_MapResourceFormats_[static_cast<U32>(MAP::TRAMPLING)] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		Arr_MapResourceFormats_[static_cast<U32>(MAP::NOISE)] = DXGI_FORMAT_R8G8B8A8_UNORM;
		Arr_MapResourceFormats_[static_cast<U32>(MAP::TINT)] = DXGI_FORMAT_R8G8B8A8_UNORM;
		Arr_MapResourceFormats_[4U] = DXGI_FORMAT_R8G8B8A8_UNORM;

		Arr_Maps_.resize(Num_Maps_);
		for (U32 idx{ 0U }; idx < static_cast<U32>(Arr_Maps_.size()); ++idx) {
			Arr_Maps_[idx].reset(new D3D12::ComputeTexture2D{});
			Arr_Maps_[idx]->Initialize(
				device,
				MapWidth_,
				MapHeight_,
				Arr_MapResourceFormats_[idx],
				std::format("Map #{}", idx).data()
			);
		}

		LocalHeap_SRV_Maps_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, Num_Maps_, false);
		for (U32 idx{ 0U }; idx < static_cast<U32>(Arr_Maps_.size()); ++idx) {
			D3D12::SRV<void>::Create(device, LocalHeap_SRV_Maps_.CPUHandle(idx), *Arr_Maps_[idx]);
		}

		LocalHeap_UAV_Maps_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, Num_Maps_, false);
		for (U32 idx{ 0U }; idx < static_cast<U32>(Arr_Maps_.size()); ++idx) {
			D3D12::UAV<void>::Create(device, LocalHeap_UAV_Maps_.CPUHandle(idx), *Arr_Maps_[idx]);
		}

		GlobalTable_Graphics_ = dxContext_.GlobalDescriptorHeap().Allocate(192U);
		for (U32 idx{ 0U }; idx < static_cast<U32>(Arr_Maps_.size()); ++idx) {
			device->CopyDescriptorsSimple(
				1U,
				GlobalTable_Graphics_.CPUHandle(idx + 32U),
				LocalHeap_SRV_Maps_.CPUHandle(idx),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
			device->CopyDescriptorsSimple(
				1U,
				GlobalTable_Graphics_.CPUHandle(idx + 32U + 96U),
				LocalHeap_SRV_Maps_.CPUHandle(idx),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		GlobalTable_Compute_ = dxContext_.GlobalDescriptorHeap().Allocate(96U);
		for (U32 idx{ 0U }; idx < static_cast<U32>(Arr_Maps_.size()); ++idx) {
			device->CopyDescriptorsSimple(
				1U,
				GlobalTable_Compute_.CPUHandle(idx + 32U),
				LocalHeap_SRV_Maps_.CPUHandle(idx),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
			device->CopyDescriptorsSimple(
				1U,
				GlobalTable_Compute_.CPUHandle(idx + 64U),
				LocalHeap_UAV_Maps_.CPUHandle(idx),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		DirectCommandAllocator0_.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		DirectCommandList0_.Initialize(device, DirectCommandAllocator0_);
		DirectCommandAllocator1_.Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		DirectCommandList1_.Initialize(device, DirectCommandAllocator1_);
		ComputeCommandAllocator_.Initialize(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		ComputeCommandList_.Initialize(device, ComputeCommandAllocator_);

		auto settings{ Utils::LoadFromFile<nlohmann::json>("Settings.json", "Assets/Grassland") };
		auto graphicsRSSetup{ D3D12::LoadSetup<D3D12::RootSignature>(settings.at("General Graphics RS Test")) };
		GraphicsRS_.Initialize(device, graphicsRSSetup, "Graphics RS");
		auto computeRSSetup{ D3D12::LoadSetup<D3D12::RootSignature>(settings.at("General Compute RS Test")) };
		ComputeRS_.Initialize(device, computeRSSetup, "Compute RS");

		dxContext_.Compile(
			VertexShader_,
			L"Assets/Grassland/VS.hlsl",
			L"vs_6_6",
			L"main",
			"Grassland::VS"
		);
		dxContext_.Compile(
			PixelShader_,
			L"Assets/Grassland/PS.hlsl",
			L"ps_6_6",
			L"main",
			"Grassland::PS"
		);
		D3D12::GraphicsPipelineState::Setup graphicsPSOSetup{};
		D3D12::BlendState blendState{ .IndependentBlendEnable{ true }, };
		blendState.RenderTarget[0] = {
			.BlendEnable{ true },
			.LogicOpEnable{ false },
			.SrcBlend{ D3D12_BLEND_SRC_ALPHA },
			.DestBlend{ D3D12_BLEND_INV_SRC_ALPHA },
			.BlendOp{ D3D12_BLEND_OP_ADD },
			.SrcBlendAlpha{ D3D12_BLEND_ONE },
			.DestBlendAlpha{ D3D12_BLEND_ONE },
			.BlendOpAlpha{ D3D12_BLEND_OP_ADD },
			.RenderTargetWriteMask{ D3D12_COLOR_WRITE_ENABLE_ALL },
		};
		blendState.RenderTarget[1] = {
			.BlendEnable{ false },
			.LogicOpEnable{ false },
			.RenderTargetWriteMask{ D3D12_COLOR_WRITE_ENABLE_ALL },
		};
		D3D12::RasterizerState rasterizerState{
			.FillMode{ D3D12_FILL_MODE_SOLID },
			.CullMode{ D3D12_CULL_MODE_NONE },
		};
		D3D12::DepthStencilState depthStencilState{
			.DepthEnable{ true },
			.DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ALL },
			.DepthFunc{ D3D12_COMPARISON_FUNC_LESS_EQUAL },
		};
		D3D12::GraphicsPipelineState::InputLayout inputLayout{};
		inputLayout.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout.Append("TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT);
		inputLayout.Append("NORMAL", 0U, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout.Append("TANGENT", 0U, DXGI_FORMAT_R32G32B32_FLOAT);
		std::vector<DXGI_FORMAT> rtvFormats{
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM,
		};
		graphicsPSOSetup <<
			GraphicsRS_ <<
			VertexShader_ <<
			PixelShader_ <<
			blendState <<
			rasterizerState <<
			depthStencilState <<
			inputLayout <<
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE <<
			rtvFormats <<
			D3D12::GraphicsPipelineState::DefaultDSVFormat;
		GraphicsPSO_.Initialize(
			device,
			graphicsPSOSetup,
			"Grassland::GraphicsPSO::GrassBlade"
		);

		Num_ComputeShaders_ = 5U;
		Arr_ComputeShaders_.resize(Num_ComputeShaders_);
		for (auto& cs : Arr_ComputeShaders_) {
			cs.reset(new D3D12::Shader{});
		}
		dxContext_.Compile(
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::BEND)],
			L"Assets/Grassland/Maps.CS.hlsl",
			L"cs_6_6",
			L"GenerateBendMap",
			"Grassland::CS::Bend"
		);
		dxContext_.Compile(
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::TRAMPLING)],
			L"Assets/Grassland/Maps.CS.hlsl",
			L"cs_6_6",
			L"GenerateTramplingMap",
			"Grassland::CS::Trampling"
		);
		dxContext_.Compile(
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::NOISE)],
			L"Assets/Grassland/Maps.CS.hlsl",
			L"cs_6_6",
			L"GenerateNoiseMap",
			"Grassland::CS::Noise"
		);
		dxContext_.Compile(
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::TINT)],
			L"Assets/Grassland/Maps.CS.hlsl",
			L"cs_6_6",
			L"GenerateTintMap",
			"Grassland::CS::Tint"
		);
		dxContext_.Compile(
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::INIT)],
			L"Assets/Grassland/Maps.CS.hlsl",
			L"cs_6_6",
			L"InitMaps",
			"Grassland::CS::Init"
		);

		Num_ComputePSOs_ = 5U;
		Arr_ComputePSOs_.resize(Num_ComputePSOs_);
		for (auto& pso : Arr_ComputePSOs_) {
			pso.reset(new D3D12::ComputePipelineState{});
		}
		Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::BEND)]->Initialize(
			device,
			ComputeRS_,
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::BEND)],
			"Grassland::ComputePSO::Bend"
		);
		Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::TRAMPLING)]->Initialize(
			device,
			ComputeRS_,
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::TRAMPLING)],
			"Grassland::ComputePSO::Trampling"
		);
		Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::NOISE)]->Initialize(
			device,
			ComputeRS_,
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::NOISE)],
			"Grassland::ComputePSO::Noise"
		);
		Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::TINT)]->Initialize(
			device,
			ComputeRS_,
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::TINT)],
			"Grassland::ComputePSO::Tint"
		);
		Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::INIT)]->Initialize(
			device,
			ComputeRS_,
			*Arr_ComputeShaders_[static_cast<U32>(COMPUTE_SHADER::INIT)],
			"Grassland::ComputePSO::Init"
		);

		ID3D12DescriptorHeap* descriptorHeaps[]{ dxContext_.GlobalDescriptorHeap().Get(), };
		ComputeCommandList_->SetDescriptorHeaps(1U, descriptorHeaps);
		ComputeCommandList_->SetComputeRootSignature(ComputeRS_.Get());
		ComputeCommandList_->SetComputeRootDescriptorTable(0U, GlobalTable_Compute_.GPUHandle(0U));
		ComputeCommandList_->SetPipelineState(
			Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::INIT)]->Get()
		);
		ComputeCommandList_->Dispatch(MapWidth_, MapHeight_, 1U);
		ComputeCommandList_->SetPipelineState(
			Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::NOISE)]->Get()
		);
		ComputeCommandList_->Dispatch(MapWidth_, MapHeight_, 1U);
		ComputeCommandList_->SetPipelineState(
			Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::TINT)]->Get()
		);
		ComputeCommandList_->Dispatch(MapWidth_, MapHeight_, 1U);
		dxContext_.ComputeQueue() << ComputeCommandList_;
		dxContext_.ComputeQueue().CPUWait(dxContext_.ComputeQueue().ExecuteBatchedCommandLists());
		ComputeCommandList_.Reset(ComputeCommandAllocator_);


		auto obj_GrassBlade{ Utils::LoadFromFile<Utils::WavefrontOBJ>("GrassBlade.obj", "Assets/Grassland") };
		auto&& meshes_GrassBlade{ Utils::Mesh::Load(obj_GrassBlade) };
		auto const& mesh_GrassBlade{ meshes_GrassBlade[0] };

		struct Vertex {
			Math::F32x3 LocalPos;
			Math::F32x2 TexCoord;
			Math::F32x3 Normal;
			Math::F32x3 Tangent;
		};
		std::vector<Vertex> vertices_GrassBlade{};
		for (auto const& vert : mesh_GrassBlade.Vertices) {
			auto& vertData{ vertices_GrassBlade.emplace_back() };
			vertData.LocalPos = mesh_GrassBlade.Positions[vert.Index_Position];
			vertData.TexCoord = mesh_GrassBlade.TexCoords[vert.Index_TexCoord];
			vertData.Normal = mesh_GrassBlade.Normals[vert.Index_Normal];
			vertData.Tangent = mesh_GrassBlade.Tangents[vert.Index_Tangent];
		}
		Num_Vertices_GrassBlade_ = static_cast<U32>(vertices_GrassBlade.size());

		DB_Mesh_GrassBlade_.Initialize(
			device,
			sizeof(Vertex) * Num_Vertices_GrassBlade_,
			"Grassland::DB::Mesh::GrassBlade"
		);
		VBV_Mesh_GrassBlade_ = D3D12::VBV::Create<Vertex>(DB_Mesh_GrassBlade_);
		D3D12::UploadBuffer ub_Mesh_GrassBlade{};
		ub_Mesh_GrassBlade.Initialize(device, DB_Mesh_GrassBlade_.SizeInBytes());
		ub_Mesh_GrassBlade.Store(
			vertices_GrassBlade.data(),
			sizeof(Vertex) * Num_Vertices_GrassBlade_,
			0LLU
		);
		DirectCommandList0_->CopyResource(DB_Mesh_GrassBlade_.Get(), ub_Mesh_GrassBlade.Get());

		DB_Constant_System_.Initialize(device, (sizeof(Constant_Scene) + 0xFFLLU) & ~0xFFLLU);
		UB_Constant_System_.Initialize(device, DB_Constant_System_.SizeInBytes());

		DB_Constant_Scene_.Initialize(device, (sizeof(Constant_Scene) + 0xFFLLU) & ~0xFFLLU);
		UB_Constant_Scene_.Initialize(device, DB_Constant_Scene_.SizeInBytes());
		UB_Constant_Scene_.Store(&Constant_Scene_, sizeof(Constant_Scene), 0LLU);
		DirectCommandList0_->CopyResource(DB_Constant_Scene_.Get(), UB_Constant_Scene_.Get());

		LocalHeap_CBV_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32U, false);
		D3D12::CBV::Create(device, LocalHeap_CBV_.CPUHandle(0U), DB_Constant_System_);
		D3D12::CBV::Create(device, LocalHeap_CBV_.CPUHandle(1U), DB_Constant_Scene_);
		for (U32 idx{ 0U }; idx < 2U; ++idx) {
			device->CopyDescriptorsSimple(
				1U,
				GlobalTable_Graphics_.CPUHandle(idx),
				LocalHeap_CBV_.CPUHandle(idx),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
			device->CopyDescriptorsSimple(
				1U,
				GlobalTable_Graphics_.CPUHandle(idx + 96U),
				LocalHeap_CBV_.CPUHandle(idx),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
			device->CopyDescriptorsSimple(
				1U,
				GlobalTable_Compute_.CPUHandle(idx),
				LocalHeap_CBV_.CPUHandle(idx),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		std::vector<D3D12_RESOURCE_BARRIER> const barriers{
			D3D12::Barrier::Transition(
				*Arr_Maps_[static_cast<U32>(MAP::BEND)],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				*Arr_Maps_[static_cast<U32>(MAP::TRAMPLING)],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				*Arr_Maps_[static_cast<U32>(MAP::NOISE)],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				*Arr_Maps_[static_cast<U32>(MAP::TINT)],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				DB_Mesh_GrassBlade_,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			),
			D3D12::Barrier::Transition(
				DB_Constant_System_,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				DB_Constant_Scene_,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
		};
		DirectCommandList0_->ResourceBarrier(static_cast<U32>(barriers.size()), barriers.data());
		dxContext_.DirectQueue() << DirectCommandList0_;
		dxContext_.DirectQueue().CPUWait(dxContext_.DirectQueue().ExecuteBatchedCommandLists());
		DirectCommandList0_.Reset(DirectCommandAllocator0_);

		std::vector<U32> texIDs{};
		resMngr.Graphics().LoadImageTextures(
			texIDs,
			{
				{ "GrassBlade", "Assets/Grassland/GrassBlade.png" },
			}
			);

		GlobalTable_ImageTextures_ = dxContext_.GlobalDescriptorHeap().Allocate(32U);
		for (U32 idx{ 0U }; idx < static_cast<U32>(texIDs.size()); ++idx) {
			device->CopyDescriptorsSimple(
				1U,
				GlobalTable_ImageTextures_.CPUHandle(idx),
				resMngr.Graphics().CPUHandle(texIDs.at(idx)),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}
	}


	void Grassland::Render(
		D3D12::Context const& dxContext_,
		D3D12::CommandList const& cmdList_,
		Math::F32x4x4<> const& worldToProj_,
		Math::F32x3 const& playerWorldPos_,
		F32 playerRadius_,
		Math::F32x3 const& enemyWorldPos_,
		F32 enemyRadius_
	) {
		static std::vector<D3D12_RESOURCE_BARRIER> const barriers0{
			D3D12::Barrier::Transition(
				*Arr_Maps_[static_cast<U32>(MAP::BEND)],
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			),
			D3D12::Barrier::Transition(
				*Arr_Maps_[static_cast<U32>(MAP::TRAMPLING)],
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			),
		};

		static std::vector<D3D12_RESOURCE_BARRIER> const barriers1{
			D3D12::Barrier::Transition(
				DB_Constant_System_,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_DEST
			),
			D3D12::Barrier::Transition(
				DB_Constant_Scene_,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_DEST
			),
		};

		static std::vector<D3D12_RESOURCE_BARRIER> const barriers2{
			D3D12::Barrier::Transition(
				DB_Constant_System_,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				DB_Constant_Scene_,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				*Arr_Maps_[static_cast<U32>(MAP::BEND)],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			),
			D3D12::Barrier::Transition(
				*Arr_Maps_[static_cast<U32>(MAP::TRAMPLING)],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			),
		};

		static D3D12_RESOURCE_BARRIER const uavBarrier_Map_Trampling{
			.Type{ D3D12_RESOURCE_BARRIER_TYPE_UAV },
			.UAV{
				.pResource{
					Arr_Maps_[static_cast<U32>(MAP::TRAMPLING)]->Get()
				},
			},
		};

		DirectCommandList0_->ResourceBarrier(static_cast<U32>(barriers0.size()), barriers0.data());
		dxContext_.DirectQueue() << DirectCommandList0_;
		dxContext_.ComputeQueue().GPUWait(
			dxContext_.DirectQueue(),
			dxContext_.DirectQueue().ExecuteBatchedCommandLists()
		);

		ID3D12DescriptorHeap* descriptorHeaps[]{ dxContext_.GlobalDescriptorHeap().Get(), };
		ComputeCommandList_->SetDescriptorHeaps(1U, descriptorHeaps);
		ComputeCommandList_->SetComputeRootSignature(ComputeRS_.Get());
		ComputeCommandList_->SetComputeRootDescriptorTable(0U, GlobalTable_Compute_.GPUHandle(0U));
		ComputeCommandList_->SetPipelineState(
			Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::TRAMPLING)]->Get()
		);
		ComputeCommandList_->Dispatch(MapWidth_ >> 1U, MapHeight_ >> 1U, 1U);
		ComputeCommandList_->ResourceBarrier(1U, &uavBarrier_Map_Trampling);
		ComputeCommandList_->SetPipelineState(
			Arr_ComputePSOs_[static_cast<U32>(COMPUTE_SHADER::BEND)]->Get()
		);
		ComputeCommandList_->Dispatch(MapWidth_, MapHeight_, 1U);
		dxContext_.ComputeQueue() << ComputeCommandList_;
		dxContext_.DirectQueue().GPUWait(
			dxContext_.ComputeQueue(),
			dxContext_.ComputeQueue().ExecuteBatchedCommandLists()
		);

		Constant_System_.Time += 1.0f;
		UB_Constant_System_.Store(&Constant_System_, sizeof(Constant_System), 0LLU);

		Constant_Scene_.WorldToProjective = worldToProj_;
		std::memcpy(&Constant_Scene_.PlayerWorldPos, playerWorldPos_(), sizeof(Math::F32x3));
		Constant_Scene_.PlayerRadius = playerRadius_;
		std::memcpy(&Constant_Scene_.EnemyWorldPos, enemyWorldPos_(), sizeof(Math::F32x3));
		Constant_Scene_.EnemyRadius = enemyRadius_;
		UB_Constant_Scene_.Store(&Constant_Scene_, sizeof(Constant_Scene), 0LLU);

		DirectCommandList1_->ResourceBarrier(static_cast<U32>(barriers1.size()), barriers1.data());

		DirectCommandList1_->CopyResource(DB_Constant_System_.Get(), UB_Constant_System_.Get());
		DirectCommandList1_->CopyResource(DB_Constant_Scene_.Get(), UB_Constant_Scene_.Get());

		DirectCommandList1_->ResourceBarrier(static_cast<U32>(barriers2.size()), barriers2.data());

		dxContext_.DirectQueue() << DirectCommandList1_;
		dxContext_.DirectQueue().CPUWait(dxContext_.DirectQueue().ExecuteBatchedCommandLists());

		DirectCommandList0_.Reset(DirectCommandAllocator0_);
		DirectCommandList1_.Reset(DirectCommandAllocator1_);
		ComputeCommandList_.Reset(ComputeCommandAllocator_);

		/*ImGui::Begin("Grassland");
		ImGui::Image(
			GlobalTable_Compute_.GPUHandle(static_cast<U32>(MAP::BEND) + 32U).ptr,
			{ static_cast<F32>(MapWidth_), static_cast<F32>(MapHeight_) }
		);
		ImGui::Image(
			GlobalTable_Compute_.GPUHandle(static_cast<U32>(MAP::TRAMPLING) + 32U).ptr,
			{ static_cast<F32>(MapWidth_), static_cast<F32>(MapHeight_) }
		);
		ImGui::End();*/

		/*auto rtv{ dxContext_.SwapChain().BackBufferRTVCPUHandle() };
		auto dsv{ dxContext_.SwapChain().DSVCPUHandle() };
		static D3D12_VIEWPORT const viewport{ 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f, };
		static D3D12_RECT const scissorRect{ 0U, 0U, 1280U, 720U };

		cmdList_->OMSetRenderTargets(1U, &rtv, false, &dsv);
		cmdList_->RSSetViewports(1U, &viewport);
		cmdList_->RSSetScissorRects(1U, &scissorRect);*/

		//ID3D12DescriptorHeap* descriptorHeaps[]{ dxContext_.GlobalDescriptorHeap().Get(), };
		//DirectCommandList0_->SetDescriptorHeaps(1U, descriptorHeaps);

		cmdList_->SetGraphicsRootSignature(GraphicsRS_.Get());
		cmdList_->SetGraphicsRootDescriptorTable(0U, GlobalTable_Graphics_.GPUHandle(0U));
		cmdList_->SetGraphicsRootDescriptorTable(1U, GlobalTable_Graphics_.GPUHandle(96U));
		cmdList_->SetGraphicsRootDescriptorTable(2U, GlobalTable_ImageTextures_.GPUHandle(0U));
		cmdList_->SetPipelineState(GraphicsPSO_.Get());
		cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList_->IASetVertexBuffers(0U, 1U, &VBV_Mesh_GrassBlade_);
		cmdList_->DrawInstanced(Num_Vertices_GrassBlade_, 512U * 512U, 0U, 0U);
	}
}