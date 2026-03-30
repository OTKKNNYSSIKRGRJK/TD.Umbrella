module Game.Editor.ActorEditor;

import <fstream>;
import <filesystem>;
import <string>;
import <map>;

import nlohmann.json;
import Game.MotionManager;

import Lumina.Utils.Data;

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Game::Editor {

	// ============================================================
	// JSON Serialization
	// ============================================================

	void to_json(json& j, const ActorTransform& t) {
		j = json{
			{"posX", t.posX}, {"posY", t.posY}, {"posZ", t.posZ},
			{"rotX", t.rotX}, {"rotY", t.rotY}, {"rotZ", t.rotZ},
			{"scaleX", t.scaleX}, {"scaleY", t.scaleY}, {"scaleZ", t.scaleZ}
		};
	}
	void from_json(const json& j, ActorTransform& t) {
		if (j.contains("posX")) j.at("posX").get_to(t.posX);
		if (j.contains("posY")) j.at("posY").get_to(t.posY);
		if (j.contains("posZ")) j.at("posZ").get_to(t.posZ);
		if (j.contains("rotX")) j.at("rotX").get_to(t.rotX);
		if (j.contains("rotY")) j.at("rotY").get_to(t.rotY);
		if (j.contains("rotZ")) j.at("rotZ").get_to(t.rotZ);
		if (j.contains("scaleX")) j.at("scaleX").get_to(t.scaleX);
		if (j.contains("scaleY")) j.at("scaleY").get_to(t.scaleY);
		if (j.contains("scaleZ")) j.at("scaleZ").get_to(t.scaleZ);
	}

	void to_json(json& j, const ActorVisual& v) {
		j = json{ {"meshPath", v.meshPath}, {"materialIndex", v.materialIndex} };
	}
	void from_json(const json& j, ActorVisual& v) {
		if (j.contains("meshPath")) j.at("meshPath").get_to(v.meshPath);
		if (j.contains("materialIndex")) j.at("materialIndex").get_to(v.materialIndex);
	}

	void to_json(json& j, const ActorCollisionVertex& v) {
		j = json{ {"x", v.x}, {"y", v.y} };
	}
	void from_json(const json& j, ActorCollisionVertex& v) {
		if (j.contains("x")) j.at("x").get_to(v.x);
		if (j.contains("y")) j.at("y").get_to(v.y);
	}

	void to_json(json& j, const ActorCollider& c) {
		j = json{
			{"type", static_cast<int>(c.type)},
			{"sizeX", c.sizeX}, {"sizeY", c.sizeY}, {"sizeZ", c.sizeZ},
			{"collisionVertices", c.collisionVertices}
		};
	}
	void from_json(const json& j, ActorCollider& c) {
		if (j.contains("type")) c.type = static_cast<ColliderType>(j.at("type").get<int>());
		if (j.contains("sizeX")) j.at("sizeX").get_to(c.sizeX);
		if (j.contains("sizeY")) j.at("sizeY").get_to(c.sizeY);
		if (j.contains("sizeZ")) j.at("sizeZ").get_to(c.sizeZ);
		if (j.contains("collisionVertices")) j.at("collisionVertices").get_to(c.collisionVertices);
	}

	void to_json(json& j, const NodeTiming& n) {
		j = json{ {"arrivalTime", n.arrivalTime}, {"easing", static_cast<int>(n.easing)} };
	}
	void from_json(const json& j, NodeTiming& n) {
		if (j.contains("arrivalTime")) j.at("arrivalTime").get_to(n.arrivalTime);
		if (j.contains("easing")) n.easing = static_cast<EasingType>(j.at("easing").get<int>());
	}

	void to_json(json& j, const MovementModule& m) {
		j = json{
			{"type", static_cast<int>(m.type)},
			{"speed", m.speed},
			{"dirX", m.dirX}, {"dirY", m.dirY}, {"dirZ", m.dirZ},
			{"range", m.range},
			{"easing", static_cast<int>(m.easing)},
			{"splineMotionName", m.splineMotionName},
			{"totalDuration", m.totalDuration},
			{"loopSpline", m.loopSpline},
			{"nodeTimings", m.nodeTimings}
		};
	}
	void from_json(const json& j, MovementModule& m) {
		if (j.contains("type")) m.type = static_cast<MovementType>(j.at("type").get<int>());
		if (j.contains("speed")) j.at("speed").get_to(m.speed);
		if (j.contains("dirX")) j.at("dirX").get_to(m.dirX);
		if (j.contains("dirY")) j.at("dirY").get_to(m.dirY);
		if (j.contains("dirZ")) j.at("dirZ").get_to(m.dirZ);
		if (j.contains("range")) j.at("range").get_to(m.range);
		if (j.contains("easing")) m.easing = static_cast<EasingType>(j.at("easing").get<int>());
		if (j.contains("splineMotionName")) j.at("splineMotionName").get_to(m.splineMotionName);
		if (j.contains("totalDuration")) j.at("totalDuration").get_to(m.totalDuration);
		if (j.contains("loopSpline")) j.at("loopSpline").get_to(m.loopSpline);
		if (j.contains("nodeTimings")) j.at("nodeTimings").get_to(m.nodeTimings);
	}

	void to_json(json& j, const InteractionModule& i) {
		j = json{
			{"type", static_cast<int>(i.type)},
			{"damageValue", i.damageValue},
			{"pushForce", i.pushForce},
			{"activationTriggerID", i.activationTriggerID}
		};
	}
	void from_json(const json& j, InteractionModule& i) {
		if (j.contains("type")) i.type = static_cast<InteractionType>(j.at("type").get<int>());
		if (j.contains("damageValue")) j.at("damageValue").get_to(i.damageValue);
		if (j.contains("pushForce")) j.at("pushForce").get_to(i.pushForce);
		if (j.contains("activationTriggerID")) j.at("activationTriggerID").get_to(i.activationTriggerID);
	}

	void to_json(json& j, const LifecycleModule& l) {
		j = json{
			{"spawnTrigger", static_cast<int>(l.spawnTrigger)},
			{"spawnValue", l.spawnValue},
			{"spawnEventID", l.spawnEventID},
			{"lifetime", l.lifetime},
			{"autoDestroyOffscreen", l.autoDestroyOffscreen}
		};
	}
	void from_json(const json& j, LifecycleModule& l) {
		if (j.contains("spawnTrigger")) l.spawnTrigger = static_cast<SpawnTriggerType>(j.at("spawnTrigger").get<int>());
		if (j.contains("spawnValue")) j.at("spawnValue").get_to(l.spawnValue);
		if (j.contains("spawnEventID")) j.at("spawnEventID").get_to(l.spawnEventID);
		if (j.contains("lifetime")) j.at("lifetime").get_to(l.lifetime);
		if (j.contains("autoDestroyOffscreen")) j.at("autoDestroyOffscreen").get_to(l.autoDestroyOffscreen);
	}

	void to_json(json& j, const ActorData& a) {
		j = json{
			{"name", a.name},
			{"transform", a.transform},
			{"visual", a.visual},
			{"collider", a.collider},
			{"movement", a.movement},
			{"interaction", a.interaction},
			{"lifecycle", a.lifecycle}
		};
	}
	void from_json(const json& j, ActorData& a) {
		if (j.contains("name")) j.at("name").get_to(a.name);
		if (j.contains("transform")) j.at("transform").get_to(a.transform);
		if (j.contains("visual")) j.at("visual").get_to(a.visual);
		if (j.contains("collider")) j.at("collider").get_to(a.collider);
		if (j.contains("movement")) j.at("movement").get_to(a.movement);
		if (j.contains("interaction")) j.at("interaction").get_to(a.interaction);
		if (j.contains("lifecycle")) j.at("lifecycle").get_to(a.lifecycle);
	}

	// ============================================================
	// ActorEditor
	// ============================================================

	void ActorEditor::Initialize() {
		ScanMotionFiles();
	}

	void ActorEditor::SaveActor(const ActorData& actor) {
		std::string filename = "actor_" + actor.name + ".json";
		std::ofstream file(filename);
		if (file.is_open()) {
			json j = actor;
			file << j.dump(4);
		}
	}

	void ActorEditor::LoadActor(ActorData& actor, const std::string& filename) {
		std::ifstream file(filename);
		if (file.is_open()) {
			json j;
			file >> j;
			actor = j.get<ActorData>();
		}
	}

	void ActorEditor::ScanMotionFiles() {
		motionFiles_.clear();
		motionFiles_.push_back(""); // (none)
		const std::string motionDir = "Assets/Data/Motion/";
		if (fs::exists(motionDir)) {
			for (const auto& entry : fs::directory_iterator(motionDir)) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					motionFiles_.push_back(entry.path().stem().string());
				}
			}
		}
	}

	void ActorEditor::SyncNodeTimings() {
		// cachedNodes_ の数と nodeTimings の数を合わせる
		auto& timings = editingActor_.movement.nodeTimings;
		int nodeCount = static_cast<int>(cachedNodes_.size());

		if (static_cast<int>(timings.size()) == nodeCount) return;

		float totalDur = editingActor_.movement.totalDuration;
		timings.resize(nodeCount);
		// 新しく増えた分は均等割り当て
		for (int i = 0; i < nodeCount; ++i) {
			timings[i].arrivalTime = (nodeCount > 1)
				? totalDur * static_cast<float>(i) / static_cast<float>(nodeCount - 1)
				: 0.0f;
		}
	}

	void ActorEditor::ExtractMeshWireframe(const std::string& meshPath) {
		cachedMeshPositions_.clear();
		cachedMeshEdges_.clear();
		cachedMeshPath_ = meshPath;

		if (meshPath.empty() || !fs::exists(meshPath)) return;

		std::string ext = fs::path(meshPath).extension().string();
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		json gltfJson;
		std::vector<uint8_t> binData;

		if (ext == ".gltf") {
			std::ifstream ifs(meshPath);
			if (!ifs.is_open()) return;
			try { ifs >> gltfJson; } catch (...) { return; }

			// .bin ファイルを探す
			if (gltfJson.contains("buffers") && gltfJson["buffers"].is_array() &&
				!gltfJson["buffers"].empty()) {
				auto& buf0 = gltfJson["buffers"][0];
				if (buf0.contains("uri") && buf0["uri"].is_string()) {
					fs::path parentPath = fs::path(meshPath).parent_path();
					std::string binPath;
					if (parentPath.empty()) {
						binPath = buf0["uri"].get<std::string>();
					} else {
						binPath = parentPath.string() + "/" + buf0["uri"].get<std::string>();
					}
					std::ifstream binFile(binPath, std::ios::binary | std::ios::ate);
					if (binFile.is_open()) {
						size_t sz = static_cast<size_t>(binFile.tellg());
						binFile.seekg(0);
						binData.resize(sz);
						binFile.read(reinterpret_cast<char*>(binData.data()), sz);
					}
				}
			}
		} else if (ext == ".glb") {
			std::ifstream ifs(meshPath, std::ios::binary);
			if (!ifs.is_open()) return;

			uint32_t magic = 0, version = 0, totalLength = 0;
			ifs.read(reinterpret_cast<char*>(&magic), 4);
			ifs.read(reinterpret_cast<char*>(&version), 4);
			ifs.read(reinterpret_cast<char*>(&totalLength), 4);
			if (magic != 0x46546C67) return;

			// JSON チャンク
			uint32_t chunkLen = 0, chunkType = 0;
			ifs.read(reinterpret_cast<char*>(&chunkLen), 4);
			ifs.read(reinterpret_cast<char*>(&chunkType), 4);
			if (chunkType != 0x4E4F534A) return;

			std::string jsonStr(chunkLen, '\0');
			ifs.read(jsonStr.data(), chunkLen);
			try { gltfJson = json::parse(jsonStr); } catch (...) { return; }

			// BIN チャンク
			if (ifs.peek() != EOF) {
				ifs.read(reinterpret_cast<char*>(&chunkLen), 4);
				ifs.read(reinterpret_cast<char*>(&chunkType), 4);
				if (chunkType == 0x004E4942) {
					binData.resize(chunkLen);
					ifs.read(reinterpret_cast<char*>(binData.data()), chunkLen);
				}
			}
		} else {
			return;
		}

		if (binData.empty()) return;
		if (!gltfJson.contains("meshes") || !gltfJson["meshes"].is_array()) return;
		if (!gltfJson.contains("accessors") || !gltfJson.contains("bufferViews")) return;

		const auto& accessors = gltfJson["accessors"];
		const auto& bufferViews = gltfJson["bufferViews"];

		// ヘルパー: accessor から float 配列を読み取る
		auto readFloats = [&](int accIdx, int expectedComponents) -> std::vector<float> {
			std::vector<float> result;
			if (accIdx < 0 || accIdx >= static_cast<int>(accessors.size())) return result;
			const auto& acc = accessors[accIdx];
			int count = acc.value("count", 0);
			int bvIdx = acc.value("bufferView", -1);
			int accOffset = acc.value("byteOffset", 0);
			if (bvIdx < 0 || bvIdx >= static_cast<int>(bufferViews.size())) return result;
			const auto& bv = bufferViews[bvIdx];
			int bvOffset = bv.value("byteOffset", 0);
			int stride = bv.value("byteStride", expectedComponents * 4);

			result.reserve(count * expectedComponents);
			for (int i = 0; i < count; ++i) {
				size_t base = static_cast<size_t>(bvOffset + accOffset + i * stride);
				for (int c = 0; c < expectedComponents; ++c) {
					size_t off = base + c * sizeof(float);
					if (off + sizeof(float) > binData.size()) { result.push_back(0.0f); continue; }
					float val;
					const uint8_t* src = &binData[off];
					uint8_t* dst = reinterpret_cast<uint8_t*>(&val);
					for (size_t b = 0; b < sizeof(float); ++b) dst[b] = src[b];
					result.push_back(val);
				}
			}
			return result;
		};

		// ヘルパー: accessor から uint16/uint32 インデックスを読み取る
		auto readIndices = [&](int accIdx) -> std::vector<uint32_t> {
			std::vector<uint32_t> result;
			if (accIdx < 0 || accIdx >= static_cast<int>(accessors.size())) return result;
			const auto& acc = accessors[accIdx];
			int count = acc.value("count", 0);
			int componentType = acc.value("componentType", 0);
			int bvIdx = acc.value("bufferView", -1);
			int accOffset = acc.value("byteOffset", 0);
			if (bvIdx < 0 || bvIdx >= static_cast<int>(bufferViews.size())) return result;
			const auto& bv = bufferViews[bvIdx];
			int bvOffset = bv.value("byteOffset", 0);

			result.reserve(count);
			for (int i = 0; i < count; ++i) {
				size_t base = static_cast<size_t>(bvOffset + accOffset);
				if (componentType == 5123) { // UNSIGNED_SHORT
					size_t off = base + i * sizeof(uint16_t);
					if (off + sizeof(uint16_t) > binData.size()) continue;
					uint16_t val;
					const uint8_t* src16 = &binData[off];
					uint8_t* dst16 = reinterpret_cast<uint8_t*>(&val);
					for (size_t b = 0; b < sizeof(uint16_t); ++b) dst16[b] = src16[b];
					result.push_back(static_cast<uint32_t>(val));
				} else if (componentType == 5125) { // UNSIGNED_INT
					size_t off = base + i * sizeof(uint32_t);
					if (off + sizeof(uint32_t) > binData.size()) continue;
					uint32_t val;
					const uint8_t* src32 = &binData[off];
					uint8_t* dst32 = reinterpret_cast<uint8_t*>(&val);
					for (size_t b = 0; b < sizeof(uint32_t); ++b) dst32[b] = src32[b];
					result.push_back(val);
				} else if (componentType == 5121) { // UNSIGNED_BYTE
					size_t off = base + i;
					if (off >= binData.size()) continue;
					result.push_back(static_cast<uint32_t>(binData[off]));
				}
			}
			return result;
		};

		// ノード階層からの各メッシュのグローバルトランスフォームの取得
		std::map<int, std::vector<int>> childrenMap;
		std::map<int, std::array<float, 16>> nodeTransforms;
		std::map<int, int> nodeToMesh;

		if (gltfJson.contains("nodes") && gltfJson["nodes"].is_array()) {
			int idx = 0;
			for (const auto& node : gltfJson["nodes"]) {
				std::array<float, 16> localMat = {
					1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
				};
				if (node.contains("matrix")) {
					for (int i=0; i<16; ++i) localMat[i] = node["matrix"][i].get<float>();
				} else {
					if (node.contains("translation")) {
						localMat[12] = node["translation"][0].get<float>();
						localMat[13] = node["translation"][1].get<float>();
						localMat[14] = node["translation"][2].get<float>();
					}
					if (node.contains("scale")) {
						localMat[0] = node["scale"][0].get<float>();
						localMat[5] = node["scale"][1].get<float>();
						localMat[10] = node["scale"][2].get<float>();
					}
				}
				nodeTransforms[idx] = localMat;
				if (node.contains("mesh")) {
					nodeToMesh[idx] = node["mesh"].get<int>();
				}
				if (node.contains("children")) {
					for (auto& c : node["children"]) {
						childrenMap[idx].push_back(c.get<int>());
					}
				}
				idx++;
			}
		}

		std::vector<std::pair<int, std::array<float, 16>>> meshInstances;
		std::function<void(int, std::array<float, 16>)> dfs = [&](int nodeIdx, std::array<float, 16> parentMat) {
			std::array<float, 16> globalMat{};
			for (int i=0; i<4; ++i) {
				for (int j=0; j<4; ++j) {
					for (int k=0; k<4; ++k) {
						globalMat[i + j*4] += parentMat[i + k*4] * nodeTransforms[nodeIdx][k + j*4];
					}
				}
			}
			if (nodeToMesh.count(nodeIdx)) {
				meshInstances.push_back({ nodeToMesh[nodeIdx], globalMat });
			}
			for (int child : childrenMap[nodeIdx]) {
				dfs(child, globalMat);
			}
		};

		std::map<int, bool> isChild;
		for (auto& [p, children] : childrenMap) {
			for (int c : children) isChild[c] = true;
		}
		if (gltfJson.contains("nodes") && gltfJson["nodes"].is_array()) {
			for (int i = 0; i < static_cast<int>(gltfJson["nodes"].size()); ++i) {
				if (!isChild[i]) {
					std::array<float, 16> rootMat = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
					dfs(i, rootMat);
				}
			}
		}

		// ノードが存在しない、またはメッシュがノードに関連付けられていない場合のフォールバック
		if (meshInstances.empty()) {
			for (size_t i = 0; i < gltfJson["meshes"].size(); ++i) {
				std::array<float, 16> rootMat = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
				meshInstances.push_back({ static_cast<int>(i), rootMat });
			}
		}

		// 全インスタンスの全プリミティブを処理
		for (const auto& instance : meshInstances) {
			int meshIdx = instance.first;
			if (meshIdx < 0 || meshIdx >= static_cast<int>(gltfJson["meshes"].size())) continue;
			const auto& mesh = gltfJson["meshes"][meshIdx];
			const auto& mat = instance.second;

			if (!mesh.contains("primitives")) continue;
			for (const auto& prim : mesh["primitives"]) {
				// POSITION 取得
				if (!prim.contains("attributes") ||
					!prim["attributes"].contains("POSITION")) continue;
				int posAccIdx = prim["attributes"]["POSITION"].get<int>();
				auto positions = readFloats(posAccIdx, 3);

				int baseVertex = static_cast<int>(cachedMeshPositions_.size());
				for (size_t i = 0; i + 2 < positions.size(); i += 3) {
					float px = positions[i];
					float py = positions[i+1];
					float pz = positions[i+2];

					float tx = px * mat[0] + py * mat[4] + pz * mat[8] + mat[12];
					float ty = px * mat[1] + py * mat[5] + pz * mat[9] + mat[13];
					float tz = px * mat[2] + py * mat[6] + pz * mat[10] + mat[14];

					cachedMeshPositions_.push_back({ tx, ty, tz });
				}

				// インデックス取得
				std::vector<uint32_t> indices;
				if (prim.contains("indices")) {
					indices = readIndices(prim["indices"].get<int>());
				} else {
					int numVerts = static_cast<int>(positions.size()) / 3;
					for (int i = 0; i < numVerts; ++i) indices.push_back(static_cast<uint32_t>(i));
				}

				// エッジ重複排除用
				std::map<int64_t, bool> edgeMap;
				auto edgeKey = [](int a, int b) -> int64_t {
					if (a > b) { int t = a; a = b; b = t; }
					return (static_cast<int64_t>(a) << 32) | static_cast<int64_t>(b);
				};

				// 三角形からエッジを抽出
				for (size_t i = 0; i + 2 < indices.size(); i += 3) {
					int v0 = baseVertex + static_cast<int>(indices[i]);
					int v1 = baseVertex + static_cast<int>(indices[i+1]);
					int v2 = baseVertex + static_cast<int>(indices[i+2]);

					auto addEdge = [&](int a, int b) {
						int64_t key = edgeKey(a, b);
						if (edgeMap.find(key) == edgeMap.end()) {
							edgeMap[key] = true;
							cachedMeshEdges_.push_back({ a, b });
						}
					};
					addEdge(v0, v1);
					addEdge(v1, v2);
					addEdge(v2, v0);
				}
			}
		}
	}
}
