module Game.EnemyManager;

import <fstream>;
import <filesystem>;
import <cmath>;
import <algorithm>;

import nlohmann.json;

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
	// EnemyData の JSON シリアライズ（EnemyEditor と同じ形式）
	void from_json(const json& j, Game::Editor::EnemyData& e) {
		if (j.contains("name")) j.at("name").get_to(e.name);
		if (j.contains("hp")) j.at("hp").get_to(e.hp);
		if (j.contains("power")) j.at("power").get_to(e.power);
		if (j.contains("gltfPath")) j.at("gltfPath").get_to(e.gltfPath);
		if (j.contains("animationMap")) j.at("animationMap").get_to(e.animationMap);
		if (j.contains("motionMap")) j.at("motionMap").get_to(e.motionMap);
		if (j.contains("collisionVertices") && j["collisionVertices"].is_array()) {
			e.collisionVertices.clear();
			for (const auto& vj : j["collisionVertices"]) {
				Game::Editor::CollisionVertex v;
				if (vj.contains("x")) v.x = vj["x"].get<float>();
				if (vj.contains("y")) v.y = vj["y"].get<float>();
				e.collisionVertices.push_back(v);
			}
		}
		if (j.contains("aggroRadius")) j.at("aggroRadius").get_to(e.aggroRadius);
		if (j.contains("attackRange")) j.at("attackRange").get_to(e.attackRange);
		if (j.contains("moveSpeed")) j.at("moveSpeed").get_to(e.moveSpeed);
		if (j.contains("attackCooldown")) j.at("attackCooldown").get_to(e.attackCooldown);
		if (j.contains("retreatThreshold")) j.at("retreatThreshold").get_to(e.retreatThreshold);
		if (j.contains("patrolRadius")) j.at("patrolRadius").get_to(e.patrolRadius);
		if (j.contains("aggressiveness")) j.at("aggressiveness").get_to(e.aggressiveness);
	}
}

namespace Game {

	// ============================
	//  シングルトン
	// ============================

	std::unique_ptr<EnemyManager> EnemyManager::instance_ = nullptr;

	EnemyManager* EnemyManager::GetInstance() {
		if (instance_ == nullptr) {
			instance_ = std::unique_ptr<EnemyManager>(new EnemyManager());
		}
		return instance_.get();
	}

	// ============================
	//  テンプレート管理
	// ============================

	void EnemyManager::LoadTemplates(const std::string& directoryPath) {
		if (!fs::exists(directoryPath)) return;

		for (const auto& entry : fs::directory_iterator(directoryPath)) {
			if (!entry.is_regular_file()) continue;
			if (entry.path().extension() != ".json") continue;

			// "area" で始まるファイルは除外（AreaEditor用）
			std::string stem = entry.path().stem().string();
			if (stem.find("area") == 0) continue;

			LoadTemplate(entry.path().string());
		}
	}

	void EnemyManager::LoadTemplate(const std::string& filePath) {
		std::ifstream file(filePath);
		if (!file.is_open()) return;

		try {
			json j;
			file >> j;
			Editor::EnemyData data;
			from_json(j, data);

			// テンプレート名は EnemyData.name を使う
			templates_[data.name] = data;
		}
		catch (...) {
			// パース失敗は無視
		}
	}

	const Editor::EnemyData* EnemyManager::GetTemplate(const std::string& name) const {
		auto it = templates_.find(name);
		if (it != templates_.end()) {
			return &it->second;
		}
		return nullptr;
	}

	std::vector<std::string> EnemyManager::GetTemplateNames() const {
		std::vector<std::string> names;
		names.reserve(templates_.size());
		for (const auto& [key, _] : templates_) {
			names.push_back(key);
		}
		return names;
	}

	void EnemyManager::ClearTemplates() {
		templates_.clear();
	}

	// ============================
	//  インスタンス管理
	// ============================

	uint32_t EnemyManager::GenerateId() {
		return nextId_++;
	}

	EnemyInstance* EnemyManager::Spawn(const std::string& templateName,
		const Lumina::Math::F32x3& position,
		bool facingRight) {
		const auto* tmpl = GetTemplate(templateName);
		if (!tmpl) return nullptr;

		return SpawnFromData(*tmpl, position, facingRight);
	}

	EnemyInstance* EnemyManager::SpawnFromData(const Editor::EnemyData& data,
		const Lumina::Math::F32x3& position,
		bool facingRight) {
		EnemyInstance inst;
		inst.baseData = data;
		inst.id = GenerateId();
		inst.position = position;
		inst.facingRight = facingRight;
		inst.InitFromBase();

		instances_.push_back(inst);
		return &instances_.back();
	}

	EnemyInstance* EnemyManager::GetInstance(uint32_t id) {
		for (auto& inst : instances_) {
			if (inst.id == id) return &inst;
		}
		return nullptr;
	}

	const EnemyInstance* EnemyManager::GetInstance(uint32_t id) const {
		for (const auto& inst : instances_) {
			if (inst.id == id) return &inst;
		}
		return nullptr;
	}

	std::vector<EnemyInstance*> EnemyManager::GetAliveInstances() {
		std::vector<EnemyInstance*> alive;
		for (auto& inst : instances_) {
			if (!inst.isDead) {
				alive.push_back(&inst);
			}
		}
		return alive;
	}

	int EnemyManager::GetAliveCount() const {
		int count = 0;
		for (const auto& inst : instances_) {
			if (!inst.isDead) ++count;
		}
		return count;
	}

	void EnemyManager::ClearInstances() {
		instances_.clear();
		nextId_ = 1;
	}

	void EnemyManager::RemoveDeadInstances() {
		instances_.erase(
			std::remove_if(instances_.begin(), instances_.end(),
				[](const EnemyInstance& inst) { return inst.isDead; }),
			instances_.end()
		);
	}

	// ============================
	//  更新
	// ============================

	void EnemyManager::Update(float deltaTime, const Lumina::Math::F32x3& playerPosition) {
		for (auto& enemy : instances_) {
			if (enemy.isDead) continue;

			// --- ハートタイマー更新 ---
			if (enemy.hurtTimer > 0.0f) {
				enemy.hurtTimer -= deltaTime;
			}

			// --- 攻撃クールダウン更新 ---
			if (enemy.attackCooldownTimer > 0.0f) {
				enemy.attackCooldownTimer -= deltaTime;
			}

			// --- プレイヤーとの距離計算 ---
			float dx = playerPosition.X - enemy.position.X;
			float dy = playerPosition.Y - enemy.position.Y;
			float dist = std::sqrt(dx * dx + dy * dy);

			// --- 撤退判定 ---
			float hpRatio = (enemy.baseData.hp > 0)
				? static_cast<float>(enemy.currentHP) / enemy.baseData.hp
				: 1.0f;

			if (enemy.baseData.retreatThreshold > 0.0f && hpRatio <= enemy.baseData.retreatThreshold) {
				enemy.aiState = EnemyInstance::AIState::Retreat;
			}

			// --- AI 状態遷移 ---
			switch (enemy.aiState) {
			case EnemyInstance::AIState::Idle:
				enemy.currentAction = "Idle";
				// 索敵範囲にプレイヤーが入った場合
				if (dist < enemy.baseData.aggroRadius) {
					if (enemy.baseData.aggressiveness > 0.0f) {
						enemy.aiState = EnemyInstance::AIState::Chase;
					}
				}
				break;

			case EnemyInstance::AIState::Patrol:
				enemy.currentAction = "Walk";
				// パトロール中にプレイヤーを発見
				if (dist < enemy.baseData.aggroRadius) {
					enemy.aiState = EnemyInstance::AIState::Chase;
				}
				break;

			case EnemyInstance::AIState::Chase:
				enemy.currentAction = "Walk";
				// 攻撃範囲に入ったら攻撃へ
				if (dist <= enemy.baseData.attackRange) {
					enemy.aiState = EnemyInstance::AIState::Attack;
				}
				// 索敵範囲外に出たら Idle に戻る
				else if (dist > enemy.baseData.aggroRadius * 1.5f) {
					enemy.aiState = EnemyInstance::AIState::Idle;
				}
				// 追跡移動
				else {
					float moveDir = (dx > 0.0f) ? 1.0f : -1.0f;
					enemy.position.X += moveDir * enemy.baseData.moveSpeed * deltaTime;
					enemy.facingRight = (dx > 0.0f);
				}
				break;

			case EnemyInstance::AIState::Attack:
				enemy.currentAction = "Attack";
				// 攻撃範囲外に出たら追跡に戻る
				if (dist > enemy.baseData.attackRange * 1.2f) {
					enemy.aiState = EnemyInstance::AIState::Chase;
				}
				break;

			case EnemyInstance::AIState::Retreat:
				enemy.currentAction = "Walk";
				// プレイヤーと反対方向に逃げる
				{
					float retreatDir = (dx > 0.0f) ? -1.0f : 1.0f;
					enemy.position.X += retreatDir * enemy.baseData.moveSpeed * 1.5f * deltaTime;
					enemy.facingRight = (retreatDir > 0.0f);
				}
				// HP が回復したら（将来の拡張）あるいは十分離れたら Idle に戻る
				if (dist > enemy.baseData.aggroRadius * 2.0f) {
					enemy.aiState = EnemyInstance::AIState::Idle;
				}
				break;
			}

			// --- 状態タイマー更新 ---
			enemy.stateTimer += deltaTime;
		}
	}

	// ============================
	//  ダメージ・インタラクション
	// ============================

	bool EnemyManager::DealDamage(uint32_t enemyId, int damage) {
		EnemyInstance* enemy = GetInstance(enemyId);
		if (!enemy || enemy->isDead) return false;

		enemy->currentHP -= damage;
		enemy->hurtTimer = 0.2f;

		if (enemy->currentHP <= 0) {
			enemy->currentHP = 0;
			enemy->isDead = true;
			if (onDeathCallback_) {
				onDeathCallback_(*enemy);
			}
			return true;
		}
		return false;
	}

	int EnemyManager::DealAreaDamage(const Lumina::Math::F32x3& origin, float radius,
		int damage, bool facingRight, bool directional) {
		int killCount = 0;

		for (auto& enemy : instances_) {
			if (enemy.isDead) continue;

			float dx = enemy.position.X - origin.X;
			float dy = enemy.position.Y - origin.Y;
			float dist = std::sqrt(dx * dx + dy * dy);

			if (dist > radius) continue;

			// 方向制限チェック
			if (directional) {
				if (facingRight && dx < -radius * 0.3f) continue;   // 右向きなのに左側の敵
				if (!facingRight && dx > radius * 0.3f) continue;   // 左向きなのに右側の敵
			}

			enemy.currentHP -= damage;
			enemy.hurtTimer = 0.2f;

			if (enemy.currentHP <= 0) {
				enemy.currentHP = 0;
				enemy.isDead = true;
				if (onDeathCallback_) {
					onDeathCallback_(enemy);
				}
				++killCount;
			}
		}

		return killCount;
	}

	// ============================
	//  コールバック
	// ============================

	void EnemyManager::SetOnEnemyDeathCallback(OnEnemyDeathCallback callback) {
		onDeathCallback_ = std::move(callback);
	}
}
