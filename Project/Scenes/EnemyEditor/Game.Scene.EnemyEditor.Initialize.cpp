module Game.Editor.EnemyEditor;

import <fstream>;
import <filesystem>;
import <string>;

import nlohmann.json;

import Lumina.Utils.Data;

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Game::Editor {
	// JSON シリアライズ定義
	void to_json(json& j, const EnemyData& e) {
		j = json{
			{"name", e.name}, {"hp", e.hp}, {"power", e.power},
			{"gltfPath", e.gltfPath}, {"animationMap", e.animationMap}
		};
	}
	void from_json(const json& j, EnemyData& e) {
		if (j.contains("name")) j.at("name").get_to(e.name);
		if (j.contains("hp")) j.at("hp").get_to(e.hp);
		if (j.contains("power")) j.at("power").get_to(e.power);
		if (j.contains("gltfPath")) j.at("gltfPath").get_to(e.gltfPath);
		if (j.contains("animationMap")) j.at("animationMap").get_to(e.animationMap);
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
}
