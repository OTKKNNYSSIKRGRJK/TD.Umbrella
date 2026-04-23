module Game.Editor.ObjMotionEditor;

import <fstream>;
import <filesystem>;
import <string>;
import <sstream>;

import nlohmann.json;
import Lumina.Utils.Data;

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Game::Editor {
	void to_json(json& j, const ObjMotionKeyframe& k) {
		j = json{
			{"time", k.time},
			{"px", k.px}, {"py", k.py}, {"pz", k.pz},
			{"rx", k.rx}, {"ry", k.ry}, {"rz", k.rz},
			{"sx", k.sx}, {"sy", k.sy}, {"sz", k.sz}
		};
	}
	void from_json(const json& j, ObjMotionKeyframe& k) {
		if (j.contains("time")) j.at("time").get_to(k.time);
		if (j.contains("px")) j.at("px").get_to(k.px);
		if (j.contains("py")) j.at("py").get_to(k.py);
		if (j.contains("pz")) j.at("pz").get_to(k.pz);
		if (j.contains("rx")) j.at("rx").get_to(k.rx);
		if (j.contains("ry")) j.at("ry").get_to(k.ry);
		if (j.contains("rz")) j.at("rz").get_to(k.rz);
		if (j.contains("sx")) j.at("sx").get_to(k.sx);
		if (j.contains("sy")) j.at("sy").get_to(k.sy);
		if (j.contains("sz")) j.at("sz").get_to(k.sz);
	}

	void to_json(json& j, const ObjMotionData& m) {
		j = json{
			{"name", m.name},
			{"duration", m.duration},
			{"isLoop", m.isLoop},
			{"targetModelPath", m.targetModelPath},
			{"keyframes", m.keyframes}
		};
	}
	void from_json(const json& j, ObjMotionData& m) {
		if (j.contains("name")) j.at("name").get_to(m.name);
		if (j.contains("duration")) j.at("duration").get_to(m.duration);
		if (j.contains("isLoop")) j.at("isLoop").get_to(m.isLoop);
		if (j.contains("targetModelPath")) j.at("targetModelPath").get_to(m.targetModelPath);
		if (j.contains("keyframes")) j.at("keyframes").get_to(m.keyframes);
	}

	void ObjMotionEditor::Initialize() {
		editingMotion_.Reset();
	}

	void ObjMotionEditor::SaveMotion(const ObjMotionData& motion) {
		// Output to Assets/Data/Motion directly? Or let's just save to current directory and they can move it or we save it to standard location.
		// As enemy editor did `./`, let's just save to motion name + `.json`. Wait, EnemyEditor saves to `./name.json`.
		// Data is saved in the working directory? Let's check EnemyEditor. update: it just saves to `filename.json`. Let's save `motion.name + ".json"`.
		// But in EnemyEditor Update it scans `Assets/Data/Motion/` for motions! So we should save it there.
		std::string dir = "Assets/Data/objMotion/";
		if (!fs::exists(dir)) {
			fs::create_directories(dir);
		}
		std::string filename = dir + motion.name + ".json";
		std::ofstream file(filename);
		if (file.is_open()) {
			json j = motion;
			file << j.dump(4);
		}
	}

	void ObjMotionEditor::LoadMotion(ObjMotionData& motion, const std::string& filename) {
		std::ifstream file(filename);
		if (file.is_open()) {
			try {
				json j;
				file >> j;
				motion = j.get<ObjMotionData>();
			} catch (...) {}
		}
	}

	void ObjMotionEditor::LoadEnemy(const std::string& filename) {
		std::ifstream file(filename);
		if (file.is_open()) {
			try {
				json j;
				file >> j;
				
				// 手動でEnemyDataをパースする（EnemyManagerのfrom_jsonを使用するか、最低限必要なものを自前で抽出）
				cachedEnemyData_.name = j.value("name", "Unknown");
				cachedEnemyData_.gltfPath = j.value("gltfPath", "");
				
				cachedEnemyData_.collisionVertices.clear();
				if (j.contains("collisionVertices") && j["collisionVertices"].is_array()) {
					for (const auto& vj : j["collisionVertices"]) {
						CollisionVertex v;
						v.x = vj.value("x", 0.0f);
						v.y = vj.value("y", 0.0f);
						cachedEnemyData_.collisionVertices.push_back(v);
					}
				}

				cachedEnemyData_.motionMap.clear();
				if (j.contains("motionMap") && j["motionMap"].is_object()) {
					for (auto it = j["motionMap"].begin(); it != j["motionMap"].end(); ++it) {
						cachedEnemyData_.motionMap[it.key()] = it.value().get<std::string>();
					}
				}

				editingEnemyFile_ = filename;
				currentActionName_ = "";
				
				ExtractMeshWireframe(cachedEnemyData_.gltfPath);
			} catch (...) {}
		}
	}

	void ObjMotionEditor::ExtractMeshWireframe(const std::string& gltfPath) {
		cachedMeshPositions_.clear();
		cachedMeshEdges_.clear();
		cachedMeshFaces_.clear();
		cachedMeshObjPath_ = gltfPath;

		if (gltfPath.empty() || !fs::exists(gltfPath)) return;

		std::string ext = fs::path(gltfPath).extension().string();
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		if (ext == ".obj") {
			std::ifstream ifs(gltfPath);
			if (!ifs.is_open()) return;
			std::string line;
			while (std::getline(ifs, line)) {
				// Remove leading whitespaces
				size_t startPos = line.find_first_not_of(" \t");
				if (startPos == std::string::npos) continue;
				line = line.substr(startPos);
				
				if (line.compare(0, 2, "v ") == 0) {
					std::istringstream iss(line.substr(2));
					float x, y, z;
					if (iss >> x >> y >> z) {
						cachedMeshPositions_.push_back({x, y, z});
					}
				} else if (line.compare(0, 2, "f ") == 0) {
					std::istringstream iss(line.substr(2));
					std::string token;
					std::vector<int> faceVerts;
					while (iss >> token) {
						size_t slashPos = token.find('/');
						int vIdx = 0;
						try {
							if (slashPos != std::string::npos) {
								vIdx = std::stoi(token.substr(0, slashPos));
							} else {
								vIdx = std::stoi(token);
							}
						} catch(...) { continue; }
						if (vIdx > 0) faceVerts.push_back(vIdx - 1);
					}
					for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
						cachedMeshFaces_.push_back({ faceVerts[0], faceVerts[i], faceVerts[i+1] });
						cachedMeshEdges_.push_back({ faceVerts[0], faceVerts[i] });
						cachedMeshEdges_.push_back({ faceVerts[i], faceVerts[i+1] });
						cachedMeshEdges_.push_back({ faceVerts[i+1], faceVerts[0] });
					}
				}
			}
		}
	}
}
