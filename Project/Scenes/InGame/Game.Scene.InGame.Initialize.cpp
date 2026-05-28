module Game.Scene.InGame : Impl;

import <vector>;
import <filesystem>;
import <random>;
import <fstream>;

import nlohmann.json;

import Game.TutorialManager;

//import Lumina;

import Lumina.Utils.Data;
import Lumina.Main;
import Lumina.D3D12;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;

import Game.MotionManager;
import Game.Player;

import Lumina.CG3D;
import Lumina.CG3D.Animation;

import Game.Events.InGame;

namespace Game::Scene::Impl {
	namespace {
		void PopulateRandomEnemiesIfEmpty(Game::Editor::AreaData& area, const std::vector<std::string>& enemyNames) {
			if (!area.enemies.empty() || enemyNames.empty()) return;

			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> countDist(1, 5);
			std::uniform_int_distribution<int> enemyDist(0, static_cast<int>(enemyNames.size()) - 1);
			std::uniform_int_distribution<int> sizeDist(0, 2);
			std::uniform_real_distribution<float> xDist(120.0f, (std::max)(121.0f, static_cast<float>(area.width) - 120.0f));
			std::uniform_real_distribution<float> yDist(80.0f, (std::max)(81.0f, static_cast<float>(area.height) - 80.0f));
			std::bernoulli_distribution faceDist(0.5);

			int spawnCount = countDist(mt);
			for (int i = 0; i < spawnCount; ++i) {
				Game::Editor::EnemyPlacement ep;
				ep.enemyName = enemyNames[enemyDist(mt)];
				ep.sizeCategory = sizeDist(mt);
				ep.facingRight = faceDist(mt);
				ep.position.x = xDist(mt);
				ep.position.y = yDist(mt);
				area.enemies.push_back(ep);
			}
		}
	}

	// テクスチャ読み込み
	template<>
	auto InGame::Initialize_<"ImageTextures">() -> void {
		// エンジン
		auto& context{ Lumina::Context::Instance() };
		// 画像や音声の読み込みなどを司るやつ
		auto& resMngr{ context.ResourceContext() };
		// D3D12関連
		auto const& d3d12Context{ context.D3D12Context() };
		// D3D12デバイス
		auto const& d3d12Device{ d3d12Context.Device() };

		std::vector<uint32_t> texIDs{};
		std::vector<std::pair<std::string, std::string>> texturesToLoad = {
			{ "uvChecker", "Assets/Img/uvChecker.png" },
			{ "Particles", "Assets/Img/Particles.png" },
			{ "pause", "Assets/Img/UI/pause.png" },
			{ "pause_resume", "Assets/Img/UI/pause_resume.png" },
			{ "pause_restart", "Assets/Img/UI/pause_restart.png" },
			{ "pause_title", "Assets/Img/UI/pause_title.png" },
			{ "gameover_retry", "Assets/Img/UI/Retry.png" },
			{ "gameover_returntotitle", "Assets/Img/UI/returntotitle.png" },
			{ "gameover", "Assets/Img/UI/gameover.png" },
			{ "White16x16", "Assets/Img/White16x16.png" },
			{ "minimap_ui", "Assets/Img/Tutorial/minimap.png" },
			{ "minimap_close_ui", "Assets/Img/Tutorial/minimap_close.png" },
			{ "pause_UI", "Assets/Img/UI/pause_UI.png" },
			{ "playerHead", "Assets/Img/UI/playerHead.png" },
			{ "reticle", "Assets/Img/UI/Umbrella_Reticle.png" },
		};
		// 追加のテクスチャ（敵など）をマージ。チュートリアルの前に登録してインデックスのズレを防ぐ
		for (const auto& addTex : AdditionalTextures_) {
			texturesToLoad.push_back(addTex);
		}

		// チュートリアル用テクスチャ
		std::vector<std::pair<std::string, std::string>> tutorialTextures = {
			{ "tut_step1_move",   "Assets/Img/Tutorial/step1_move.png" },
			{ "tut_step2_jump",   "Assets/Img/Tutorial/step2_jump.png" },
			{ "tut_step3_attack", "Assets/Img/Tutorial/step3_attack.png" },
			{ "tut_step_rakkasan", "Assets/Img/Tutorial/rakkasan.png" },
		};
		for (const auto& tutTex : tutorialTextures) {
			if (std::filesystem::exists(tutTex.second)) {
				texturesToLoad.push_back(tutTex);
			}
		}

		resMngr.Graphics().LoadImageTextures(
			texIDs,
			texturesToLoad
		);

		// シェーダーで使えるディスクリプタ
		GlobalTable_SRV_ImageTexture_ = d3d12Context.GlobalDescriptorHeap().Allocate(64U); // 余裕をもたせて64個確保
		for (uint32_t idx{ 0U }; idx < static_cast<uint32_t>(texIDs.size()); ++idx) {
			
			// さき読み込んだテクスチャのSRVをシェーダーで使えるディスクリプタにコピー
			d3d12Device->CopyDescriptorsSimple(
				1U,
				GlobalTable_SRV_ImageTexture_.CPUHandle(idx),
				resMngr.Graphics().CPUHandle(texIDs.at(idx)),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		// Load spline motion assets at startup so MotionManager has data available
		// Motion JSON files are expected under Assets/Data/Motion/*.json
		MotionManager::GetInstance()->LoadMotions("Assets/Data/Motion/");
	}

	// メッシュ読み込み
	template<>
	auto InGame::Initialize_<"Meshes">() -> void {
		auto const& d3d12Context{ Lumina::Context::Instance().D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		// マルチメッシュ対応なのでstd::vector<Lumina::Utils::Mesh>形式に
		// Lumina::Utils::Meshにはメッシュ1個分が入る
		auto&& teapot{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"teapot.obj", "Assets"
				)
			)
		};

		auto&& umbrellaHandle{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"UmbrellaHandle.obj", "Assets/Hamada/Umbrella"
				)
			)
		};

		auto&& umbrellaCloseTop{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"UmbrellaTopClose.obj", "Assets/Hamada/Umbrella"
				)
			)
		};

		auto&& umbrellaOpenTop{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"UmbrellaTop.obj", "Assets/Hamada/Umbrella"
				)
			)
		};

		auto&& cubeMesh{
			Lumina::Utils::Mesh::Load(
				Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(
					"cube.obj", "Assets"
				)
			)
		};

		using MeshCollection = std::vector<Lumina::Utils::Mesh>;
		
		// アップロード用vector
		MeshCollection meshesToBeUploaded{};

		// メッシュvectorをアップロードリストに追加
		// 可読性向上させるべくラムダ式に
		auto addMeshesToBeUploaded{
			[&] (MeshCollection const& meshCollection_) -> void {
				meshesToBeUploaded.insert(
					meshesToBeUploaded.cend(),
					meshCollection_.cbegin(),
					meshCollection_.cend()
				);
			}
		};

		addMeshesToBeUploaded(teapot);
		addMeshesToBeUploaded(umbrellaHandle);
		addMeshesToBeUploaded(umbrellaCloseTop);
		addMeshesToBeUploaded(umbrellaOpenTop);

		CubeMeshIdx_ = meshesToBeUploaded.size();
		addMeshesToBeUploaded(cubeMesh);

		// 敵用
		EnemyMeshIndices_.clear();
		
		std::map<std::string, std::shared_ptr<SkinnedModel>> gltfCache_Skinned;
		std::map<std::string, MeshRange> gltfCache_Static;
		std::map<std::string, std::string> gltfCache_TexName;
		std::map<std::string, std::string> gltfCache_TexPath;
		std::map<std::string, uint32_t> texNameToIndex;

		namespace fs = std::filesystem;
		if (fs::exists("Assets/Data/Enemy/")) {
			for (const auto& entry : fs::directory_iterator("Assets/Data/Enemy/")) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					std::string fName = entry.path().filename().string();
					
					Game::Editor::EnemyData ed;
					enemyEditor_.LoadEnemy(ed, fName);
					
					if (!ed.gltfPath.empty() && ed.gltfPath.size() > 4) {
						if (gltfCache_Skinned.contains(ed.gltfPath)) {
							EnemySkinnedModels_[ed.name] = gltfCache_Skinned[ed.gltfPath];
							if (gltfCache_TexName.contains(ed.gltfPath)) {
								std::string cachedTexName = gltfCache_TexName[ed.gltfPath];
								if (texNameToIndex.contains(cachedTexName)) {
									EnemyTextureIndices_[ed.name] = texNameToIndex[cachedTexName];
								}
							}
							continue;
						}
						if (gltfCache_Static.contains(ed.gltfPath)) {
							EnemyMeshIndices_[ed.name] = gltfCache_Static[ed.gltfPath];
							if (gltfCache_TexName.contains(ed.gltfPath)) {
								std::string cachedTexName = gltfCache_TexName[ed.gltfPath];
								if (texNameToIndex.contains(cachedTexName)) {
									EnemyTextureIndices_[ed.name] = texNameToIndex[cachedTexName];
								}
							}
							continue;
						}

						std::string ext = ed.gltfPath.substr(ed.gltfPath.size() - 4);
						for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

						try {
							using MeshCollection = std::vector<Lumina::Utils::Mesh>;
							MeshCollection validMeshes;
							std::string diffuseTexName = "";
							std::string diffuseTexPath = "";

							if (ext == ".obj") {
								auto&& objParser = Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(ed.gltfPath);
								
								// OBJからMTL名を取得してテクスチャパスを解決
								if (!objParser.MTLFileNames().empty()) {
									std::string mtlFileName = fs::path(objParser.MTLFileNames()[0]).filename().string();
									std::string mtlPath = (fs::path(ed.gltfPath).parent_path() / mtlFileName).string();
									try {
										auto&& mtlParser = Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontMTL>(mtlPath);
										std::string texRawName = mtlParser.TextureFileName();
										if (!texRawName.empty()) {
											std::string texFileName = fs::path(texRawName).filename().string();
											diffuseTexName = ed.name + "_diffuse";
											diffuseTexPath = (fs::path(mtlPath).parent_path() / texFileName).string();
										}
									} catch(...) {}
								}

								auto&& enemyMesh = Lumina::Utils::Mesh::Load(objParser);
								
								for (auto& m : enemyMesh) {
									if (!m.Positions.empty() && !m.Vertices.empty()) {
										validMeshes.push_back(std::move(m));
									}
								}
							}
							else if (ext == "gltf" || ext == ".glb") {
								fs::path gPath = ed.gltfPath;
								std::string parentPath = gPath.parent_path().string();
								std::replace(parentPath.begin(), parentPath.end(), '\\', '/');
								
								Lumina::String luminaFileName(gPath.filename().string().c_str());
								Lumina::String luminaDirPath(parentPath.c_str());
								auto collection = Lumina::CG3D::Import(gPath.filename().string(), parentPath);
								auto animations = Lumina::CG3D::LoadAnimationFile(luminaFileName, luminaDirPath);
								
								bool hasAnimation = !animations.empty();

								if (hasAnimation && collection.Meshes.size() > 1) {
									Lumina::CG3D::Mesh combinedMesh;
									combinedMesh.Name = collection.Meshes[0].Name;
									combinedMesh.Index_Material = collection.Meshes[0].Index_Material;
									
									size_t vertexOffset = 0;
									
									for (size_t mIdx = 0; mIdx < collection.Meshes.size(); ++mIdx) {
										const auto& mesh = collection.Meshes[mIdx];
										
										combinedMesh.Vertices.insert(
											combinedMesh.Vertices.end(),
											mesh.Vertices.begin(),
											mesh.Vertices.end()
										);
										
										for (auto idx : mesh.Indices) {
											combinedMesh.Indices.push_back(idx + static_cast<uint32_t>(vertexOffset));
										}
										
										for (const auto& pair : mesh.SkinClusterData) {
											const auto& jointName = pair.first;
											const auto& jointWeight = pair.second;
											
											auto& targetWeight = combinedMesh.SkinClusterData[jointName];
											targetWeight.INV_BindPose = jointWeight.INV_BindPose;
											
											for (const auto& vw : jointWeight.VertexWeights) {
												auto vwCopy = vw;
												vwCopy.VertexID += static_cast<uint32_t>(vertexOffset);
												targetWeight.VertexWeights.push_back(vwCopy);
											}
										}
										
										vertexOffset = combinedMesh.Vertices.size();
									}
									
									collection.Meshes.clear();
									collection.Meshes.push_back(std::move(combinedMesh));
								}
								
								if (!collection.Materials.empty() && !collection.Materials[0].FilePath_Diffuse.empty()) {
									diffuseTexName = ed.name + "_diffuse";
									diffuseTexPath = (gPath.parent_path() / collection.Materials[0].FilePath_Diffuse).string();
								}
								
								if (hasAnimation) {
									auto skinnedModel = std::make_shared<SkinnedModel>();
									skinnedModel->Collection_ = std::move(collection);
									skinnedModel->Animations_ = std::move(animations);
									
									skinnedModel->VertexBuffer_.Initialize(
										d3d12Device,
										sizeof(Lumina::CG3D::Mesh::Vertex) * skinnedModel->Collection_.Meshes[0].Vertices.size()
									);
									skinnedModel->VertexBuffer_.Store(
										skinnedModel->Collection_.Meshes[0].Vertices.data(),
										sizeof(Lumina::CG3D::Mesh::Vertex) * skinnedModel->Collection_.Meshes[0].Vertices.size(),
										0LLU
									);
									skinnedModel->VBV_ = Lumina::D3D12::VBV::Create<Lumina::CG3D::Mesh::Vertex>(skinnedModel->VertexBuffer_);
									
									skinnedModel->IndexBuffer_.Initialize(
										d3d12Device,
										sizeof(Lumina::U32) * skinnedModel->Collection_.Meshes[0].Indices.size()
									);
									skinnedModel->IndexBuffer_.Store(
										skinnedModel->Collection_.Meshes[0].Indices.data(),
										sizeof(Lumina::U32) * skinnedModel->Collection_.Meshes[0].Indices.size(),
										0LLU
									);
									skinnedModel->IBV_ = Lumina::D3D12::IBV::Create(skinnedModel->IndexBuffer_);
									
									EnemySkinnedModels_[ed.name] = skinnedModel;
									gltfCache_Skinned[ed.gltfPath] = skinnedModel;
									
									if (!diffuseTexName.empty()) {
										uint32_t newTexIdx = static_cast<uint32_t>(15 + AdditionalTextures_.size());
										EnemyTextureIndices_[ed.name] = newTexIdx;
										AdditionalTextures_.push_back({ diffuseTexName, diffuseTexPath });
										texNameToIndex[diffuseTexName] = newTexIdx;
										gltfCache_TexName[ed.gltfPath] = diffuseTexName;
										gltfCache_TexPath[ed.gltfPath] = diffuseTexPath;
									}
									continue; // Skip static mesh processing
								}
								
								// Process as static mesh if no animation
								std::map<size_t, Lumina::Math::F32x4x4<>> meshTransforms;
								std::function<void(const Lumina::CG3D::Node&, Lumina::Math::F32x4x4<>)> dfs = [&](const Lumina::CG3D::Node& node, Lumina::Math::F32x4x4<> parentMat) {
									Lumina::Math::F32x4x4<> globalMat = node.Transform_Local * parentMat;
									for (auto mIdx : node.Indices_Mesh) {
										meshTransforms[mIdx] = globalMat;
									}
									for (const auto& child : node.Children) {
										dfs(child, globalMat);
									}
								};
								dfs(collection.Root, Lumina::Math::F32x4x4<>::Identity);

								for (size_t mIdx = 0; mIdx < collection.Meshes.size(); ++mIdx) {
									auto& cgMesh = collection.Meshes[mIdx];
									if (cgMesh.Vertices.empty()) continue;

									Lumina::Math::F32x4x4<> globalMat = Lumina::Math::F32x4x4<>::Identity;
									if (meshTransforms.count(mIdx)) {
										globalMat = meshTransforms[mIdx];
									}

									Lumina::Utils::Mesh umesh;
									umesh.Name = cgMesh.Name;
									umesh.Positions.resize(cgMesh.Vertices.size());
									umesh.TexCoords.resize(cgMesh.Vertices.size());
									umesh.Normals.resize(cgMesh.Vertices.size());
									umesh.Tangents.resize(cgMesh.Vertices.size(), {0.0f, 0.0f, 0.0f});
									umesh.Vertices.resize(cgMesh.Indices.size());

									for (size_t i = 0; i < cgMesh.Vertices.size(); ++i) {
										float px = cgMesh.Vertices[i].Position.X;
										float py = cgMesh.Vertices[i].Position.Y;
										float pz = cgMesh.Vertices[i].Position.Z;

										float tx = px * globalMat[0].X() + py * globalMat[1].X() + pz * globalMat[2].X() + globalMat[3].X();
										float ty = px * globalMat[0].Y() + py * globalMat[1].Y() + pz * globalMat[2].Y() + globalMat[3].Y();
										float tz = px * globalMat[0].Z() + py * globalMat[1].Z() + pz * globalMat[2].Z() + globalMat[3].Z();

										umesh.Positions[i] = { tx, ty, tz };
										umesh.TexCoords[i] = cgMesh.Vertices[i].TexCoord;
										
										// Note: Normally normals should be multiplied by InverseTranspose,
										// but for uniform scale/rotation, globalMat is acceptable here.
										float nx = cgMesh.Vertices[i].Normal.X;
										float ny = cgMesh.Vertices[i].Normal.Y;
										float nz = cgMesh.Vertices[i].Normal.Z;
										float tnx = nx * globalMat[0].X() + ny * globalMat[1].X() + nz * globalMat[2].X();
										float tny = nx * globalMat[0].Y() + ny * globalMat[1].Y() + nz * globalMat[2].Y();
										float tnz = nx * globalMat[0].Z() + ny * globalMat[1].Z() + nz * globalMat[2].Z();

										umesh.Normals[i] = { tnx, tny, tnz };
									}

									for (size_t i = 0; i < cgMesh.Indices.size(); i += 3) {
										Lumina::Math::F32x3 tangent = Lumina::Utils::Mesh::CalculateTangent(
											umesh.Positions[cgMesh.Indices[i]], umesh.TexCoords[cgMesh.Indices[i]],
											umesh.Positions[cgMesh.Indices[i+1]], umesh.TexCoords[cgMesh.Indices[i+1]],
											umesh.Positions[cgMesh.Indices[i+2]], umesh.TexCoords[cgMesh.Indices[i+2]]
										);
										umesh.Tangents[cgMesh.Indices[i]] = tangent;
										umesh.Tangents[cgMesh.Indices[i+1]] = tangent;
										umesh.Tangents[cgMesh.Indices[i+2]] = tangent;

										for (int v = 0; v < 3; ++v) {
											uint32_t idx = cgMesh.Indices[i+v];
											umesh.Vertices[i+v].Index_Position = idx;
											umesh.Vertices[i+v].Index_TexCoord = idx;
											umesh.Vertices[i+v].Index_Normal = idx;
											umesh.Vertices[i+v].Index_Tangent = idx;
										}
									}

									validMeshes.push_back(std::move(umesh));
								}
							}

							if (!validMeshes.empty()) {
								EnemyMeshIndices_[ed.name] = { meshesToBeUploaded.size(), validMeshes.size() };
								gltfCache_Static[ed.gltfPath] = EnemyMeshIndices_[ed.name];
								if (!diffuseTexName.empty()) {
									// 既存の基本テクスチャ12枚の後に登録される前提でインデックスを計算
									uint32_t newTexIdx = static_cast<uint32_t>(15 + AdditionalTextures_.size());
									EnemyTextureIndices_[ed.name] = newTexIdx;
									AdditionalTextures_.push_back({ diffuseTexName, diffuseTexPath });
									texNameToIndex[diffuseTexName] = newTexIdx;
									gltfCache_TexName[ed.gltfPath] = diffuseTexName;
									gltfCache_TexPath[ed.gltfPath] = diffuseTexPath;
								}
								addMeshesToBeUploaded(validMeshes);
							}
						} catch (...) {
							// 読み込み失敗時はスキップ
						}
					}
				}
			}
		}

		// Actor用メッシュ（プロジェクタイル描画用）
		ActorMeshIndices_.clear();
		if (fs::exists("Assets/Data/Actor/")) {
			for (const auto& entry : fs::directory_iterator("Assets/Data/Actor/")) {
				if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
				std::string fName = entry.path().filename().string();
				if (fName.find("actor_") != 0 || fName.size() <= 11) continue;

				std::string actorName = fName.substr(6, fName.size() - 11);

				// Actor JSON からメッシュパスを読み取る
				try {
					std::string actorFullPath = "Assets/Data/Actor/" + fName;
					std::ifstream actorFile(actorFullPath);
					if (!actorFile.is_open()) continue;
					nlohmann::json aj;
					actorFile >> aj;

					std::string meshPath;
					if (aj.contains("visual") && aj["visual"].is_object()) {
						if (aj["visual"].contains("meshPath")) {
							meshPath = aj["visual"]["meshPath"].get<std::string>();
						}
					}

					if (meshPath.empty()) continue;
					if (ActorMeshIndices_.contains(actorName)) continue;

					// .obj メッシュをロード
					std::string ext = fs::path(meshPath).extension().string();
					for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

					if (ext == ".obj") {
						auto&& actorMesh = Lumina::Utils::Mesh::Load(
							Lumina::Utils::LoadFromFile<Lumina::Utils::WavefrontOBJ>(meshPath)
						);

						using MeshCollection = std::vector<Lumina::Utils::Mesh>;
						MeshCollection validMeshes;
						for (auto& m : actorMesh) {
							if (!m.Positions.empty() && !m.Vertices.empty()) {
								validMeshes.push_back(std::move(m));
							}
						}

						if (!validMeshes.empty()) {
							ActorMeshIndices_[actorName] = { meshesToBeUploaded.size(), validMeshes.size() };
							addMeshesToBeUploaded(validMeshes);
						}
					}
				} catch (...) {
					// Actor メッシュ読み込み失敗時はスキップ
				}
			}
		}

		// メッシュデータをGPU側にアップロードするやつ
		Lumina::MeshUploader meshUploader{};
		meshUploader.Initialize(d3d12Context);
		meshUploader.Begin();
		for (auto const& mesh : meshesToBeUploaded) {
			meshUploader.Batch(mesh);
		}
		meshUploader.End(MeshShaderAssets_);

		// MeshShaderAssets_ : Lumina::MeshShaderAssetが入ってる
		// Lumina::MeshShaderAsset : バッファとかいろいろシェーダーが使えるやつが入ってて、描画の際にLumina::MeshManagerに渡す
	}
	
	// シェーダーにマテリアルを使ってもらうにはバッファとビューが必要だから
	// ここでこいつらの下ごしらえを
	template<>
	auto InGame::Initialize_<"MeshMaterials">() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		// とりあえず64個分のアップロードバッファを確保する
		UB_Materials_.resize(64U);
		for (auto& ub : UB_Materials_) {
			ub = std::make_unique<Lumina::D3D12::UploadBuffer>();
			ub->Initialize(d3d12Device, 256LLU);
		}

		// マテリアル用ディスクリプタヒープ（64個分）
		// シェーダー側には見えないけど、メッシュバッチとともにメッシュマネージャになんとかしてもらう
		LocalHeap_Materials_.Initialize(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64U, false);
		
		// CBV作成
		Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_Materials_.CPUHandle(0U), *UB_Materials_[0]);
		Material0_.RGBA = { 1.0f, 1.0f, 1.0f, 1.0f };
		Material0_.ID_DiffuseMap = 0;
		UB_Materials_[0]->Store(&Material0_, sizeof(Material0_), 0LLU);

		// 敵用のマテリアルを設定
		uint32_t materialIdx = 1; // 0番は共通で使っているため1番から割り当て
		EnemyMaterialIndices_.clear();

		for (const auto& pair : EnemyTextureIndices_) {
			if (materialIdx >= 64U) break; // 余裕を見て制限

			const auto& enemyName = pair.first;
			uint32_t texIdx = pair.second;

			Lumina::D3D12::CBV::Create(d3d12Device, LocalHeap_Materials_.CPUHandle(materialIdx), *UB_Materials_[materialIdx]);
			
			MeshMaterial mat{};
			mat.RGBA = { 1.0f, 1.0f, 1.0f, 1.0f };
			mat.ID_DiffuseMap = texIdx; // ここに敵ごと固有のテクスチャIDを設定

			UB_Materials_[materialIdx]->Store(&mat, sizeof(MeshMaterial), 0LLU);
			
			EnemyMaterialIndices_[enemyName] = materialIdx;
			materialIdx++;
		}
	}

	template<>
	auto InGame::Initialize_<"Camera">() -> void {
		Camera_ = std::make_unique<Lumina::Utils::Camera>();
		Camera_->LookAt({ 0.0f, 0.0f, -30.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
		Camera_->Perspective(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);

		Camera_Player_ = std::make_unique<Lumina::Utils::Camera>();
		Camera_Player_->LookAt({ 0.0f, 0.0f, -30.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
		Camera_Player_->Perspective(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
	}

	template<>
	auto InGame::Initialize_<"Resource, View">(
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {
		WorldToHomogeneous_ = std::make_unique<Lumina::Math::F32x4x4<>>();
		*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
		UB_WorldToHomogeneous_.Initialize(d3d12Device_, 256LLU);
		LocalHeap_Scene_.Initialize(d3d12Device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16U, false);
		Lumina::D3D12::CBV::Create(d3d12Device_, LocalHeap_Scene_.CPUHandle(0U), UB_WorldToHomogeneous_);

		UB_WorldToHomogeneous_.Store(WorldToHomogeneous_.get(), sizeof(Lumina::Math::F32x4x4<>), 0LLU);
	}
	
	template<>
	auto InGame::Initialize_<"Pipeline, Canvas, RenderPass">() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		d3d12Context.Compile(
			VS_MeshDeferredGeometry_,
			L"Assets/Shaders/MeshCommon.VS.hlsl",
			L"vs_6_6",
			L"main",
			"Mesh.DeferredGeometry.VS"
		);
		d3d12Context.Compile(
			PS_MeshDeferredGeometry_,
			L"Assets/Shaders/MeshCommon.PS.hlsl",
			L"ps_6_6",
			L"main",
			"Mesh.DeferredGeometry.PS"
		);

		Lumina::D3D12::BlendState blendState_None{};
		blendState_None.RenderTarget[0].BlendEnable = false;
		blendState_None.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendState_None.RenderTarget[1].BlendEnable = false;
		blendState_None.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		Lumina::D3D12::GraphicsPSO::InputLayout inputLayout_Mesh{};
		inputLayout_Mesh.Append("IDX_POSITION", 0U, DXGI_FORMAT_R32_UINT);
		inputLayout_Mesh.Append("IDX_TEXCOORD", 0U, DXGI_FORMAT_R32_UINT);
		inputLayout_Mesh.Append("IDX_NORMAL", 0U, DXGI_FORMAT_R32_UINT);
		inputLayout_Mesh.Append("IDX_TANGENT", 0U, DXGI_FORMAT_R32_UINT);

		GraphicsPSO_MeshDeferredGeometry_.Initialize(
			d3d12Device,
			Lumina::Context::Instance().MeshContext().RootSignature(),
			VS_MeshDeferredGeometry_,
			PS_MeshDeferredGeometry_,
			blendState_None,
			Lumina::D3D12::RasterizerState{
				.FillMode{ D3D12_FILL_MODE_SOLID },
				.CullMode{ D3D12_CULL_MODE_BACK },
			},
			Lumina::D3D12::DepthStencilState{
				.DepthEnable{ true },
				.DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ALL },
				.DepthFunc{ D3D12_COMPARISON_FUNC_LESS_EQUAL },
				.StencilEnable{ false },
			},
			inputLayout_Mesh,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			{
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				DXGI_FORMAT_R8G8B8A8_UNORM,
			},
			Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
			);

		Canvas_.AllocateTextures(2U, true);
		Canvas_.RenderTexture(0U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		Canvas_.RenderTexture(1U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM);
		Canvas_.DepthTexture().Initialize(d3d12Device, 1280U, 720U);
		Canvas_.TransitionResourceStates(d3d12Device, d3d12Context.DirectQueue());
		Canvas_.CreateViews(d3d12Device);
		Canvas_.Viewport(0U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 1280.0f },
			.Height{ 720.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_.ScissorRect(0U) = D3D12_RECT{
			.left{ 0 },
			.top{ 0 },
			.right{ 1280 },
			.bottom{ 720 },
		};
		Canvas_.Viewport(1U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 0.0f },
			.Height{ 0.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_.ScissorRect(1U) = D3D12_RECT{
			.left{ 640 },
			.top{ 360 },
			.right{ 1280 },
			.bottom{ 720 },
		};

		Canvas_GeometryPass_.AllocateTextures(3U, true);
		// * Albedo
		Canvas_GeometryPass_.RenderTexture(0U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		// * Normal
		Canvas_GeometryPass_.RenderTexture(1U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM);
		// * BleedingFactor, EdgeDensityFactor
		Canvas_GeometryPass_.RenderTexture(2U).Initialize(d3d12Device, 1280U, 720U, DXGI_FORMAT_R8G8B8A8_UNORM);
		Canvas_GeometryPass_.DepthTexture().Initialize(d3d12Device, 1280U, 720U);
		Canvas_GeometryPass_.TransitionResourceStates(d3d12Device, d3d12Context.DirectQueue());
		Canvas_GeometryPass_.CreateViews(d3d12Device);
		Canvas_GeometryPass_.Viewport(0U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 1280.0f },
			.Height{ 720.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_GeometryPass_.ScissorRect(0U) = D3D12_RECT{
			.left{ 0 },
			.top{ 0 },
			.right{ 1280 },
			.bottom{ 720 },
		};
		Canvas_GeometryPass_.Viewport(1U) = D3D12_VIEWPORT{
			.TopLeftX{ 0.0f },
			.TopLeftY{ 0.0f },
			.Width{ 0.0f },
			.Height{ 0.0f },
			.MinDepth{ 0.0f },
			.MaxDepth{ 1.0f },
		};
		Canvas_GeometryPass_.ScissorRect(1U) = D3D12_RECT{
			.left{ 640 },
			.top{ 360 },
			.right{ 1280 },
			.bottom{ 720 },
		};

		Lumina::F32 const clearColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
		GeometryPass_.Initialize(3U, true);
		GeometryPass_.RenderTarget(0).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			clearColor
		);
		GeometryPass_.RenderTarget(0).EndingEvent().Preserve();
		GeometryPass_.RenderTarget(1).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			clearColor
		);
		GeometryPass_.RenderTarget(1).EndingEvent().Preserve();
		GeometryPass_.RenderTarget(2).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			clearColor
		);
		GeometryPass_.RenderTarget(2).EndingEvent().Preserve();
		GeometryPass_.DepthStencil().DepthBeginningEvent().ClearTarget(
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			{ .Depth{ 1.0f }, }
		);
		GeometryPass_.DepthStencil().DepthEndingEvent().Preserve();
		GeometryPass_.DepthStencil().StencilBeginningEvent().NoAccess();
		GeometryPass_.DepthStencil().StencilEndingEvent().NoAccess();

		for (uint32_t idx{ 0U }; idx < Canvas_GeometryPass_.Num_RenderTargets(); ++idx) {
			GeometryPass_.RenderTarget(idx).View() = Canvas_GeometryPass_.RTV(idx);
		}
		GeometryPass_.DepthStencil().View() = Canvas_GeometryPass_.DSV();
		

		MergePass_.Initialize(1U, true);
		MergePass_.RenderTarget(0).BeginningEvent().ClearTarget(
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			clearColor
		);
		MergePass_.RenderTarget(0).EndingEvent().Preserve();
		MergePass_.DepthStencil().DepthEndingEvent().Preserve();
		MergePass_.DepthStencil().StencilBeginningEvent().NoAccess();
		MergePass_.DepthStencil().StencilEndingEvent().NoAccess();

		PrimitiveManager_ = std::make_unique<Lumina::PrimitiveManager>();
		PrimitiveManager_->Initialize(d3d12Context);

		GlobalTable_SRV_CanvasTexture_ = d3d12Context.GlobalDescriptorHeap().Allocate(8U);
		Lumina::D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_SRV_CanvasTexture_.CPUHandle(0U),
			Canvas_GeometryPass_.RenderTexture(0U)
		);
		Lumina::D3D12::SRV<void>::Create(
			d3d12Device,
			GlobalTable_SRV_CanvasTexture_.CPUHandle(1U),
			Canvas_GeometryPass_.RenderTexture(1U)
		);
	}

	template<>
	auto InGame::Initialize_<"RenderPipeline">() -> void {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

		auto config{ Lumina::Utils::LoadFromFile<nlohmann::json>("Assets/Configs/SkinnedMesh.json") };
		auto&& rsSetup{ Lumina::D3D12::LoadSetup<Lumina::D3D12::RootSignature>(config.at("RS")) };
		RS_Skinning_.Initialize(d3d12Device, rsSetup);

		d3d12Context.Compile(
			VS_SkinnedMeshDeferredGeometry_,
			L"Assets/Shaders/MeshSkinning.VS.hlsl",
			L"vs_6_6",
			L"main",
			"SkinnedMesh.DeferredGeometry.VS"
		);
		d3d12Context.Compile(
			PS_SkinnedMeshDeferredGeometry_,
			L"Assets/Shaders/MeshSkinning.PS.hlsl",
			L"ps_6_6",
			L"main",
			"SkinnedMesh.DeferredGeometry.PS"
		);

		Lumina::D3D12::BlendState blendState_None{};
		blendState_None.RenderTarget[0].BlendEnable = false;
		blendState_None.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendState_None.RenderTarget[1].BlendEnable = false;
		blendState_None.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		Lumina::D3D12::GraphicsPSO::InputLayout inputLayout_Mesh{};
		inputLayout_Mesh.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout_Mesh.Append("TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT);
		inputLayout_Mesh.Append("NORMAL", 0U, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout_Mesh.Append("WEIGHT", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT, 1U);
		inputLayout_Mesh.Append("PALETTE", 0U, DXGI_FORMAT_R32G32B32A32_SINT, 1U);

		GraphicsPSO_SkinnedMeshDeferredGeometry_.Initialize(
			d3d12Device,
			RS_Skinning_,
			VS_SkinnedMeshDeferredGeometry_,
			PS_SkinnedMeshDeferredGeometry_,
			blendState_None,
			Lumina::D3D12::RasterizerState{
				.FillMode{ D3D12_FILL_MODE_SOLID },
				.CullMode{ D3D12_CULL_MODE_BACK },
			},
			Lumina::D3D12::DepthStencilState{
				.DepthEnable{ true },
				.DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ALL },
				.DepthFunc{ D3D12_COMPARISON_FUNC_LESS_EQUAL },
				.StencilEnable{ false },
			},
			inputLayout_Mesh,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			{
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				DXGI_FORMAT_R8G8B8A8_UNORM,
			},
			Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
		);

		UB_Transforms_.Initialize(d3d12Device, 256LLU);
		GlobalTable_CBV_Scene_ = d3d12Context.GlobalDescriptorHeap().Allocate(1U);
		Lumina::D3D12::CBV::Create(d3d12Device, GlobalTable_CBV_Scene_.CPUHandle(0U), UB_Transforms_);

		GlobalTable_Materials_ = d3d12Context.GlobalDescriptorHeap().Allocate(64U);
		for (uint32_t i = 0; i < 64U; ++i) {
			Lumina::D3D12::CBV::Create(d3d12Device, GlobalTable_Materials_.CPUHandle(i), *UB_Materials_[i]);
		}
	}

	template<>
	auto InGame::Initialize_<"Player">() -> void {
		Player_ = std::make_unique<Player>();
		Player_->Initialize();
		Player_->SetMesh(MeshShaderAssets_[0]);
		Player_->SetMeshMaterialCBV(LocalHeap_Materials_.CPUHandle(0U));

		Player_->GetUmbrella().handle_->SetMesh(MeshShaderAssets_[1]);
		Player_->GetUmbrella().handle_->SetMeshMaterialCBV(LocalHeap_Materials_.CPUHandle(0U));

		Player_->GetUmbrella().top_->SetMesh(MeshShaderAssets_[2]);
		Player_->GetUmbrella().top_->SetMeshOpen(MeshShaderAssets_[3]);
		Player_->GetUmbrella().top_->SetMeshMaterialCBV(LocalHeap_Materials_.CPUHandle(0U));

		CollisionManager_ = std::make_unique<CollisionManager>();
		ConvexColliderDebugRenderer_ = std::make_unique<ConvexColliderDebugRenderer>();
		ConvexColliderDebugRenderer_->Initialize();
	}

	template<>
	auto InGame::Initialize_<"[Debug]">() -> void {
		#if defined(_DEBUG)
		TerrainEditor_ = std::make_unique<TerrainEditor>();
		TerrainEditor_->Initialize();
		TerrainEditor_->SetShapes(*Terrain_);
		TerrainEditor_->SetCamera(*Camera_);
		TerrainEditor_->SetViewport(reinterpret_cast<Lumina::Utils::Viewport const&>(Canvas_.Viewport(0U)));
		enemyEditor_.Initialize();
		objMotionEditor_.Initialize();
		#endif

		// Release ビルドでもミニマップ用にエリアデータを読み込む
		areaEditor_.Initialize();

		playState_.IsPlaying = true;
		CheckAndLoadArea(0);
	}

	template<>
	auto InGame::Initialize_<"Lighting">(
		Lumina::D3D12::Context const& d3d12Context_
	) -> void {
		DeferredLighting_ = std::make_unique<Lumina::DeferredLighting>();
		DeferredLighting_->Initialize(d3d12Context_, 1280U, 720U);

		List_PointLight_.Initialize(2048U);
		List_LocalToWorld_LightSphere_.Initialize(2048U);
	}

	template<>
	auto InGame::Initialize_<"Particles">(
		Lumina::D3D12::Context const& d3d12Context_,
		Lumina::D3D12::GraphicsDevice const& d3d12Device_
	) -> void {

		// * パイプライン初期化

		auto config_ParticleSystem{
			Lumina::Utils::LoadFromFile<nlohmann::json>(
				"Assets/Configs/ParticleSystem.json"
			)
		};
		RS_ParticleSystem_.Initialize(
			d3d12Device_,
			Lumina::D3D12::LoadSetup<Lumina::D3D12::RootSignature>(
				config_ParticleSystem.at("Common RS")
			)
		);

		d3d12Context_.Compile(
			VS_BasicParticle_,
			L"Assets/Shaders/Particle2.VS.hlsl",
			L"vs_6_6",
			L"main",
			"Particle2.VS"
		);
		d3d12Context_.Compile(
			PS_BasicParticle_,
			L"Assets/Shaders/Particle2.PS.hlsl",
			L"ps_6_6",
			L"main",
			"Particle2.PS"
		);

		Lumina::D3D12::BlendState blendState_AdditiveMode{};
		blendState_AdditiveMode.RenderTarget[0] = {
			.BlendEnable{ true },
			.SrcBlend{ D3D12_BLEND_SRC_ALPHA },
			.DestBlend{ D3D12_BLEND_ONE },
			.BlendOp{ D3D12_BLEND_OP_ADD },
			.SrcBlendAlpha{ D3D12_BLEND_SRC_ALPHA },
			.DestBlendAlpha{ D3D12_BLEND_ONE },
			.BlendOpAlpha{ D3D12_BLEND_OP_ADD },
			.RenderTargetWriteMask{ D3D12_COLOR_WRITE_ENABLE_ALL },
		};

		Lumina::D3D12::GraphicsPSO::InputLayout inputLayout_Particle{};
		inputLayout_Particle.Append("POSITION", 0U, DXGI_FORMAT_R32G32B32A32_FLOAT);
		inputLayout_Particle.Append("TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT);
		GraphicsPSO_BasicParticle_AdditiveMode_.Initialize(
			d3d12Device_,
			RS_ParticleSystem_,
			VS_BasicParticle_,
			PS_BasicParticle_,
			blendState_AdditiveMode,
			Lumina::D3D12::RasterizerState{
				.FillMode{ D3D12_FILL_MODE_SOLID },
				.CullMode{ D3D12_CULL_MODE_NONE },
			},
			Lumina::D3D12::DepthStencilState{
				.DepthEnable{ true },
				.DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ZERO },
				.DepthFunc{ D3D12_COMPARISON_FUNC_LESS_EQUAL },
				.StencilEnable{ false },
			},
			inputLayout_Particle,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			{
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				DXGI_FORMAT_R8G8B8A8_UNORM,
			},
			Lumina::D3D12::GraphicsPSO::DefaultDSVFormat
		);

		// * 定数バッファ初期化

		UB_WorldToProjective_.Initialize(d3d12Device_, 256LLU);
		LocalHeap_CBV_.Initialize(d3d12Device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32U, false);
		Lumina::D3D12::CBV::Create(d3d12Device_, LocalHeap_CBV_.CPUHandle(0U), UB_WorldToProjective_);

		// * レンダラ

		AmbientSparkles_ = std::make_unique<Lumina::ParticleSystem<Lumina::Particle>>();
		AmbientSparkles_->Initialize(d3d12Context_, 384U);
		Raindrops_ = std::make_unique<Lumina::ParticleSystem<Lumina::Particle>>();
		Raindrops_->Initialize(d3d12Context_, 1024U);

		PlayerEffects_ = std::make_unique<Lumina::ParticleSystem<Lumina::Particle>>();
		PlayerEffects_->Initialize(d3d12Context_, 512U);
		UmbrellaEffects_ = std::make_unique<Lumina::ParticleSystem<Lumina::Particle>>();
		UmbrellaEffects_->Initialize(d3d12Context_, 512U);

		KnockEffects_ = std::make_unique<Lumina::ParticleSystem<Lumina::Particle>>();
		KnockEffects_->Initialize(d3d12Context_, 256U);

		EnemyEffects_ = std::make_unique<Lumina::ParticleSystem<Lumina::Particle>>();
		EnemyEffects_->Initialize(d3d12Context_, 512U);
	}

	template<>
	auto InGame::Initialize_<"Events">() -> void {
		auto& context{ Lumina::Context::Instance() };
		auto& eventMngr{ context.EventContext() };

		eventMngr.RegisterType<Event::InGame::OnPlayerMove>();
		eventMngr.RegisterType<Event::InGame::OnPlayerJump>();
		eventMngr.RegisterType<Event::InGame::OnPlayerAttack>();

		eventMngr.AddEventListener<Event::InGame::OnPlayerMove>(
			[this] (Event::InGame::OnPlayerMove& event_) {
				this->Update_<"OnPlayerMove">(event_);
			}
		);
		eventMngr.AddEventListener<Event::InGame::OnPlayerJump>(
			[this] (Event::InGame::OnPlayerJump& event_) {
				this->Update_<"OnPlayerJump">(event_);
			}
		);
		eventMngr.AddEventListener<Event::InGame::OnPlayerAttack>(
			[this] (Event::InGame::OnPlayerAttack& event_) {
				this->Update_<"OnPlayerAttack">(event_);
			}
		);
	}

	template<>
	auto InGame::Initialize_<"Watercolor">() -> void {
		[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
		[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
		[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

		Watercolor_ = std::make_unique<Lumina::Watercolor>();
		Watercolor_->Initialize();
	}

	void InGame::Initialize() {
		auto& context{ Lumina::Context::Instance() };
		auto const& d3d12Context{ context.D3D12Context() };
		auto const& d3d12Device{ d3d12Context.Device() };

		MotionManager::GetInstance()->LoadMotions("Assets/Data/Motion/");

		Initialize_<"Meshes">();
		Initialize_<"ImageTextures">();
		Initialize_<"MeshMaterials">();
		Initialize_<"Camera">();
		Initialize_<"Resource, View">(d3d12Device);
		Initialize_<"Pipeline, Canvas, RenderPass">();
		Initialize_<"Player">();
		Initialize_<"Lighting">(d3d12Context);
		Initialize_<"Particles">(d3d12Context, d3d12Device);
		Initialize_<"RenderPipeline">();
		Initialize_<"Watercolor">();
		Initialize_<"Events">();

		Terrain_ = std::make_unique<TerrainShapeCollection>();
		Terrain_->Initialize(Lumina::Utils::LoadFromFile<nlohmann::json>("Assets/Data/Terrain/area0.json"));

		TerrainRenderer_ = std::make_unique<TerrainRenderer>();
		TerrainRenderer_->Initialize();

		Initialize_<"[Debug]">();

		// チュートリアルマネージャー初期化
		TutorialManager_ = std::make_unique<Game::TutorialManager>();
		TutorialManager_->Initialize();
		TutorialManager_->RegisterSequences();
		// チュートリアルテクスチャは基本テクスチャ11枚 + 追加テクスチャの直後に配置
		TutorialManager_->TutorialTextureStartIndex = 14U + static_cast<uint32_t>(AdditionalTextures_.size());
		TutorialManager_->TutorialTextureCount = 4U;

		// チュートリアル用PrimitiveManager（深度テストなし、オーバーレイ描画用）
		PrimitiveManager_Tutorial_ = std::make_unique<Lumina::PrimitiveManager>();
		PrimitiveManager_Tutorial_->Initialize(
			d3d12Context,
			L"Assets/Shaders/Primitive.VS.hlsl",
			L"Assets/Shaders/Primitive.PS.hlsl",
			false,
			false  // 深度テスト無効
		);

		// ミニマップ専用PrimitiveManager（深度テストなし）
		PrimitiveManager_Minimap_ = std::make_unique<Lumina::PrimitiveManager>();
		PrimitiveManager_Minimap_->Initialize(
			d3d12Context,
			L"Assets/Shaders/Primitive.VS.hlsl",
			L"Assets/Shaders/Primitive.PS.hlsl",
			false,
			false  // 深度テスト無効
		);

		// 初回（セッション内）かつエリア0ならチュートリアル開始
		if (playState_.CurrentArea.index == 0) {
			TutorialManager_->TryStartSequence("BasicControls");
		}
	}

	InGame::InGame() = default;
	InGame::~InGame() = default;
}