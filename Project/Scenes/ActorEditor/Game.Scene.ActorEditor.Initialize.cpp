module Game.Editor.ActorEditor;

import <fstream>;
import <filesystem>;
import <string>;

import nlohmann.json;
import Game.MotionManager;

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

	void to_json(json& j, const ActorCollider& c) {
		j = json{
			{"type", static_cast<int>(c.type)},
			{"sizeX", c.sizeX}, {"sizeY", c.sizeY}, {"sizeZ", c.sizeZ}
		};
	}
	void from_json(const json& j, ActorCollider& c) {
		if (j.contains("type")) c.type = static_cast<ColliderType>(j.at("type").get<int>());
		if (j.contains("sizeX")) j.at("sizeX").get_to(c.sizeX);
		if (j.contains("sizeY")) j.at("sizeY").get_to(c.sizeY);
		if (j.contains("sizeZ")) j.at("sizeZ").get_to(c.sizeZ);
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
}
