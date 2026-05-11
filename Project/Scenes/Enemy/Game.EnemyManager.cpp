module Game.EnemyManager;

import <fstream>;
import <filesystem>;
import <cmath>;
import <algorithm>;
import <array>;
import <random>;

import nlohmann.json;
import Game.MathUtils;
import Game.Player;
import Game.Umbrella;
import Game.ProjectileManager;

import Game.Events;

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
	using Vector2 = std::pair<float, float>;
	constexpr int kMinEnemySizeTier = 0;
	constexpr int kMaxEnemySizeTier = 2;
 constexpr float kTurnedFacingYaw = 3.14159265f;
	constexpr float kEnemyFacingTurnSpeed = 8.0f;
	constexpr int kSplitChildCount = 2;
	constexpr float kSplitHorizontalVelocity = 1.2f;
	constexpr float kSplitVerticalVelocity = 2.5f;
	constexpr float kSplitSpawnInvulnerability = 0.15f;
	constexpr float kEnemyHpScale = 0.45f;
	constexpr float kLargeAttackWindup = 0.75f;
	constexpr float kMediumAttackWindup = 0.45f;
	constexpr float kSmallAttackWindup = 0.2f;
    constexpr float kLargeBurstSpeed = 3.4f;
	constexpr float kMediumBurstSpeed = 1.9f;
   constexpr float kSmallBurstSpeed = 1.0f;
	constexpr float kSmallStrafeAmplitude = 1.8f;
	constexpr float kLargeLandingStunDuration = 0.3f;
	constexpr float kLargeLandingImpactThreshold = 3.0f;

	int GetScaledEnemyHp(int baseHp) {
     if (baseHp < 0) {
			return baseHp;
		}
		return (std::max)(1, static_cast<int>(std::round(static_cast<float>(baseHp) * kEnemyHpScale)));
	}

	bool HasInvulnerableHpSetting(const Game::Editor::EnemyData& data) {
		if (data.hp < 0) {
			return true;
		}

		return std::any_of(
			data.sizeTiers.cbegin(),
			data.sizeTiers.cend(),
			[](const auto& tier) {
				return tier.hp < 0;
			}
		);
	}

	void ConfigureEnemyBehaviorBySize(Game::EnemyInstance& enemy) {
		switch (enemy.sizeTier) {
		case 2:
			enemy.attackWindupDuration = kLargeAttackWindup;
			enemy.attackDuration = 0.5f;
			enemy.burstSpeedMultiplier = kLargeBurstSpeed;
			enemy.preferredCombatDistance = enemy.baseData.attackRange * 0.9f;
			break;
		case 1:
			enemy.attackWindupDuration = kMediumAttackWindup;
			enemy.attackDuration = 0.35f;
			enemy.burstSpeedMultiplier = kMediumBurstSpeed;
			enemy.preferredCombatDistance = enemy.baseData.attackRange;
			break;
		default:
			enemy.attackWindupDuration = kSmallAttackWindup;
			enemy.attackDuration = 0.2f;
			enemy.burstSpeedMultiplier = kSmallBurstSpeed;
			enemy.preferredCombatDistance = enemy.baseData.attackRange * 1.3f;
			break;
		}
	}

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
		const std::vector<Game::Editor::CollisionVertex>& inputVerts) {
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
	void from_json(const json& j, Game::Editor::Node& n) {
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
		if (j.contains("velocityFrictionX")) j.at("velocityFrictionX").get_to(n.velocityFrictionX);
		if (j.contains("jumpVelocityXMult")) j.at("jumpVelocityXMult").get_to(n.jumpVelocityXMult);
		if (j.contains("jumpVelocityY")) j.at("jumpVelocityY").get_to(n.jumpVelocityY);
		if (j.contains("splineMotionName")) j.at("splineMotionName").get_to(n.splineMotionName);
		if (j.contains("splineDuration")) j.at("splineDuration").get_to(n.splineDuration);

		// Backwards-compat migration:
		// Older editor versions stored a node-level boolean trigger in `animationName`
		// as the literal string "BOOL:Name". On load we convert that into the
		// dedicated `boundBool` field so runtime can evaluate it independently of
		// `boundMotion`/`boundMotionNodeIndex`.
		if (n.boundBool.empty() && n.animationName.rfind("BOOL:", 0) == 0) {
			n.boundBool = n.animationName.substr(5);
			n.animationName.clear();
		}
	}

	void from_json(const json& j, Game::Editor::Link& l) {
		if (j.contains("from")) j.at("from").get_to(l.from);
		if (j.contains("to")) j.at("to").get_to(l.to);
		if (j.contains("condition")) j.at("condition").get_to(l.condition);
	}

	void from_json(const json& j, Game::Editor::EnemyData& e) {
		if (j.contains("name")) j.at("name").get_to(e.name);
		if (j.contains("hp")) j.at("hp").get_to(e.hp);
		if (j.contains("power")) j.at("power").get_to(e.power);
		if (j.contains("gltfPath")) j.at("gltfPath").get_to(e.gltfPath);
		if (j.contains("sizeTiers") && j["sizeTiers"].is_array()) {
			size_t count = (std::min)(e.sizeTiers.size(), j["sizeTiers"].size());
			for (size_t i = 0; i < count; ++i) {
				const auto& tj = j["sizeTiers"][i];
				if (tj.contains("hp")) tj.at("hp").get_to(e.sizeTiers[i].hp);
				if (tj.contains("power")) tj.at("power").get_to(e.sizeTiers[i].power);
				if (tj.contains("scale")) tj.at("scale").get_to(e.sizeTiers[i].scale);
			}
		}
		if (j.contains("animationMap")) j.at("animationMap").get_to(e.animationMap);
		if (j.contains("motionMap")) j.at("motionMap").get_to(e.motionMap);
		if (j.contains("collisionVertices") && j["collisionVertices"].is_array()) {
			e.collisionVertices.clear();
			for (const auto& vj : j["collisionVertices"]) {
				Game::Editor::CollisionVertex v;
				if (vj.contains("x")) vj.at("x").get_to(v.x);
				if (vj.contains("y")) vj.at("y").get_to(v.y);
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

		if (j.contains("attackType")) {
			std::string atype;
			j.at("attackType").get_to(atype);
			e.attackType = (atype == "Ranged")
				? Game::Editor::EnemyData::AttackType::Ranged
				: Game::Editor::EnemyData::AttackType::Melee;
		}

		if (j.contains("projectile") && j["projectile"].is_object()) {
			const auto& pj = j["projectile"];
			if (pj.contains("actorName")) pj.at("actorName").get_to(e.projectile.actorName);
			if (pj.contains("isHoming")) pj.at("isHoming").get_to(e.projectile.isHoming);
			if (pj.contains("trajectory") && !pj.contains("isHoming")) {
				std::string traj;
				pj.at("trajectory").get_to(traj);
				e.projectile.isHoming = (traj == "Homing");
			}
			if (pj.contains("homingStrength")) pj.at("homingStrength").get_to(e.projectile.homingStrength);
			if (pj.contains("damage")) pj.at("damage").get_to(e.projectile.damage);
			if (pj.contains("lifetime")) pj.at("lifetime").get_to(e.projectile.lifetime);
			if (pj.contains("colliderRadius")) pj.at("colliderRadius").get_to(e.projectile.colliderRadius);
		}

		if (j.contains("nodes") && j["nodes"].is_array()) {
			e.nodes.clear();
			for (const auto& nodeJson : j["nodes"]) {
				Game::Editor::Node node;
				from_json(nodeJson, node);
				e.nodes.push_back(std::move(node));
			}
		}

		if (j.contains("links") && j["links"].is_array()) {
			e.links.clear();
			for (const auto& linkJson : j["links"]) {
				Game::Editor::Link link;
				from_json(linkJson, link);
				e.links.push_back(std::move(link));
			}
		}
	}

	void SpawnSplitChildren(
		Game::EnemyManager& manager,
		const Game::Editor::EnemyData& data,
		const Lumina::Math::F32x3& position,
		const Lumina::Math::F32x3& parentVelocity,
		bool facingRight,
		int parentSizeTier) {
        if (HasInvulnerableHpSetting(data)) return;
		if (parentSizeTier <= kMinEnemySizeTier) return;

		int childSizeTier = parentSizeTier - 1;
		float spawnOffset = (std::max)(0.3f, data.sizeTiers[childSizeTier].scale * 1.5f);

		for (int i = 0; i < kSplitChildCount; ++i) {
			float dir = (i == 0) ? -1.0f : 1.0f;
			Lumina::Math::F32x3 childPos = position;
			childPos.X += dir * spawnOffset;
			childPos.Y += 0.05f;

			auto* child = manager.SpawnFromData(
				data,
				childPos,
				(dir > 0.0f) ? true : false,
				data.sizeTiers[childSizeTier].scale,
				childSizeTier
			);
			if (!child) continue;

			child->velocity = parentVelocity;
			child->velocity.X = dir * kSplitHorizontalVelocity;
			child->velocity.Y = (std::max)(parentVelocity.Y, kSplitVerticalVelocity);
			child->facingRight = (dir > 0.0f) ? true : facingRight;
			child->hurtTimer = kSplitSpawnInvulnerability;
			child->recentlyDamagedThisFrame = true;
			child->UpdateCollider();
		}
	}
}

namespace Game {
	std::unique_ptr<EnemyBehavior> CreateEnemyBehavior(const Editor::EnemyData& data) {
		if (data.name == "KingSlime") {
			return std::make_unique<KingSlimeBehavior>();
		}
		return std::make_unique<EnemyBehavior>();
	}

	void KingSlimeBehavior::OnSpawn(EnemyInstance& enemy) {
		enemy.currentAction = "Idle";
		walk_ = false;
		followPhase_ = FollowPhase::None;
	}

	void KingSlimeBehavior::Update(EnemyInstance& enemy, float deltaTime, const Lumina::Math::F32x3& playerPosition) {
		// Detect when the state machine enters a node with boundBool == "followAbove"
		// by checking the current node's boundBool
		const Game::Editor::Node* currentNode = nullptr;
		for (const auto& n : enemy.baseData.nodes) {
			if (n.state == enemy.currentAction) {
				currentNode = &n;
				break;
			}
		}

		// --- Jump cooldown: accumulate combat time and set jumpReady when due ---
		if (followPhase_ == FollowPhase::None) {
			jumpCooldownTimer_ += deltaTime;
			if (jumpCooldownTimer_ >= jumpCooldownInterval_) {
				enemy.runtimeBoolFlags["jumpReady"] = true;
			}
		}

		// --- Activate follow-above when entering JumpUp state ---
		if (currentNode && currentNode->boundBool == "followAbove" && followPhase_ == FollowPhase::None) {
			followPhase_ = FollowPhase::Rising;
			enemy.runtimeBoolFlags["followAboveDone"] = false;
			enemy.runtimeBoolFlags["jumpReady"] = false;
			jumpCooldownTimer_ = 0.0f;
			// Store start position and compute target for lerp
			riseStartPos_ = enemy.position;
			riseTargetPos_ = { playerPosition.X, playerPosition.Y + hoverHeight_, enemy.position.Z };
			riseTimer_ = 0.0f;
		}

		// --- Follow-above phase handling ---
		switch (followPhase_) {
		case FollowPhase::Rising:
			// Update target every frame so it follows the moving player
			riseTargetPos_.X = playerPosition.X;
			riseTargetPos_.Y = playerPosition.Y + hoverHeight_;
			// Linear interpolation from start to target position
			riseTimer_ += deltaTime;
			{
				float t = (std::min)(riseTimer_ / riseDuration_, 1.0f);
				enemy.position.X = riseStartPos_.X + (riseTargetPos_.X - riseStartPos_.X) * t;
				enemy.position.Y = riseStartPos_.Y + (riseTargetPos_.Y - riseStartPos_.Y) * t;
				enemy.position.Z = riseStartPos_.Z;
				// Override velocity to prevent gravity interference
				enemy.velocity.X = 0.0f;
				enemy.velocity.Y = 0.0f;
			}
			if (riseTimer_ >= riseDuration_) {
				followPhase_ = FollowPhase::Tracking;
				followTimer_ = followDuration_;
				lastTrackedX_ = playerPosition.X;
			}
			break;

		case FollowPhase::Tracking:
			// Track player X while hovering at fixed height, invisible (off-screen)
			lastTrackedX_ = playerPosition.X;
			enemy.position.X = playerPosition.X;
			enemy.position.Y = playerPosition.Y + hoverHeight_;
			enemy.velocity.X = 0.0f;
			enemy.velocity.Y = 0.0f;
			followTimer_ -= deltaTime;
			if (followTimer_ <= 0.0f) {
				// Start dropping
				followPhase_ = FollowPhase::Dropping;
				enemy.position.X = lastTrackedX_;
				enemy.velocity.Y = dropSpeed_;
				enemy.velocity.X = 0.0f;
			}
			break;

		case FollowPhase::Dropping:

			{
				float expectedGroundedVelY = -9.8f * deltaTime;
				bool isGrounded = (enemy.velocity.Y >= expectedGroundedVelY - 0.5f) && (enemy.velocity.Y <= 0.0f);
				if (isGrounded && enemy.position.Y < playerPosition.Y + hoverHeight_) {
					followPhase_ = FollowPhase::None;
					enemy.runtimeBoolFlags["followAboveDone"] = true;
				}
			}
			break;

		case FollowPhase::None:
		default:
			break;
		}

		// Walk flag: set when current node has boundBool == "walk" or "Walk"
		if (currentNode) {
			walk_ = (currentNode->boundBool == "walk" || currentNode->boundBool == "Walk");
		}
	}

	bool KingSlimeBehavior::IsWalkActive() const {
		return walk_;
	}

	bool KingSlimeBehavior::IsMotionPlaying() const {
		return followPhase_ != FollowPhase::None;
	}

	int KingSlimeBehavior::GetActiveNodeIndex() const {
		return -1;
	}

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
			col->SetYourType(COL_Player | COL_Player_Attack | COL_Ground | COL_Player_Attack_Smash);
			col->SetUserData(this);
			col->SetVertices(verts3d);
			col->SetWorldPosition(position);

			Lumina::Math::F32x3 scale{ 1.0f, 1.0f, 1.0f };
			Lumina::Math::F32x3 rot{ 0.0f, 0.0f, 0.0f };
			if (!facingRight) {
				rot.Y = 3.14159265f; // 反転
			}
			auto initWorld = Game::MathUtils::SRT(scale, rot, position);
			col->SetWorldMatrix(initWorld);


			col->onCollisionCallback = [this](Collider* other, const Lumina::Math::F32x3& pushOut) {
				if (other->GetMyType() == COL_Ground) {
					float verticalImpactSpeed = -this->velocity.Y;
					Lumina::Math::F32x3 actualPush = { -pushOut.X, -pushOut.Y, -pushOut.Z };
					position.X += actualPush.X;
					position.Y += actualPush.Y;
					position.Z += actualPush.Z;

					Lumina::Math::F32x3 normal = actualPush;
					float length = std::sqrt(normal.X * normal.X + normal.Y * normal.Y + normal.Z * normal.Z);
					if (length > 0.0f) {
						normal.X /= length;
						normal.Y /= length;
						normal.Z /= length;
					}

					// 足元に地面があるかのチェック (Playerを参考)
					if (normal.Y > 0.8f) {
						if (this->velocity.Y <= 0.0f) {
							if (this->velocity.Y < 0.0f) {
								this->velocity.Y = 0.0f;
							}
							if (this->sizeTier == kMaxEnemySizeTier && verticalImpactSpeed >= kLargeLandingImpactThreshold) {
								this->landingStunTimer = kLargeLandingStunDuration;
								this->velocity.X *= 0.2f;
							}
						}
					} else if (std::abs(normal.X) > 0.3f && normal.Y > -0.2f) {
						// 壁や急な斜面に直面している場合、ジャンプして凹みを乗り越える
						bool isBlockedForward = (this->facingRight && normal.X < 0.0f) || (!this->facingRight && normal.X > 0.0f);
						// 大幅に落下中でなければジャンプ（穴から抜け出す）
						if (isBlockedForward && this->velocity.Y >= -2.0f && this->velocity.Y <= 1.0f) {
							this->velocity.Y = 6.5f; // 脱出用ジャンプ
						}
					}
				}
           else if (other->GetMyType() == COL_Player) {
				Player* player = static_cast<Player*>(other->GetUserData());
				if (player != nullptr) {
					// Do not damage player for tutorial/invulnerable-configured enemies
					if (!HasInvulnerableHpSetting(this->baseData)) {
						player->GetStatusComponent().TakeDamage((std::max)(0.25f, this->baseData.power));
					}
				}
			}
				else if (other->GetMyType() == COL_Player_Attack) {
                        if (this->hurtTimer <= 0.0f && !this->recentlyDamagedThisFrame) {
							this->recentlyDamagedThisFrame = true;
							// Do not apply horizontal knockback on player attack; only apply damage.
							Umbrella::Top* umbrellaTop = static_cast<Umbrella::Top*>(other->GetUserData());
							Game::EnemyManager::GetInstance()->DealDamage(this->id, (int)umbrellaTop->GetStatusComponent().GetAttack());
						}
				}
				else if (other->GetMyType() == COL_Player_Attack_Smash) {
					Player* player = static_cast<Player*>(other->GetUserData());
					Game::EnemyManager::GetInstance()->DealDamage(this->id, (int)(player->GetUmbrella().top_->GetStatusComponent().GetAttack()));
				}
			};

			col->UpdateAABB();
			colliders.emplace_back(std::move(col));
			};

		if (IsConvexPolygon(baseData.collisionVertices)) {
			// 凸多角形 → そのまま1つのコライダー
			std::vector<Lumina::Math::F32x3> verts3d;
			verts3d.reserve(baseData.collisionVertices.size() * 2);
			for (const auto& v : baseData.collisionVertices) {
				verts3d.push_back({ v.x * modelScale, v.y * modelScale, -kDepth * modelScale });
				verts3d.push_back({ v.x * modelScale, v.y * modelScale,  kDepth * modelScale });
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
					verts3d.push_back({ v.x * modelScale, v.y * modelScale, -kDepth * modelScale });
					verts3d.push_back({ v.x * modelScale, v.y * modelScale,  kDepth * modelScale });
				}
				makeCollider(verts3d);
			}
		}
	}

	void EnemyInstance::UpdateCollider() {
		Lumina::Math::F32x3 scale{ modelScale, modelScale, modelScale };
		Lumina::Math::F32x3 rot{ 0.0f, 0.0f, 0.0f };
		if (!facingRight) {
			rot.Y = 3.14159265f; // rotate 180 degrees
		}
		auto worldMat = Game::MathUtils::SRT(scale, rot, position);

		for (auto& col : colliders) {
			position.Z = 0.0f; // Zは常に0
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

			// Persist migration of legacy "BOOL:..." stored in animationName into
			// explicit "boundBool" fields so editor/runtime do not rely on the
			// legacy format. If any node was migrated, update the JSON on disk.
			bool jsonChanged = false;
			if (j.contains("nodes") && j["nodes"].is_array()) {
				for (size_t i = 0; i < data.nodes.size() && i < j["nodes"].size(); ++i) {
					const auto& node = data.nodes[i];
					json& nodeJson = j["nodes"][i];
					// If boundBool was produced by migration, ensure it's written.
					if (!node.boundBool.empty()) {
						if (!nodeJson.contains("boundBool") || nodeJson["boundBool"].get<std::string>() != node.boundBool) {
							nodeJson["boundBool"] = node.boundBool;
							jsonChanged = true;
						}
						// Clear legacy animationName if it contained BOOL: prefix
						if (nodeJson.contains("animationName") && nodeJson["animationName"].is_string()) {
							std::string anim = nodeJson["animationName"].get<std::string>();
							if (anim.rfind("BOOL:", 0) == 0) {
								nodeJson["animationName"] = "";
								jsonChanged = true;
							}
						}
					}
				}
				if (jsonChanged) {
					// write back changes to the same file (best-effort)
					try {
						std::ofstream ofs(filePath, std::ios::trunc);
						if (ofs.is_open()) {
							ofs << j.dump(4);
						}
					} catch (...) {
						// ignore write errors
					}
				}
			}

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
		bool facingRight, float scale) {
		const auto* tmpl = GetTemplate(templateName);
		if (!tmpl) return nullptr;

		return SpawnFromData(*tmpl, position, facingRight, scale);
	}

	EnemyInstance* EnemyManager::SpawnFromData(const Editor::EnemyData& data,
		const Lumina::Math::F32x3& position,
		bool facingRight, float scale, int sizeTier) {
		EnemyInstance inst;
		inst.baseData = data;
		inst.behavior = CreateEnemyBehavior(inst.baseData);
		for (auto& tier : inst.baseData.sizeTiers) {
			tier.hp = GetScaledEnemyHp(tier.hp);
		}
		inst.baseData.hp = GetScaledEnemyHp(inst.baseData.hp);
		inst.id = GenerateId();
		inst.position = position;
		inst.facingRight = facingRight;
       inst.renderFacingYaw = facingRight ? 0.0f : kTurnedFacingYaw;
		inst.sizeTier = sizeTier;
		inst.InitFromBase();
		ConfigureEnemyBehaviorBySize(inst);
		if (scale > 0.0f) {
			inst.modelScale = scale;
		}
		if (inst.behavior) {
			inst.behavior->OnSpawn(inst);
		}
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
		// reset per-frame damage guard
		for (auto& enemy : instances_) {
			enemy.recentlyDamagedThisFrame = false;
		}

		for (auto& enemy : instances_) {
			if (enemy.isDead) continue;

			Lumina::Math::F32x3 posBeforePhysics = enemy.position;

			// --- 物理挙動（重力） ---
			enemy.velocity.Y -= 9.8f * deltaTime;
			enemy.position.Y += enemy.velocity.Y * deltaTime;
			enemy.position.X += enemy.velocity.X * deltaTime;
			enemy.position.Z += enemy.velocity.Z * deltaTime;

			// --- ハートタイマー更新 ---
			if (enemy.hurtTimer > 0.0f) {
				enemy.hurtTimer -= deltaTime;
			}

			// --- 攻撃クールダウン更新 ---
			if (enemy.attackCooldownTimer > 0.0f) {
				enemy.attackCooldownTimer -= deltaTime;
			}
			if (enemy.preAttackTimer > 0.0f) {
				enemy.preAttackTimer -= deltaTime;
			}
			if (enemy.attackTimer > 0.0f) {
				enemy.attackTimer -= deltaTime;
			}
			if (enemy.landingStunTimer > 0.0f) {
				enemy.landingStunTimer -= deltaTime;
			}

			// --- プレイヤーとの距離計算 ---
			float dx = playerPosition.X - enemy.position.X;
			float dy = playerPosition.Y - enemy.position.Y;
			float dist = std::sqrt(dx * dx + dy * dy);

			if (enemy.landingStunTimer > 0.0f) {
				enemy.currentAction = "Idle";
				enemy.velocity.X *= 0.75f;
				enemy.stateTimer += deltaTime;
				enemy.UpdateCollider();
				continue;
			}

			// --- 撤退判定 ---
			float hpRatio = (enemy.baseData.hp > 0)
				? static_cast<float>(enemy.currentHP) / enemy.baseData.hp
				: 1.0f;

			if (enemy.baseData.retreatThreshold > 0.0f && hpRatio <= enemy.baseData.retreatThreshold) {
				enemy.aiState = EnemyInstance::AIState::Retreat;
			}

			// --- AI 状態遷移 ---
			// ノード（JSONステート）が定義されている場合はレガシーAIスイッチを無視する
			if (enemy.baseData.nodes.empty()) {
				switch (enemy.aiState) {
				case EnemyInstance::AIState::Idle:
					enemy.currentAction = "Idle";
					enemy.velocity.X *= 0.9f;
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
				
				// 遠距離タイプの敵で、プレイヤーに近づきすぎた場合は攻撃よりも後退を優先する
				if (enemy.baseData.attackType == Editor::EnemyData::AttackType::Ranged && dist < enemy.preferredCombatDistance * 0.5f) {
					float moveDir = (dx > 0.0f) ? -1.0f : 1.0f;
					enemy.position.X += moveDir * enemy.baseData.moveSpeed * deltaTime;
					enemy.facingRight = (dx > 0.0f);
				}
				// 攻撃範囲に入ったら攻撃前アクションへ
				else if (dist <= enemy.preferredCombatDistance && enemy.attackCooldownTimer <= 0.0f) {
					enemy.aiState = EnemyInstance::AIState::PreAttack;
					enemy.preAttackTimer = enemy.attackWindupDuration;
					enemy.velocity.X = 0.0f;
				}
				// 索敵範囲外に出たら Idle に戻る
				else if (dist > enemy.baseData.aggroRadius * 1.5f) {
					enemy.aiState = EnemyInstance::AIState::Idle;
				}
				// 追跡移動または距離調整
				else {
					float moveDir = (dx > 0.0f) ? 1.0f : -1.0f;
					float moveSpeed = enemy.baseData.moveSpeed;
					
					if (enemy.baseData.attackType == Editor::EnemyData::AttackType::Ranged) {
						// 遠距離タイプは適正距離の範囲内で姿勢を保つ
						float keepDistanceMin = enemy.preferredCombatDistance * 0.8f;
						if (dist < keepDistanceMin) {
							moveDir = (dx > 0.0f) ? -1.0f : 1.0f; // 少し近いので離れる
						} else if (dist <= enemy.preferredCombatDistance) {
							moveSpeed = 0.0f; // 適正距離に入っているので止まって待機
						}
					} else if (enemy.sizeTier == kMinEnemySizeTier) {
						float orbitOffset = std::sin(enemy.stateTimer * 6.0f + enemy.id) * kSmallStrafeAmplitude;
						float desiredX = playerPosition.X - moveDir * enemy.preferredCombatDistance + orbitOffset;
						moveDir = (desiredX > enemy.position.X) ? 1.0f : -1.0f;
						moveSpeed *= 1.35f;
					}
					
					enemy.position.X += moveDir * moveSpeed * deltaTime;
					enemy.facingRight = (dx > 0.0f);
				}
				break;

			case EnemyInstance::AIState::PreAttack:
				enemy.currentAction = "Walk";
				enemy.velocity.X *= 0.8f;
				enemy.facingRight = (dx > 0.0f);
				if (dist > enemy.baseData.aggroRadius * 1.5f) {
					enemy.aiState = EnemyInstance::AIState::Idle;
				}
				else if (enemy.preAttackTimer <= 0.0f) {
					enemy.aiState = EnemyInstance::AIState::Attack;
					enemy.attackTimer = enemy.attackDuration;

					if (enemy.baseData.attackType == Editor::EnemyData::AttackType::Ranged) {
						// 遠距離攻撃: プロジェクタイルを発射
						ProjectileManager::GetInstance()->Fire(
							enemy.position,
							playerPosition,
							enemy.baseData.projectile,
							enemy.id
						);
						// 遠距離攻撃時は突進しない
						enemy.velocity.X *= 0.3f;
					} else {
						// 近接攻撃: 従来の突進
						float attackDir = (dx > 0.0f) ? 1.0f : -1.0f;
						enemy.velocity.X = attackDir * enemy.baseData.moveSpeed * enemy.burstSpeedMultiplier;
						enemy.velocity.Y = (std::max)(enemy.velocity.Y, 1.5f);
					}
				}
				break;

			case EnemyInstance::AIState::Attack:
				enemy.currentAction = "Attack";
				if (enemy.attackTimer <= 0.0f) {
					enemy.attackCooldownTimer = enemy.baseData.attackCooldown;
					enemy.aiState = EnemyInstance::AIState::Chase;
					enemy.velocity.X *= 0.35f;
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
				} // end switch(enemy.aiState)
			} // end if(enemy.baseData.nodes.empty())

			// --- 状態タイマー更新 ---
			enemy.stateTimer += deltaTime;

			if (enemy.behavior) {
				enemy.behavior->Update(enemy, deltaTime, playerPosition);
			}

			// --- JSON ステートマシンによる行動制御 ---
			// ノードが定義されている敵はステートマシンで currentAction を駆動する
			if (!enemy.baseData.nodes.empty()) {
				// 初回: currentAction に対応するノードを探す
				int currentNodeId = -1;
				for (const auto& n : enemy.baseData.nodes) {
					if (n.state == enemy.currentAction) { currentNodeId = n.id; break; }
				}
				// ノードが見つからなければ最初のノードから開始
				if (currentNodeId == -1) {
					currentNodeId = enemy.baseData.nodes.front().id;
					enemy.currentAction = enemy.baseData.nodes.front().state;
					enemy.stateTimer = 0.0f;
				}

				// リンク条件を評価して遷移
				for (const auto& link : enemy.baseData.links) {
					if (link.from != currentNodeId) continue;
					std::string c = link.condition;
					if (c.rfind("Time>=", 0) == 0) {
						try {
							float threshold = std::stof(c.substr(6));
							if (enemy.stateTimer >= threshold) {
								// 遷移先のノードを探す
								for (const auto& n : enemy.baseData.nodes) {
									if (n.id == link.to) {
										enemy.currentAction = n.state;
										enemy.stateTimer = 0.0f;
										break;
									}
								}
								break;
							}
						} catch (...) {}
					}
					else if (c.find("Dist<=") != std::string::npos) {
						try {
							size_t pos = c.find("Dist<=");
							float threshold = std::stof(c.substr(pos + 6));
							if (dist <= threshold) {
								for (const auto& n : enemy.baseData.nodes) {
									if (n.id == link.to) {
										enemy.currentAction = n.state;
										enemy.stateTimer = 0.0f;
										break;
									}
								}
								break;
							}
						} catch (...) {}
					}
					else if (c.find("Dist>") != std::string::npos) {
						try {
							size_t pos = c.find("Dist>");
							float threshold = std::stof(c.substr(pos + 5));
							if (dist > threshold) {
								for (const auto& n : enemy.baseData.nodes) {
									if (n.id == link.to) {
										enemy.currentAction = n.state;
										enemy.stateTimer = 0.0f;
										break;
									}
								}
								break;
							}
						} catch (...) {}
					}
					else if (c.find("Always") != std::string::npos) {
						for (const auto& n : enemy.baseData.nodes) {
							if (n.id == link.to) {
								enemy.currentAction = n.state;
								enemy.stateTimer = 0.0f;
								break;
							}
						}
						break;
					}
					// BOOL: condition - check runtime bool flags on the instance
					else if (c.rfind("BOOL:", 0) == 0) {
						std::string flag = c.substr(5);
						auto flagIt = enemy.runtimeBoolFlags.find(flag);
						if (flagIt != enemy.runtimeBoolFlags.end() && flagIt->second) {
							for (const auto& n : enemy.baseData.nodes) {
								if (n.id == link.to) {
									enemy.currentAction = n.state;
									enemy.stateTimer = 0.0f;
									// Consume the flag (reset it)
									flagIt->second = false;
									break;
								}
							}
							break;
						}
					}
				}

				// ステート遷移が発生した場合はその場で currentNodeId を更新する
				if (enemy.stateTimer == 0.0f) {
					for (const auto& n : enemy.baseData.nodes) {
						if (n.state == enemy.currentAction) {
							currentNodeId = n.id;
							break;
						}
					}
				}

				// --- ステートに応じた物理挙動 ---
				const Game::Editor::Node* currentNodeInfo = nullptr;
				for (const auto& n : enemy.baseData.nodes) {
					if (n.id == currentNodeId) {
						currentNodeInfo = &n;
						break;
					}
				}

				if (currentNodeInfo) {
					// プレイヤーのほうを向く
					if (currentNodeInfo->facePlayer) {
						enemy.facingRight = (dx > 0.0f);
					}

                    // Node entry handling: allow nodes to trigger actions on entering.
					// - If a node's `boundBool` contains a firing token (e.g. "FireProjectile"),
					//   spawn a projectile immediately on entry. This enables node-driven
					//   flows like: Charge -> Time>=X -> Shoot (where Shoot node triggers fire).
					// - Also start spline motion on entry if specified.
					if (enemy.stateTimer == 0.0f) {
             const std::string& fb = currentNodeInfo->boundBool;
				// If entering a Charge state for a ranged enemy, spawn a visual attached projectile
				if (currentNodeInfo->state == "Charge" && enemy.baseData.attackType == Editor::EnemyData::AttackType::Ranged) {
					Game::ProjectileData pd = enemy.baseData.projectile;
					pd.spawnAttached = true;
					pd.scaleOnCharge = true; // チャージエフェクト（弾の巨大化）を有効にする
                        // offset relative to the enemy model (tunable). place the visual
						// bullet well above the slime's head so it is clearly separated
						// and ensure activation (firing) originates from that position.
						pd.attachOffset = { 0.0f, 1.2f * enemy.modelScale, 0.0f };
					ProjectileManager::GetInstance()->Fire(
						enemy.position,
						playerPosition,
						pd,
						enemy.id
					);
				}
				// If the node requests a fire action via boundBool, attempt to activate any attached projectile;
				// if none exists, fall back to spawning a new projectile.
				else if (!fb.empty() && (fb == "FireProjectile" || fb == "fireProjectile" || fb == "Shoot" || fb == "shoot" || fb == "Fire" || fb == "fire")) {
					bool activated = ProjectileManager::GetInstance()->ActivateAttachedProjectile(enemy.id, playerPosition);
					if (!activated) {
						ProjectileManager::GetInstance()->Fire(
							enemy.position,
							playerPosition,
							enemy.baseData.projectile,
							enemy.id
						);
					}
				}

						if (!currentNodeInfo->splineMotionName.empty()) {
							enemy.motionController.Play(currentNodeInfo->splineMotionName, enemy.position, currentNodeInfo->splineDuration);
						}
					}

					// SplineMotion再生中なら物理演算をオーバーライド
					if (enemy.motionController.IsPlaying()) {
						Lumina::Math::F32x3 dir = enemy.facingRight ? Lumina::Math::F32x3{1.0f, 0.0f, 0.0f} : Lumina::Math::F32x3{-1.0f, 0.0f, 0.0f};
						
						Lumina::Math::F32x3 oldOffset = enemy.motionController.GetLastLocalOffset();
						(void)enemy.motionController.Update(deltaTime, dir); // absolute position is unused
						Lumina::Math::F32x3 newOffset = enemy.motionController.GetLastLocalOffset();
						
						Lumina::Math::F32x3 delta;
						delta.X = newOffset.X - oldOffset.X;
						delta.Y = newOffset.Y - oldOffset.Y;
						delta.Z = newOffset.Z - oldOffset.Z;
						
						if (deltaTime > 0.0f) {
							enemy.velocity.X = delta.X / deltaTime;
							enemy.velocity.Y = delta.Y / deltaTime;
							enemy.velocity.Z = delta.Z / deltaTime;
						} else {
							enemy.velocity = {0.0f, 0.0f, 0.0f};
						}

						// 重力などの汎用物理移動をキャンセルし、Splineの純粋な相対移動(delta)を適用する
						// posBeforePhysics はコリジョン押し出し結果が維持された正しい開始位置
						enemy.position.X = posBeforePhysics.X + delta.X;
						enemy.position.Y = posBeforePhysics.Y + delta.Y;
						enemy.position.Z = posBeforePhysics.Z + delta.Z;
                    } else {
						// If the node is marked with a boolean "walk" trigger, drive horizontal
						// movement via velocity so node-driven state machines can make enemies walk
						// without relying on spline motions.
						bool nodeWalk = (currentNodeInfo->boundBool == "walk" || currentNodeInfo->boundBool == "Walk");
						if (nodeWalk) {
							float moveDir = enemy.facingRight ? 1.0f : -1.0f;
							enemy.velocity.X = moveDir * enemy.baseData.moveSpeed;
						}

						// 継続的な摩擦/ブレーキ
						// 空中にいるときは横方向の摩擦を軽減し、落下中の慣性を保つ
						float expectedGroundedVelY = -9.8f * deltaTime;
						bool isGrounded = std::abs(enemy.velocity.Y - expectedGroundedVelY) < 0.001f;

						if (isGrounded) {
							enemy.velocity.X *= currentNodeInfo->velocityFrictionX;
						} else {
							// 空中では摩擦を最小限にする（極端な減速を防ぐ）
							float airFriction = (std::max)(currentNodeInfo->velocityFrictionX, 0.98f);
							enemy.velocity.X *= airFriction;
						}

						// ステート突入時の付加力（1フレーム目のみ付与するため approximate で判定）
						if (enemy.stateTimer < deltaTime * 1.5f) {
							if (currentNodeInfo->jumpVelocityXMult != 0.0f) {
								float jumpDir = (dx > 0.0f) ? 1.0f : -1.0f;
								enemy.velocity.X = jumpDir * enemy.baseData.moveSpeed * currentNodeInfo->jumpVelocityXMult;
							}
							// Y軸はジャンプ力代入（Slimeのもともとの挙動に合わせて単純設定）
							if (currentNodeInfo->jumpVelocityY != 0.0f) {
								enemy.velocity.Y = currentNodeInfo->jumpVelocityY;
							}
						}
					}
				}

				// ステートマシンで駆動されているのでデフォルトAIを上書き
				// (Chase等に入らないようにする)
				enemy.aiState = EnemyInstance::AIState::Idle;
			}

            float targetFacingYaw = enemy.facingRight ? 0.0f : kTurnedFacingYaw;
			float turnStep = kEnemyFacingTurnSpeed * deltaTime;
			if (enemy.renderFacingYaw < targetFacingYaw) {
				enemy.renderFacingYaw = (std::min)(enemy.renderFacingYaw + turnStep, targetFacingYaw);
			} else if (enemy.renderFacingYaw > targetFacingYaw) {
				enemy.renderFacingYaw = (std::max)(enemy.renderFacingYaw - turnStep, targetFacingYaw);
			}

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
		if (enemy->currentHP < 0 || HasInvulnerableHpSetting(enemy->baseData)) return false;

		enemy->currentHP -= damage;
		enemy->hurtTimer = 0.2f;
		enemy->velocity.Y = std::max(enemy->velocity.Y, 3.5f);

		if (enemy->currentHP <= 0) {
			auto splitData = enemy->baseData;
			auto splitPosition = enemy->position;
			auto splitVelocity = enemy->velocity;
			bool splitFacingRight = enemy->facingRight;
			int splitTier = enemy->sizeTier;

			enemy->currentHP = 0;
			enemy->isDead = true;
			++Event::EnemiesDefeated;
			if (onDeathCallback_) {
				onDeathCallback_(*enemy);
			}
			SpawnSplitChildren(*this, splitData, splitPosition, splitVelocity, splitFacingRight, splitTier);
			return true;
		}
		return false;
	}

	int EnemyManager::DealAreaDamage(const Lumina::Math::F32x3& origin, float radius,
		int damage, bool facingRight, bool directional) {
		int killCount = 0;
		struct SplitRequest {
			Editor::EnemyData data;
			Lumina::Math::F32x3 position;
			Lumina::Math::F32x3 velocity;
			bool facingRight = true;
			int sizeTier = 1;
		};
		std::vector<SplitRequest> splitRequests;

		for (auto& enemy : instances_) {
			if (enemy.isDead) continue;
			if (enemy.currentHP < 0 || HasInvulnerableHpSetting(enemy.baseData)) continue;

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
            // Prevent horizontal knockback from area damage; only apply vertical impulse via DealDamage.
			enemy.velocity.Y = std::max(enemy.velocity.Y, 3.5f);

			if (enemy.currentHP <= 0) {
				splitRequests.push_back({
					enemy.baseData,
					enemy.position,
					enemy.velocity,
					enemy.facingRight,
					enemy.sizeTier
				});
				enemy.currentHP = 0;
				enemy.isDead = true;
				++Event::EnemiesDefeated;
				if (onDeathCallback_) {
					onDeathCallback_(enemy);
				}
				++killCount;
			}
		}

		for (const auto& split : splitRequests) {
			SpawnSplitChildren(*this, split.data, split.position, split.velocity, split.facingRight, split.sizeTier);
		}

		return killCount;
	}

	// ============================
	//  コールバック
	// ============================

	void EnemyManager::SetOnEnemyDeathCallback(OnEnemyDeathCallback callback) {
		onDeathCallback_ = std::move(callback);
	}

	EnemyManager::EnemyManager() {
		instances_.reserve(4096U);
	}
}
