module Game.Editor.AreaEditor;

import <vector>;
import <string>;
import <fstream>;
import <filesystem>;
import <algorithm>;
import <map>;

import nlohmann.json;

import Lumina.Utils.Data;

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace Game::Editor {
	// JSON シリアライズ / デシリアライズ定義
	void to_json(json& j, const Vector2& v) {
		j = json{ {"x", v.x}, {"y", v.y} };
	}
	void from_json(const json& j, Vector2& v) {
		j.at("x").get_to(v.x);
		j.at("y").get_to(v.y);
	}

	void to_json(json& j, const RectCollision& r) {
		j = json{ {"position", r.position}, {"size", r.size} };
	}
	void from_json(const json& j, RectCollision& r) {
		j.at("position").get_to(r.position);
		j.at("size").get_to(r.size);
	}

	void to_json(json& j, const AreaConnection& c) {
		j = json{ {"targetAreaIndex", c.targetAreaIndex}, {"position", c.position} };
	}
	void from_json(const json& j, AreaConnection& c) {
		j.at("targetAreaIndex").get_to(c.targetAreaIndex);
		if (j.contains("position")) {
			j.at("position").get_to(c.position);
		} else if (j.contains("trigger") && j.at("trigger").contains("position")) {
			j.at("trigger").at("position").get_to(c.position);
		}
	}

	void to_json(json& j, const EnemyPlacement& e) {
		j = json{ {"enemyName", e.enemyName}, {"position", e.position}, {"facingRight", e.facingRight}, {"sizeCategory", e.sizeCategory} };
	}
	void from_json(const json& j, EnemyPlacement& e) {
		if (j.contains("enemyName")) j.at("enemyName").get_to(e.enemyName);
		if (j.contains("position")) j.at("position").get_to(e.position);
		if (j.contains("facingRight")) j.at("facingRight").get_to(e.facingRight);
		if (j.contains("sizeCategory")) j.at("sizeCategory").get_to(e.sizeCategory);
	}

	void to_json(json& j, const CollisionPoint& p) {
		j = json{ {"position", p.position}, {"radius", p.radius} };
	}
	void from_json(const json& j, CollisionPoint& p) {
		if (j.contains("position")) j.at("position").get_to(p.position);
		if (j.contains("radius")) j.at("radius").get_to(p.radius);
	}

	void to_json(json& j, const CollisionGroup& cg) {
		j = json{ {"name", cg.name}, {"points", cg.points} };
	}
	void from_json(const json& j, CollisionGroup& cg) {
		if (j.contains("name")) j.at("name").get_to(cg.name);
		if (j.contains("points")) j.at("points").get_to(cg.points);
	}

	void to_json(json& j, const AreaData& a) {
		j = a.originalJson;
		j["name"] = a.name;
		j["index"] = a.index;
		j["width"] = a.width;
		j["height"] = a.height;
		j["backgroundMusic"] = a.backgroundMusic;
		j["connections"] = a.connections;
		j["enemies"] = a.enemies;
		j["collisionGroups"] = a.collisionGroups;
		j["editorPos"] = a.editorPos;
	}

	void from_json(const json& j, AreaData& a) {
		if (j.is_object()) {
			a.originalJson = j;
		} else {
			a.originalJson = json::object();
		}

		if (j.contains("name")) {
			if (j.at("name").is_string()) {
				std::string n = j.at("name").get<std::string>();
				try { a.name = std::stoi(n); } catch (...) { a.name = 0; }
			} else {
				j.at("name").get_to(a.name);
			}
		} else {
			a.name = 0;
		}

		if (j.contains("index")) {
			j.at("index").get_to(a.index);
		}

		a.name = a.index;

		if (j.contains("width")) j.at("width").get_to(a.width);
		if (j.contains("height")) j.at("height").get_to(a.height);
		if (j.contains("backgroundMusic")) j.at("backgroundMusic").get_to(a.backgroundMusic);
		if (j.contains("connections")) j.at("connections").get_to(a.connections);
		if (j.contains("enemies")) j.at("enemies").get_to(a.enemies);
		if (j.contains("collisionGroups")) j.at("collisionGroups").get_to(a.collisionGroups);
		if (j.contains("editorPos")) j.at("editorPos").get_to(a.editorPos);
	}

	namespace {
		void SaveAreaFile(const AreaData& area, std::vector<std::string>& recentFiles, std::vector<AreaData>& allAreas) {
			fs::create_directories("Assets/Data/Terrain");
			std::string filename = "area" + std::to_string(area.name) + ".json";
			std::ofstream file("Assets/Data/Terrain/" + filename);
			if (!file.is_open()) {
				return;
			}

			json j = area;
			file << j.dump(4);

			bool found = false;
			for (auto& f : recentFiles) {
				if (f == filename) {
					found = true;
					break;
				}
			}
			if (!found) recentFiles.push_back(filename);

			bool foundArea = false;
			for (auto& a : allAreas) {
				if (a.name == area.name) {
					a = area;
					foundArea = true;
					break;
				}
			}
			if (!foundArea) allAreas.push_back(area);
		}
	}

	void AreaEditor::Initialize() {
		recentFiles_.clear();
		allAreas_.clear();
		enemyFiles_.clear();
		editingArea_.Reset();

		bool firstLoaded = false;
		if (fs::exists("Assets/Data/Terrain")) {
			for (const auto& entry : fs::directory_iterator("Assets/Data/Terrain")) {
				std::string fName = entry.path().filename().string();
				if (entry.path().extension() == ".json" && fName.find("area") == 0) {
					recentFiles_.push_back(fName);
					AreaData a;
					LoadArea(a, fName);
					allAreas_.push_back(a);

					if (!firstLoaded && fName == "area0.json") {
						editingArea_ = a;
						firstLoaded = true;
					}
				}
			}
		}

		if (!firstLoaded && !allAreas_.empty()) {
			editingArea_ = allAreas_[0];
		}
	}

	void AreaEditor::SaveArea(const AreaData& area_in, bool autoSyncConnections) {
		AreaData area = area_in;
		area.name = area.index;

		std::vector<int> affectedTargets;
		if (autoSyncConnections) {
			for (const auto& a : allAreas_) {
				if (a.name == area.name) {
					for (const auto& oldConn : a.connections) {
						int oldTarget = oldConn.targetAreaIndex;
						if (oldTarget != area.index && std::find(affectedTargets.begin(), affectedTargets.end(), oldTarget) == affectedTargets.end()) {
							affectedTargets.push_back(oldTarget);
						}
					}
					break;
				}
			}
			for (const auto& newConn : area.connections) {
				int newTarget = newConn.targetAreaIndex;
				if (newTarget != area.index && std::find(affectedTargets.begin(), affectedTargets.end(), newTarget) == affectedTargets.end()) {
					affectedTargets.push_back(newTarget);
				}
			}
			for (const auto& otherArea : allAreas_) {
				if (otherArea.name == area.name) {
					continue;
				}
				for (const auto& conn : otherArea.connections) {
					if (conn.targetAreaIndex == area.index) {
						if (std::find(affectedTargets.begin(), affectedTargets.end(), otherArea.index) == affectedTargets.end()) {
							affectedTargets.push_back(otherArea.index);
						}
						break;
					}
				}
			}
		}

		SaveAreaFile(area, recentFiles_, allAreas_);

		if (!autoSyncConnections) {
			return;
		}

		for (int targetId : affectedTargets) {
			auto it = std::find_if(allAreas_.begin(), allAreas_.end(),
				[targetId](const AreaData& a) { return a.name == targetId; });

			if (it == allAreas_.end()) {
				continue;
			}

			int expectedCount = 0;
			for (const auto& c : area.connections) {
				if (c.targetAreaIndex == targetId) {
					expectedCount++;
				}
			}

			AreaData targetArea = *it;
			std::vector<AreaConnection> preservedConnections;
			std::vector<AreaConnection> reciprocalConnections;
			for (const auto& conn : targetArea.connections) {
				if (conn.targetAreaIndex == area.index) {
					reciprocalConnections.push_back(conn);
				} else {
					preservedConnections.push_back(conn);
				}
			}

			if (static_cast<int>(reciprocalConnections.size()) < expectedCount) {
				for (int i = static_cast<int>(reciprocalConnections.size()); i < expectedCount; ++i) {
					AreaConnection newConn;
					newConn.targetAreaIndex = area.index;
					newConn.position = {
						targetArea.width / 2.0f - 16.0f + static_cast<float>(i) * 32.0f,
						targetArea.height / 2.0f - 16.0f
					};
					reciprocalConnections.push_back(newConn);
				}
			} else if (static_cast<int>(reciprocalConnections.size()) > expectedCount) {
				reciprocalConnections.resize(expectedCount);
			}

			preservedConnections.insert(preservedConnections.end(), reciprocalConnections.begin(), reciprocalConnections.end());
			targetArea.connections = std::move(preservedConnections);
			SaveAreaFile(targetArea, recentFiles_, allAreas_);
		}
	}

	void AreaEditor::LoadArea(AreaData& area, const std::string& filename) {
		std::string path = "Assets/Data/Terrain/" + filename;
		std::ifstream file(path);
		if (file.is_open()) {
			json j;
			file >> j;
			area = j.get<AreaData>();
		}
	}

	void AreaEditor::DeleteArea(int areaIndex) {
		std::string filename = "area" + std::to_string(areaIndex) + ".json";
		std::string path = "Assets/Data/Terrain/" + filename;
		if (fs::exists(path)) {
			fs::remove(path);
		}

		auto it = std::remove(recentFiles_.begin(), recentFiles_.end(), filename);
		recentFiles_.erase(it, recentFiles_.end());

		std::vector<AreaData> areasToUpdate;
		for (auto& a : allAreas_) {
			if (a.name == areaIndex) continue;
			auto eraseIt = std::remove_if(a.connections.begin(), a.connections.end(),
				[areaIndex](const AreaConnection& c) { return c.targetAreaIndex == areaIndex; });
			if (eraseIt != a.connections.end()) {
				a.connections.erase(eraseIt, a.connections.end());
				areasToUpdate.push_back(a);
			}
		}

		for (const auto& a : areasToUpdate) {
			SaveArea(a, false);
		}

		auto itArea = std::remove_if(allAreas_.begin(), allAreas_.end(),
			[areaIndex](const AreaData& a) { return a.name == areaIndex; });
		allAreas_.erase(itArea, allAreas_.end());

		if (editingArea_.name == areaIndex) {
			editingArea_.Reset();
		}
	}
}
