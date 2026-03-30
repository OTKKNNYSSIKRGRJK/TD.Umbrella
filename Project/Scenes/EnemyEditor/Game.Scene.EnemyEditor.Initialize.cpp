module Game.Editor.EnemyEditor;

import <fstream>;
import <filesystem>;
import <string>;
import <map>;

import nlohmann.json;

import Lumina.Utils.Data;

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Game::Editor {
	void to_json(json& j, const CollisionVertex& v) {
		j = json{ {"x", v.x}, {"y", v.y} };
	}
	void from_json(const json& j, CollisionVertex& v) {
		if (j.contains("x")) j.at("x").get_to(v.x);
		if (j.contains("y")) j.at("y").get_to(v.y);
	}

	// JSON シリアライズ定義
	void to_json(json& j, const EnemyData& e) {
		j = json{
			{"name", e.name}, {"hp", e.hp}, {"power", e.power},
			{"gltfPath", e.gltfPath}, {"animationMap", e.animationMap},
			{"motionMap", e.motionMap},
			{"collisionVertices", e.collisionVertices},
			{"aggroRadius", e.aggroRadius}, {"attackRange", e.attackRange},
			{"moveSpeed", e.moveSpeed}, {"attackCooldown", e.attackCooldown},
			{"retreatThreshold", e.retreatThreshold},
			{"patrolRadius", e.patrolRadius}, {"aggressiveness", e.aggressiveness}
		};
	}
	void from_json(const json& j, EnemyData& e) {
		if (j.contains("name")) j.at("name").get_to(e.name);
		if (j.contains("hp")) j.at("hp").get_to(e.hp);
		if (j.contains("power")) j.at("power").get_to(e.power);
		if (j.contains("gltfPath")) j.at("gltfPath").get_to(e.gltfPath);
		if (j.contains("animationMap")) j.at("animationMap").get_to(e.animationMap);
		if (j.contains("motionMap")) j.at("motionMap").get_to(e.motionMap);
		if (j.contains("collisionVertices")) j.at("collisionVertices").get_to(e.collisionVertices);
		if (j.contains("aggroRadius")) j.at("aggroRadius").get_to(e.aggroRadius);
		if (j.contains("attackRange")) j.at("attackRange").get_to(e.attackRange);
		if (j.contains("moveSpeed")) j.at("moveSpeed").get_to(e.moveSpeed);
		if (j.contains("attackCooldown")) j.at("attackCooldown").get_to(e.attackCooldown);
		if (j.contains("retreatThreshold")) j.at("retreatThreshold").get_to(e.retreatThreshold);
		if (j.contains("patrolRadius")) j.at("patrolRadius").get_to(e.patrolRadius);
		if (j.contains("aggressiveness")) j.at("aggressiveness").get_to(e.aggressiveness);
	}

	void EnemyEditor::Initialize() {
	}

	void EnemyEditor::SaveEnemy(const EnemyData& enemy) {
		std::string filename = enemy.name + ".json";
		std::ofstream file(filename);
		if (file.is_open()) {
			json j = enemy;
			file << j.dump(4);
		}
	}

	void EnemyEditor::LoadEnemy(EnemyData& enemy, const std::string& filename) {
		std::ifstream file(filename);
		if (file.is_open()) {
			json j;
			file >> j;
			enemy = j.get<EnemyData>();
		}
	}

	std::vector<std::string> EnemyEditor::ExtractAnimationNames(const std::string& gltfPath) {
		std::vector<std::string> names;
		if (gltfPath.empty()) return names;

		if (!fs::exists(gltfPath)) return names;

		std::string ext = fs::path(gltfPath).extension().string();
		// 拡張子を小文字に変換
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		json gltfJson;

		if (ext == ".gltf") {
			// .gltf: テキストJSONとしてパース
			std::ifstream ifs(gltfPath);
			if (!ifs.is_open()) return names;
			try {
				ifs >> gltfJson;
			} catch (...) {
				return names;
			}
		} else if (ext == ".glb") {
			std::ifstream ifs(gltfPath, std::ios::binary);
			if (!ifs.is_open()) return names;

			uint32_t magic = 0, version = 0, totalLength = 0;
			ifs.read(reinterpret_cast<char*>(&magic), 4);
			ifs.read(reinterpret_cast<char*>(&version), 4);
			ifs.read(reinterpret_cast<char*>(&totalLength), 4);

			if (magic != 0x46546C67) return names; // "glTF" マジックナンバー

			uint32_t chunkLength = 0, chunkType = 0;
			ifs.read(reinterpret_cast<char*>(&chunkLength), 4);
			ifs.read(reinterpret_cast<char*>(&chunkType), 4);

			if (chunkType != 0x4E4F534A) return names; // JSON チャンクでなければ中止

			std::string jsonStr(chunkLength, '\0');
			ifs.read(jsonStr.data(), chunkLength);

			try {
				gltfJson = json::parse(jsonStr);
			} catch (...) {
				return names;
			}
		} else {
			return names;
		}

		//"animations" 配列内の各要素の "name" を取得
		if (gltfJson.contains("animations") && gltfJson["animations"].is_array()) {
			for (size_t i = 0; i < gltfJson["animations"].size(); ++i) {
				const auto& anim = gltfJson["animations"][i];
				if (anim.contains("name") && anim["name"].is_string()) {
					names.push_back(anim["name"].get<std::string>());
				} else {
					names.push_back("Animation_" + std::to_string(i));
				}
			}
		}

		return names;
	}

	void EnemyEditor::ExtractMeshWireframe(const std::string& gltfPath) {
		cachedMeshPositions_.clear();
		cachedMeshEdges_.clear();
		cachedMeshGltfPath_ = gltfPath;

		if (gltfPath.empty() || !fs::exists(gltfPath)) return;

		std::string ext = fs::path(gltfPath).extension().string();
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		json gltfJson;
		std::vector<uint8_t> binData; // バイナリデータ

		if (ext == ".gltf") {
			std::ifstream ifs(gltfPath);
			if (!ifs.is_open()) return;
			try { ifs >> gltfJson; } catch (...) { return; }

			// .bin ファイルを探す
			if (gltfJson.contains("buffers") && gltfJson["buffers"].is_array() &&
				!gltfJson["buffers"].empty()) {
				auto& buf0 = gltfJson["buffers"][0];
				if (buf0.contains("uri") && buf0["uri"].is_string()) {
					fs::path parentPath = fs::path(gltfPath).parent_path();
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
			std::ifstream ifs(gltfPath, std::ios::binary);
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
					// 簡易パースのためクォータニオン(rotation)はここでは省略
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

		std::vector<std::pair<int, std::array<float, 16>>> meshInstances; // meshIdx, globalMat
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
			for (int i = 0; i < gltfJson["nodes"].size(); ++i) {
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
			if (meshIdx < 0 || meshIdx >= gltfJson["meshes"].size()) continue;
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
					// インデックスなし → 順番に並ぶ
					int numVerts = static_cast<int>(positions.size()) / 3;
					for (int i = 0; i < numVerts; ++i) indices.push_back(static_cast<uint32_t>(i));
				}

				// エッジ重複排除用: (a,b) を int64_t キーにエンコード
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
