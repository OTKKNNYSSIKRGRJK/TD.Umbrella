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
			{"gltfPath", e.gltfPath}, {"animationMap", e.animationMap},
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
}
