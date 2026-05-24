module Game.Editor.EnemyEditor;

import <fstream>;
import <filesystem>;
import <string>;
import <sstream>;
import <map>;

import nlohmann.json;

import Lumina.Utils.Data;
import Lumina.CG3D;
import Lumina.Core.Math;
import Lumina.Core.String;

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

	void to_json(json& j, const SizeTier& t) {
		j = json{ {"hp", t.hp}, {"power", t.power}, {"scale", t.scale} };
	}
	void from_json(const json& j, SizeTier& t) {
		if (j.contains("hp")) j.at("hp").get_to(t.hp);
		if (j.contains("power")) j.at("power").get_to(t.power);
		if (j.contains("scale")) j.at("scale").get_to(t.scale);
	}

	void to_json(json& j, const Node& n) {
		j = json{
			{"id", n.id}, {"name", n.name}, {"state", n.state}, {"x", n.x}, {"y", n.y},
			{"animationName", n.animationName},
			{"boundMotion", n.boundMotion},
			{"boundMotionNodeIndex", n.boundMotionNodeIndex},
			{"boundBool", n.boundBool},
			{"facePlayer", n.facePlayer},
            {"loop", n.loop},
			{"loopCooldown", n.loopCooldown},
			{"requireGrounded", n.requireGrounded},
			{"proceduralPitch", n.proceduralPitch},
			{"velocityFrictionX", n.velocityFrictionX},
         {"jumpVelocityXMult", n.jumpVelocityXMult},
			{"jumpVelocityY", n.jumpVelocityY},
			{"prepScale", { {"start", n.prepScale.start}, {"peak", n.prepScale.peak}, {"duration", n.prepScale.duration} }},
       {"prepOffset", { {"x", n.prepOffset.x}, {"y", n.prepOffset.y} }},
		{"prepYaw", n.prepYaw},
		{"prepHold", n.prepHold},
		{"prepSound", n.prepSound},
		{"prepParticle", n.prepParticle},
			{"splineMotionName", n.splineMotionName},
			{"splineDuration", n.splineDuration},
			{"isAttack", n.isAttack},
			{"damageMultiplier", n.damageMultiplier}
		};
	}
	void from_json(const json& j, Node& n) {
		if (j.contains("id")) j.at("id").get_to(n.id);
		if (j.contains("name")) j.at("name").get_to(n.name);
		if (j.contains("state")) j.at("state").get_to(n.state);
		if (j.contains("x")) j.at("x").get_to(n.x);
		if (j.contains("y")) j.at("y").get_to(n.y);
		if (j.contains("animationName")) j.at("animationName").get_to(n.animationName);
		if (j.contains("boundMotion")) j.at("boundMotion").get_to(n.boundMotion);
		if (j.contains("boundMotionNodeIndex")) j.at("boundMotionNodeIndex").get_to(n.boundMotionNodeIndex);
		if (j.contains("boundBool")) j.at("boundBool").get_to(n.boundBool);
		if (j.contains("facePlayer")) j.at("facePlayer").get_to(n.facePlayer);

        		if (j.contains("loop")) j.at("loop").get_to(n.loop);
		if (j.contains("loopCooldown")) j.at("loopCooldown").get_to(n.loopCooldown);
		if (j.contains("requireGrounded")) j.at("requireGrounded").get_to(n.requireGrounded);
		if (j.contains("proceduralPitch")) j.at("proceduralPitch").get_to(n.proceduralPitch);
		if (j.contains("velocityFrictionX")) j.at("velocityFrictionX").get_to(n.velocityFrictionX);
		if (j.contains("jumpVelocityXMult")) j.at("jumpVelocityXMult").get_to(n.jumpVelocityXMult);
		if (j.contains("jumpVelocityY")) j.at("jumpVelocityY").get_to(n.jumpVelocityY);

        if (j.contains("isAttack")) j.at("isAttack").get_to(n.isAttack);
        if (j.contains("damageMultiplier")) j.at("damageMultiplier").get_to(n.damageMultiplier);

		if (j.contains("prepScale") && j["prepScale"].is_object()) {
			auto const & ps = j["prepScale"];
			if (ps.contains("start")) ps.at("start").get_to(n.prepScale.start);
			if (ps.contains("peak")) ps.at("peak").get_to(n.prepScale.peak);
			if (ps.contains("duration")) ps.at("duration").get_to(n.prepScale.duration);
		}
        if (j.contains("prepOffset") && j["prepOffset"].is_object()) {
			auto const & po = j["prepOffset"];
			if (po.contains("x")) po.at("x").get_to(n.prepOffset.x);
			if (po.contains("y")) po.at("y").get_to(n.prepOffset.y);
		}
		if (j.contains("prepYaw")) j.at("prepYaw").get_to(n.prepYaw);
		if (j.contains("prepHold")) j.at("prepHold").get_to(n.prepHold);
		if (j.contains("prepSound")) j.at("prepSound").get_to(n.prepSound);
		if (j.contains("prepParticle")) j.at("prepParticle").get_to(n.prepParticle);
		if (j.contains("splineMotionName")) j.at("splineMotionName").get_to(n.splineMotionName);
		if (j.contains("splineDuration")) j.at("splineDuration").get_to(n.splineDuration);
		if (j.contains("requireGrounded")) j.at("requireGrounded").get_to(n.requireGrounded);

		// migration: if user previously put "BOOL:Attack" in animationName
		if (n.boundBool.empty() && n.animationName.rfind("BOOL:", 0) == 0) {
			n.boundBool = n.animationName.substr(5);
			n.animationName.clear();
		}
	}
	
	void to_json(json& j, const Link& l) {
		j = json{ {"from", l.from}, {"to", l.to}, {"condition", l.condition} };
	}
	void from_json(const json& j, Link& l) {
		if (j.contains("from")) j.at("from").get_to(l.from);
		if (j.contains("to")) j.at("to").get_to(l.to);
		if (j.contains("condition")) j.at("condition").get_to(l.condition);
	}

	// JSON シリアライズ定義
	void to_json(json& j, const EnemyData& e) {
		j = json{
			{"name", e.name}, {"hp", e.hp}, {"power", e.power},
			{"gltfPath", e.gltfPath},
			{"sizeTiers", e.sizeTiers},
			{"animationMap", e.animationMap},
			{"motionMap", e.motionMap},
			{"collisionVertices", e.collisionVertices},
			{"aggroRadius", e.aggroRadius}, {"attackRange", e.attackRange},
			{"moveSpeed", e.moveSpeed}, {"attackCooldown", e.attackCooldown},
			{"retreatThreshold", e.retreatThreshold},
			{"patrolRadius", e.patrolRadius}, {"aggressiveness", e.aggressiveness},
           {"attackType", (e.attackType == EnemyData::AttackType::Ranged) ? "Ranged" : "Melee"},
			{"noSplit", e.noSplit},
			{"nodes", e.nodes},
			{"links", e.links},
		};

		// プロジェクタイル設定（遠距離攻撃時のみ有効だが常に保存）
		j["projectile"] = json{
			{"actorName", e.projectile.actorName},
			{"isHoming", e.projectile.isHoming},
			{"homingStrength", e.projectile.homingStrength},
			{"damage", e.projectile.damage},
			{"lifetime", e.projectile.lifetime},
			{"colliderRadius", e.projectile.colliderRadius},
		};
	}
	void from_json(const json& j, EnemyData& e) {
		if (j.contains("name")) j.at("name").get_to(e.name);
		if (j.contains("hp")) j.at("hp").get_to(e.hp);
		if (j.contains("power")) j.at("power").get_to(e.power);
		if (j.contains("gltfPath")) j.at("gltfPath").get_to(e.gltfPath);
		if (j.contains("sizeTiers") && j["sizeTiers"].is_array() && j["sizeTiers"].size() == 3) {
			j.at("sizeTiers").get_to(e.sizeTiers);
		}
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

		if (j.contains("attackType")) {
			std::string atype = j["attackType"].get<std::string>();
			e.attackType = (atype == "Ranged") ? EnemyData::AttackType::Ranged : EnemyData::AttackType::Melee;
		}

		if (j.contains("projectile") && j["projectile"].is_object()) {
			const auto& pj = j["projectile"];
			// 新形式: actorName ベース
			if (pj.contains("actorName")) pj.at("actorName").get_to(e.projectile.actorName);
			if (pj.contains("isHoming")) pj.at("isHoming").get_to(e.projectile.isHoming);
			// 旧形式の後方互換: trajectory が "Homing" なら isHoming を true に
			if (pj.contains("trajectory") && !pj.contains("isHoming")) {
				std::string traj = pj["trajectory"].get<std::string>();
				e.projectile.isHoming = (traj == "Homing");
			}
			if (pj.contains("homingStrength")) pj.at("homingStrength").get_to(e.projectile.homingStrength);
			if (pj.contains("damage")) pj.at("damage").get_to(e.projectile.damage);
			if (pj.contains("lifetime")) pj.at("lifetime").get_to(e.projectile.lifetime);
			if (pj.contains("colliderRadius")) pj.at("colliderRadius").get_to(e.projectile.colliderRadius);
		}

		if (j.contains("nodes")) j.at("nodes").get_to(e.nodes);
		if (j.contains("links")) j.at("links").get_to(e.links);
       if (j.contains("noSplit")) j.at("noSplit").get_to(e.noSplit);
	}

	bool EvaluateLinkCondition(const std::string& condition, const LinkEvalContext& ctx) {
		if (condition.empty()) return true;

		// 前後の空白を除去したコピーで比較
		std::string c = condition;
		while (!c.empty() && c.front() == ' ') c.erase(c.begin());
		while (!c.empty() && c.back() == ' ') c.pop_back();

		if (c == "Always") return true;

		// --- BOOL: フラグ条件 ---
		if (c.rfind("BOOL:", 0) == 0 || c.rfind("!BOOL:", 0) == 0) {
			bool negate = (c.rfind("!BOOL:", 0) == 0);
			std::string flag = c.substr(negate ? 6 : 5);
			if (!negate && !flag.empty() && flag.front() == '!') {
				negate = true;
				flag = flag.substr(1);
			}
			while (!flag.empty() && flag.front() == ' ') flag.erase(flag.begin());
			while (!flag.empty() && flag.back() == ' ') flag.pop_back();
			if (ctx.boolFlags) {
				auto it = ctx.boolFlags->find(flag);
				if (it != ctx.boolFlags->end()) return negate ? !it->second : it->second;
			}
			return negate ? true : false;
		}

		// --- 時間条件 ---
		if (c.rfind("Time>=", 0) == 0) { try { return ctx.stateElapsedTime >= std::stof(c.substr(6)); } catch (...) { return false; } }
		if (c.rfind("Time>", 0) == 0)  { try { return ctx.stateElapsedTime >  std::stof(c.substr(5)); } catch (...) { return false; } }

		// --- 距離条件 ---
		if (c.rfind("Dist<=", 0) == 0) { try { return ctx.distToPlayer <= std::stof(c.substr(6)); } catch (...) { return false; } }
		if (c.rfind("Dist>", 0) == 0)  { try { return ctx.distToPlayer >  std::stof(c.substr(5)); } catch (...) { return false; } }

		// --- HP条件 ---
		if (c.rfind("HP<=", 0) == 0) { try { return ctx.hpRatio <= std::stof(c.substr(4)); } catch (...) { return false; } }
		if (c.rfind("HP<", 0) == 0)  { try { return ctx.hpRatio <  std::stof(c.substr(3)); } catch (...) { return false; } }
		if (c.rfind("HP>=", 0) == 0) { try { return ctx.hpRatio >= std::stof(c.substr(4)); } catch (...) { return false; } }
		if (c.rfind("HP>", 0) == 0)  { try { return ctx.hpRatio >  std::stof(c.substr(3)); } catch (...) { return false; } }
		if (c.rfind("HP==", 0) == 0) { try { return std::abs(ctx.hpRatio - std::stof(c.substr(4))) < 0.001f; } catch (...) { return false; } }

		// --- 接地条件 ---
		if (c == "Grounded")  return ctx.isGrounded;
		if (c == "!Grounded") return !ctx.isGrounded;

		return false;
	}

	void EnemyEditor::Initialize() {
	}

	void EnemyActionEditor::Initialize() {
	}

	void EnemyEditor::SaveEnemy(const EnemyData& enemy) {
		fs::create_directories("Assets/Data/Enemy");
		std::string filename = "Assets/Data/Enemy/" + enemy.name + ".json";
		std::ofstream file(filename);
		if (file.is_open()) {
			json j = enemy;
			file << j.dump(4);
		}
	}

	void EnemyEditor::LoadEnemy(EnemyData& enemy, const std::string& filename) {
		std::string fullPath = "Assets/Data/Enemy/" + filename;
		std::ifstream file(fullPath);
		if (file.is_open()) {
			try {
				json j;
				file >> j;
				enemy = j.get<EnemyData>();
			} catch (...) {
				// JSONパースエラー時はログを出すか無視する
				// 敵データ以外のjson（vcpkg.json等）を読み込んだときのクラッシュを防ぐ
			}
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

	void EnemyActionEditor::SaveEnemy(const EnemyData& enemy) {
    fs::create_directories("Assets/Data/Enemy");
    std::string filename = enemy.name + ".json";
    std::string fullPath = "Assets/Data/Enemy/" + filename;
    std::ofstream file(fullPath);
    if (file.is_open()) {
        json j = enemy;
        file << j.dump(4);
    }

    // persist into per-file storage so runtime state is kept per JSON
    perFileEnemies_[filename] = enemy;
    PerFileRuntime rt;
    rt.currentStateId = currentStateId_;
    rt.currentStateElapsedTime = currentStateElapsedTime_;
    rt.previousStateId = previousStateId_;
    rt.transitionFlashTimer = transitionFlashTimer_;
    rt.firstNodeStarted = firstNodeStarted_;
    rt.undoStack = undoStack_;
    rt.cachedAnimationNames = cachedAnimationNames_;
    rt.runtimeBoolFlags = runtimeBoolFlags_;
    perFileRuntimes_[filename] = std::move(rt);
    activeFileName_ = filename;
	}

	void EnemyActionEditor::LoadEnemy(EnemyData& enemy, const std::string& filename) {
    std::string fullPath = "Assets/Data/Enemy/" + filename;
    std::ifstream file(fullPath);
    EnemyData loaded;
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            loaded = j.get<EnemyData>();
        } catch (...) {
        }
    }

    // store into per-file map
    std::string fname = filename;
    perFileEnemies_[fname] = loaded;

    // restore runtime state if exists
    if (perFileRuntimes_.find(fname) != perFileRuntimes_.end()) {
        auto& rt = perFileRuntimes_[fname];
        currentStateId_ = rt.currentStateId;
        currentStateElapsedTime_ = rt.currentStateElapsedTime;
        previousStateId_ = rt.previousStateId;
        transitionFlashTimer_ = rt.transitionFlashTimer;
        firstNodeStarted_ = rt.firstNodeStarted;
        undoStack_ = rt.undoStack;
        cachedAnimationNames_ = rt.cachedAnimationNames;
        runtimeBoolFlags_ = rt.runtimeBoolFlags;
    } else {
        // initialize runtime for this file
        PerFileRuntime rt;
        rt.currentStateId = -1;
        rt.currentStateElapsedTime = 0.0f;
        rt.previousStateId = -1;
        rt.transitionFlashTimer = 0.0f;
        rt.firstNodeStarted = false;
        rt.cachedAnimationNames = ExtractAnimationNames(loaded.gltfPath);
        rt.runtimeBoolFlags.clear();
        perFileRuntimes_[fname] = rt;
        currentStateId_ = -1;
        currentStateElapsedTime_ = 0.0f;
        previousStateId_ = -1;
        transitionFlashTimer_ = 0.0f;
        firstNodeStarted_ = false;
        cachedAnimationNames_ = perFileRuntimes_[fname].cachedAnimationNames;
        runtimeBoolFlags_.clear();
    }

    // set editing enemy reference
    enemy = loaded;
    editingEnemy_ = loaded;
    activeFileName_ = fname;
	}

	std::vector<std::string> EnemyActionEditor::ExtractAnimationNames(const std::string& gltfPath) {
		std::vector<std::string> names;
		if (gltfPath.empty()) return names;

		if (!fs::exists(gltfPath)) return names;

		std::string ext = fs::path(gltfPath).extension().string();
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		json gltfJson;

		if (ext == ".gltf") {
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

			if (magic != 0x46546C67) return names; // "glTF"

			uint32_t chunkLength = 0, chunkType = 0;
			ifs.read(reinterpret_cast<char*>(&chunkLength), 4);
			ifs.read(reinterpret_cast<char*>(&chunkType), 4);

			if (chunkType != 0x4E4F534A) return names;

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
		cachedMeshFaces_.clear();
		cachedMeshGltfPath_ = gltfPath;

		posedMeshPositions_.clear();
		vertexWeightsCache_.clear();
		cachedAnimations_.clear();
		cachedSkeleton_ = Lumina::CG3D::Skeleton{};

		if (gltfPath.empty() || !fs::exists(gltfPath)) return;

		std::string ext = fs::path(gltfPath).extension().string();
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		json gltfJson;
		std::vector<uint8_t> binData; // バイナリデータ

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
			return;
		}

		if (ext == ".gltf" || ext == ".glb") {
			fs::path path(gltfPath);
			std::string fileName = path.filename().string();
			std::string dirPath = path.parent_path().string();

			try {
				Lumina::String luminaFileName(fileName.c_str());
				Lumina::String luminaDirPath(dirPath.c_str());
				auto collection = Lumina::CG3D::Import(fileName, dirPath);
				cachedSkeleton_ = Lumina::CG3D::CreateSkeleton(collection.Root);
				cachedAnimations_ = Lumina::CG3D::LoadAnimationFile(luminaFileName, luminaDirPath);
				
				for (const auto& mesh : collection.Meshes) {
					int baseVertex = static_cast<int>(cachedMeshPositions_.size());
					
					for (const auto& v : mesh.Vertices) {
						cachedMeshPositions_.push_back({ v.Position.X, v.Position.Y, v.Position.Z });
					}
					
					for (size_t i = 0; i + 2 < mesh.Indices.size(); i += 3) {
						int v0 = baseVertex + mesh.Indices[i];
						int v1 = baseVertex + mesh.Indices[i+1];
						int v2 = baseVertex + mesh.Indices[i+2];
						cachedMeshFaces_.push_back({ v0, v1, v2 });
						
						cachedMeshEdges_.push_back({ v0, v1 });
						cachedMeshEdges_.push_back({ v1, v2 });
						cachedMeshEdges_.push_back({ v2, v0 });
					}
					
					for (const auto& [jointName, weightData] : mesh.SkinClusterData) {
						auto it = cachedSkeleton_.IDX_Joint.find(jointName);
						if (it != cachedSkeleton_.IDX_Joint.end()) {
							int jointIndex = it->second;
							for (const auto& vw : weightData.VertexWeights) {
								int globalVertIdx = baseVertex + vw.VertexID;
								if (globalVertIdx >= vertexWeightsCache_.size()) {
									vertexWeightsCache_.resize(globalVertIdx + 1);
								}
								vertexWeightsCache_[globalVertIdx].push_back({ jointIndex, vw.Weight });
							}
						}
					}
				}
				
				posedMeshPositions_ = cachedMeshPositions_;
				
				invBindPoses_.resize(cachedSkeleton_.ARR_Joint.size());
				currentJointMatrices_.resize(cachedSkeleton_.ARR_Joint.size());
				for (size_t i = 0; i < cachedSkeleton_.ARR_Joint.size(); ++i) {
					invBindPoses_[i] = Lumina::Math::F32x4x4<>::Identity;
					currentJointMatrices_[i] = Lumina::Math::F32x4x4<>::Identity;
				}
				for (const auto& mesh : collection.Meshes) {
					for (const auto& [jointName, weightData] : mesh.SkinClusterData) {
						auto it = cachedSkeleton_.IDX_Joint.find(jointName);
						if (it != cachedSkeleton_.IDX_Joint.end()) {
							int jointIndex = it->second;
							invBindPoses_[jointIndex] = weightData.INV_BindPose;
						}
					}
				}
			} catch (...) {
				// Failed to load via CG3D, ignore
			}
		}
	}
}
