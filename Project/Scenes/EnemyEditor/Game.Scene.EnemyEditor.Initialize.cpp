module Game.Editor.EnemyEditor;

import <fstream>;
import <filesystem>;
import <string>;

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
}
