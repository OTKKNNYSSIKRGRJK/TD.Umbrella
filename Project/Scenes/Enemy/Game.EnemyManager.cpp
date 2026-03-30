module Game.EnemyManager;

import <fstream>;
import <filesystem>;
import <cmath>;
import <algorithm>;
import <array>;

import nlohmann.json;
import Game.MathUtils;

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
	using Vector2 = std::pair<float, float>;

	// 2D外積 (p1-p0) x (p2-p0)
	float Cross2D(const Vector2& p0, const Vector2& p1, const Vector2& p2) {
		return (p1.first - p0.first) * (p2.second - p0.second)
		     - (p1.second - p0.second) * (p2.first - p0.first);
	}

	// ポリゴンが凸かどうかを判定
	bool IsConvexPolygon(const std::vector<Game::Editor::CollisionVertex>& verts) {
		if (verts.size() < 3) return false;
		int n = static_cast<int>(verts.size());
		bool hasPositive = false, hasNegative = false;
		for (int i = 0; i < n; ++i) {
			int j = (i + 1) % n;
			int k = (i + 2) % n;
			float cross = Cross2D(
				{ verts[i].x, verts[i].y },
				{ verts[j].x, verts[j].y },
				{ verts[k].x, verts[k].y }
			);
			if (cross > 0.0f) hasPositive = true;
			if (cross < 0.0f) hasNegative = true;
			if (hasPositive && hasNegative) return false;
		}
		return true;
	}

	// 点が三角形の内部にあるか判定 (Barycentric)
	bool PointInTriangle(const Vector2& p, const Vector2& a, const Vector2& b, const Vector2& c) {
		float d1 = Cross2D(a, b, p);
		float d2 = Cross2D(b, c, p);
		float d3 = Cross2D(c, a, p);
		bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
		bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
		return !(hasNeg && hasPos);
	}

	// ポリゴンの面積符号 (正=CCW, 負=CW)
	float PolygonSignedArea(const std::vector<Vector2>& poly) {
		float area = 0.0f;
		int n = static_cast<int>(poly.size());
		for (int i = 0; i < n; ++i) {
			int j = (i + 1) % n;
			area += poly[i].first * poly[j].second;
			area -= poly[j].first * poly[i].second;
		}
		return area * 0.5f;
	}

	// Ear Clipping 三角形分割
	// 入力: 2D頂点列（単純多角形）
	// 出力: 三角形のインデックス列 (i0,i1,i2, i0,i1,i2, ...)
	std::vector<std::array<int, 3>> TriangulateEarClipping(
		const std::vector<Game::Editor::CollisionVertex>& inputVerts)
	{
		std::vector<std::array<int, 3>> triangles;
		int n = static_cast<int>(inputVerts.size());
		if (n < 3) return triangles;

		// 作業用インデックスリスト
		std::vector<int> indices(n);
		// CCW になるように順序を決定
		std::vector<Vector2> poly(n);
		for (int i = 0; i < n; ++i) {
			poly[i] = { inputVerts[i].x, inputVerts[i].y };
		}

		if (PolygonSignedArea(poly) > 0.0f) {
			// CCW
			for (int i = 0; i < n; ++i) indices[i] = i;
		} else {
			// CW → 反転してCCWに
			for (int i = 0; i < n; ++i) indices[i] = (n - 1) - i;
		}

		int remaining = n;
		int failCount = 0;

		while (remaining > 3) {
			bool earFound = false;
			for (int i = 0; i < remaining; ++i) {
				int prev = (i + remaining - 1) % remaining;
				int next = (i + 1) % remaining;

				Vector2 a = poly[indices[prev]];
				Vector2 b = poly[indices[i]];
				Vector2 c = poly[indices[next]];

				// 凸頂点か？ (CCW前提なのでcross > 0 が凸)
				if (Cross2D(a, b, c) <= 0.0f) continue;

				// 他の頂点が三角形内にないか？
				bool isEar = true;
				for (int j = 0; j < remaining; ++j) {
					if (j == prev || j == i || j == next) continue;
					if (PointInTriangle(poly[indices[j]], a, b, c)) {
						isEar = false;
						break;
					}
				}

				if (isEar) {
					triangles.push_back({ indices[prev], indices[i], indices[next] });
					indices.erase(indices.begin() + i);
					--remaining;
					earFound = true;
					failCount = 0;
					break;
				}
			}

			if (!earFound) {
				++failCount;
				if (failCount > remaining) break; // 無限ループ防止
			}
		}

		// 残りの3頂点
		if (remaining == 3) {
			triangles.push_back({ indices[0], indices[1], indices[2] });
		}

		return triangles;
	}

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
	//  コライダー初期化（凸包分割対応）
	// ============================

	void EnemyInstance::InitCollider() {
		colliders.clear();
		if (baseData.collisionVertices.size() < 3) return;

		constexpr float kDepth = 0.1f;

		auto makeCollider = [&](const std::vector<Lumina::Math::F32x3>& verts3d) {
			auto col = std::make_unique<ConvexCollider>();
			col->SetMyType(COL_Enemy);
			col->SetYourType(COL_Player | COL_Player_Attack);
			col->SetUserData(this);
			col->SetVertices(verts3d);
			col->SetWorldPosition(position);

			col->onCollisionCallback = [this](Collider* other, const Lumina::Math::F32x3& pushOut) {
				if (other->GetMyType() == COL_Ground || other->GetMyType() == COL_Player) {
					position.X += (-pushOut.X);
					position.Y += (-pushOut.Y);
					position.Z += (-pushOut.Z);
				}
				// Player Attack takes damage handling elsewhere or could be handled here
			};

			col->UpdateAABB();
			colliders.push_back(std::move(col));
		};

		if (IsConvexPolygon(baseData.collisionVertices)) {
			// 凸多角形 → そのまま1つのコライダー
			std::vector<Lumina::Math::F32x3> verts3d;
			verts3d.reserve(baseData.collisionVertices.size() * 2);
			for (const auto& v : baseData.collisionVertices) {
				verts3d.push_back({ v.x, v.y, -kDepth });
				verts3d.push_back({ v.x, v.y,  kDepth });
			}
			makeCollider(verts3d);
		} else {
			// 凹多角形 → Ear Clipping で三角形に分割
			auto triangles = TriangulateEarClipping(baseData.collisionVertices);
			for (const auto& tri : triangles) {
				std::vector<Lumina::Math::F32x3> verts3d;
				verts3d.reserve(6); // 三角形3頂点 × 前後2面
				for (int idx : tri) {
					const auto& v = baseData.collisionVertices[idx];
					verts3d.push_back({ v.x, v.y, -kDepth });
					verts3d.push_back({ v.x, v.y,  kDepth });
				}
				makeCollider(verts3d);
			}
		}
	}

	void EnemyInstance::UpdateCollider() {
		Lumina::Math::F32x3 scale{ 1.0f, 1.0f, 1.0f };
		Lumina::Math::F32x3 rot{ 0.0f, 0.0f, 0.0f };
		if (!facingRight) {
			rot.Y = 3.14159265f; // rotate 180 degrees
		}
		auto worldMat = Game::MathUtils::SRT(scale, rot, position);

		for (auto& col : colliders) {
			col->SetWorldPosition(position);
			col->SetWorldMatrix(worldMat);
			col->UpdateAABB();
		}
	}

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
		inst.InitCollider();

		instances_.push_back(std::move(inst));
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

	void EnemyManager::RegisterCollidersTo(CollisionManager& cm) {
		for (auto& enemy : instances_) {
			if (enemy.isDead) continue;
			for (auto& col : enemy.colliders) {
				cm.SetColliders(col.get());
			}
		}
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

			// --- コライダー位置更新 ---
			enemy.UpdateCollider();
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
