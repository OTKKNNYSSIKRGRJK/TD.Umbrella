module Game.ProjectileManager;

import <cmath>;
import <algorithm>;
import <fstream>;
import <filesystem>;

import nlohmann.json;
import Game.MathUtils;
import Game.Player;
import Game.EnemyManager;

namespace fs = std::filesystem;

namespace {
	// Actor JSON をロードするローカル関数
	// (ActorEditor の from_json は別モジュール実装部なので直接呼べないため手動パース)
	bool LoadActorDataFromFile(const std::string& actorName, Game::Editor::ActorData& out) {
		std::string filename = "Assets/Data/Actor/actor_" + actorName + ".json";
		if (!fs::exists(filename)) return false;

		std::ifstream file(filename);
		if (!file.is_open()) return false;

		try {
			nlohmann::json j;
			file >> j;

			if (j.contains("name")) j.at("name").get_to(out.name);

			// Transform
			if (j.contains("transform") && j["transform"].is_object()) {
				const auto& t = j["transform"];
				if (t.contains("posX")) t.at("posX").get_to(out.transform.posX);
				if (t.contains("posY")) t.at("posY").get_to(out.transform.posY);
				if (t.contains("posZ")) t.at("posZ").get_to(out.transform.posZ);
				if (t.contains("rotX")) t.at("rotX").get_to(out.transform.rotX);
				if (t.contains("rotY")) t.at("rotY").get_to(out.transform.rotY);
				if (t.contains("rotZ")) t.at("rotZ").get_to(out.transform.rotZ);
				if (t.contains("scaleX")) t.at("scaleX").get_to(out.transform.scaleX);
				if (t.contains("scaleY")) t.at("scaleY").get_to(out.transform.scaleY);
				if (t.contains("scaleZ")) t.at("scaleZ").get_to(out.transform.scaleZ);
			}

			// Visual
			if (j.contains("visual") && j["visual"].is_object()) {
				const auto& v = j["visual"];
				if (v.contains("meshPath")) v.at("meshPath").get_to(out.visual.meshPath);
				if (v.contains("materialIndex")) v.at("materialIndex").get_to(out.visual.materialIndex);
			}

			// Movement
			if (j.contains("movement") && j["movement"].is_object()) {
				const auto& m = j["movement"];
				if (m.contains("type")) out.movement.type = static_cast<Game::Editor::MovementType>(m.at("type").get<int>());
				if (m.contains("speed")) m.at("speed").get_to(out.movement.speed);
				if (m.contains("dirX")) m.at("dirX").get_to(out.movement.dirX);
				if (m.contains("dirY")) m.at("dirY").get_to(out.movement.dirY);
				if (m.contains("dirZ")) m.at("dirZ").get_to(out.movement.dirZ);
				if (m.contains("range")) m.at("range").get_to(out.movement.range);
				if (m.contains("easing")) out.movement.easing = static_cast<Game::Editor::EasingType>(m.at("easing").get<int>());
				if (m.contains("splineMotionName")) m.at("splineMotionName").get_to(out.movement.splineMotionName);
				if (m.contains("totalDuration")) m.at("totalDuration").get_to(out.movement.totalDuration);
				if (m.contains("loopSpline")) m.at("loopSpline").get_to(out.movement.loopSpline);
				if (m.contains("nodeTimings") && m["nodeTimings"].is_array()) {
					for (const auto& nj : m["nodeTimings"]) {
						Game::Editor::NodeTiming nt;
						if (nj.contains("arrivalTime")) nj.at("arrivalTime").get_to(nt.arrivalTime);
						if (nj.contains("easing")) nt.easing = static_cast<Game::Editor::EasingType>(nj.at("easing").get<int>());
						out.movement.nodeTimings.push_back(nt);
					}
				}
			}

			// Interaction
			if (j.contains("interaction") && j["interaction"].is_object()) {
				const auto& ind = j["interaction"];
				if (ind.contains("type")) out.interaction.type = static_cast<Game::Editor::InteractionType>(ind.at("type").get<int>());
				if (ind.contains("damageValue")) ind.at("damageValue").get_to(out.interaction.damageValue);
				if (ind.contains("pushForce")) ind.at("pushForce").get_to(out.interaction.pushForce);
				if (ind.contains("activationTriggerID")) ind.at("activationTriggerID").get_to(out.interaction.activationTriggerID);
			}

			// Collider
			if (j.contains("collider") && j["collider"].is_object()) {
				const auto& c = j["collider"];
				if (c.contains("type")) out.collider.type = static_cast<Game::Editor::ColliderType>(c.at("type").get<int>());
				if (c.contains("sizeX")) c.at("sizeX").get_to(out.collider.sizeX);
				if (c.contains("sizeY")) c.at("sizeY").get_to(out.collider.sizeY);
				if (c.contains("sizeZ")) c.at("sizeZ").get_to(out.collider.sizeZ);
				if (c.contains("collisionVertices") && c["collisionVertices"].is_array()) {
					for (const auto& vj : c["collisionVertices"]) {
						Game::Editor::ActorCollisionVertex v;
						if (vj.contains("x")) vj.at("x").get_to(v.x);
						if (vj.contains("y")) vj.at("y").get_to(v.y);
						out.collider.collisionVertices.push_back(v);
					}
				}
			}

			// Lifecycle
			if (j.contains("lifecycle") && j["lifecycle"].is_object()) {
				const auto& l = j["lifecycle"];
				if (l.contains("spawnTrigger")) out.lifecycle.spawnTrigger = static_cast<Game::Editor::SpawnTriggerType>(l.at("spawnTrigger").get<int>());
				if (l.contains("spawnValue")) l.at("spawnValue").get_to(out.lifecycle.spawnValue);
				if (l.contains("spawnEventID")) l.at("spawnEventID").get_to(out.lifecycle.spawnEventID);
				if (l.contains("lifetime")) l.at("lifetime").get_to(out.lifecycle.lifetime);
				if (l.contains("autoDestroyOffscreen")) l.at("autoDestroyOffscreen").get_to(out.lifecycle.autoDestroyOffscreen);
			}

			return true;
		} catch (...) {
			return false;
		}
	}
}

namespace Game {

	// ============================
	//  コライダー初期化・更新
	// ============================

	void Projectile::InitCollider() {
		collider = std::make_unique<ConvexCollider>();
		collider->SetMyType(COL_Enemy_Attack);
		collider->SetYourType(COL_Player | COL_Ground);
		collider->SetUserData(this);

		std::vector<Lumina::Math::F32x3> verts;

       // 1. スケールの取得
		float sx = (actorData.transform.scaleX > 0.0f) ? actorData.transform.scaleX : 1.0f;
		float sy = (actorData.transform.scaleY > 0.0f) ? actorData.transform.scaleY : 1.0f;
		float sz = (actorData.transform.scaleZ > 0.0f) ? actorData.transform.scaleZ : 1.0f;

		// Apply visualScale (for charge-up visuals) so collider roughly matches visual size
		float vs = (this->visualScale > 0.0f) ? this->visualScale : 1.0f;
		sx *= vs; sy *= vs; sz *= vs;

		if (actorData.collider.type == Game::Editor::ColliderType::Polygon && !actorData.collider.collisionVertices.empty()) {
			// ActorEditorで描いたポリゴンを使用
            float depthZ = (actorData.collider.sizeZ > 0.0f) ? actorData.collider.sizeZ * vs : 0.5f * vs;
			for (const auto& v : actorData.collider.collisionVertices) {
				// Scale collision vertices by visualScale so collider follows charge-up size
				verts.push_back({ v.x * vs, v.y * vs, depthZ });
				verts.push_back({ v.x * vs, v.y * vs, -depthZ });
			}
		} else if (actorData.collider.type == Game::Editor::ColliderType::Box) {
			// Actorの Box サイズそのものを使用し、全体の Scale も適用
			float hw = (actorData.collider.sizeX > 0.0f ? actorData.collider.sizeX : 1.0f) * sx;
			float hh = (actorData.collider.sizeY > 0.0f ? actorData.collider.sizeY : 1.0f) * sy;
			float hd = (actorData.collider.sizeZ > 0.0f ? actorData.collider.sizeZ : 1.0f) * sz;
			verts = {
				{ -hw, -hh, -hd }, {  hw, -hh, -hd }, {  hw,  hh, -hd }, { -hw,  hh, -hd },
				{ -hw, -hh,  hd }, {  hw, -hh,  hd }, {  hw,  hh,  hd }, { -hw,  hh,  hd },
			};
		} else {
			// フォールバック（未設定時のデフォルトの立方体）
			// メッシュのTransform Scaleを考慮して自動で縮小する
			float rX = 1.0f * sx;
			float rY = 1.0f * sy;
			float rZ = 1.0f * sz;
			verts = {
				{ -rX, -rY, -rZ }, {  rX, -rY, -rZ }, {  rX,  rY, -rZ }, { -rX,  rY, -rZ },
				{ -rX, -rY,  rZ }, {  rX, -rY,  rZ }, {  rX,  rY,  rZ }, { -rX,  rY,  rZ },
			};
		}
		
		collider->SetVertices(verts);
		collider->SetWorldPosition(position);

		float rx = actorData.transform.rotX * 3.14159265f / 180.0f;
		float ry = actorData.transform.rotY * 3.14159265f / 180.0f;
		float rz = actorData.transform.rotZ * 3.14159265f / 180.0f;
		float ox = actorData.transform.posX;
		float oy = actorData.transform.posY;
		float oz = actorData.transform.posZ;

		auto localMat = Game::MathUtils::SRT({ 1.0f, 1.0f, 1.0f }, { rx, ry, rz }, { ox, oy, oz });
		auto worldPosMat = Game::MathUtils::Translate(position);
		auto worldMat = localMat * worldPosMat;

		collider->SetWorldMatrix(worldMat);
		collider->UpdateAABB();
	}

	void Projectile::UpdateCollider() {
		if (!collider) return;
		collider->SetWorldPosition(position);

		float rx = actorData.transform.rotX * 3.14159265f / 180.0f;
		float ry = actorData.transform.rotY * 3.14159265f / 180.0f;
		float rz = actorData.transform.rotZ * 3.14159265f / 180.0f;
		float ox = actorData.transform.posX;
		float oy = actorData.transform.posY;
		float oz = actorData.transform.posZ;

		auto localMat = Game::MathUtils::SRT({ 1.0f, 1.0f, 1.0f }, { rx, ry, rz }, { ox, oy, oz });
		auto worldPosMat = Game::MathUtils::Translate(position);
		auto worldMat = localMat * worldPosMat;

		collider->SetWorldMatrix(worldMat);
		collider->UpdateAABB();
	}

	// ============================
	//  シングルトン
	// ============================

	std::unique_ptr<ProjectileManager> ProjectileManager::instance_ = nullptr;

	ProjectileManager* ProjectileManager::GetInstance() {
		if (instance_ == nullptr) {
			instance_ = std::unique_ptr<ProjectileManager>(new ProjectileManager());
		}
		return instance_.get();
	}

	uint32_t ProjectileManager::GenerateId() {
		return nextId_++;
	}

	bool ProjectileManager::ActivateAttachedProjectile(uint32_t ownerEnemyId, const Lumina::Math::F32x3& target) {
		for (auto& p : projectiles_) {
			if (p.isDead) continue;
			if (p.ownerEnemyId == ownerEnemyId && p.isAttached) {
				// compute velocity toward target using actorData / speed
				float dx = target.X - p.position.X;
				float dy = target.Y - p.position.Y;
				float dz = target.Z - p.position.Z;
				float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
				if (dist < 0.001f) dist = 1.0f;
				float nx = dx / dist;
				float ny = dy / dist;
				float nz = dz / dist;

				float speed = (std::max)(1.0f, p.actorData.movement.speed);

				if (p.data.isHoming) {
					p.velocity = { nx * speed, ny * speed, nz * speed };
				} else {
					switch (p.actorData.movement.type) {
					case Editor::MovementType::Linear:
					case Editor::MovementType::None:
					default:
						p.velocity = { nx * speed, ny * speed, nz * speed };
						break;
					case Editor::MovementType::PingPong:
						p.velocity = { nx * speed, ny * speed, nz * speed };
						p.traveledDistance = 0.0f;
						p.pingPongDirection = 1.0f;
						break;
					case Editor::MovementType::Spline:
						p.velocity = { nx * speed, ny * speed, nz * speed };
						p.motionController.Play(p.actorData.movement.splineMotionName, p.position, p.actorData.movement.totalDuration);
						break;
					}
				}

				p.isAttached = false;
                // Ensure visual scale and collider are preserved when detaching
				p.actorData.transform.scaleX = p.visualScale;
				p.actorData.transform.scaleY = p.visualScale;
				p.actorData.transform.scaleZ = p.visualScale;
				p.InitCollider();
				p.splineOrigin = p.position;
				return true;
			}
		}
		return false;
	}

	// ============================
	//  発射
	// ============================

	void ProjectileManager::Fire(
		const Lumina::Math::F32x3& origin,
		const Lumina::Math::F32x3& target,
		const ProjectileData& data,
		uint32_t ownerEnemyId
	) {
		Projectile proj;
		proj.data = data;
		proj.id = GenerateId();
		proj.position = origin;
		proj.ownerEnemyId = ownerEnemyId;
		proj.aliveTime = 0.0f;
		proj.isDead = false;
		proj.splineOrigin = origin;

		// Actor データをロード
		if (!data.actorName.empty()) {
			LoadActorDataFromFile(data.actorName, proj.actorData);

			// ActorEditorで設定されたパラメータでEnemyData側の設定（デフォルト値）を上書き
			proj.data.lifetime = proj.actorData.lifecycle.lifetime;
			proj.data.damage = static_cast<int>(proj.actorData.interaction.damageValue);
			proj.data.colliderRadius = proj.actorData.collider.sizeX;

			// If NOT spawned as attached, but the actor data has a transform offset, apply it relative to the enemy's facing and scale!
			if (!data.spawnAttached) {
				auto* enemy = Game::EnemyManager::GetInstance()->GetInstance(ownerEnemyId);
				if (enemy) {
					float ox = proj.actorData.transform.posX;
					float oy = proj.actorData.transform.posY;
					float oz = proj.actorData.transform.posZ;
					proj.position.X += (enemy->facingRight ? ox : -ox) * enemy->modelScale;
					proj.position.Y += oy * enemy->modelScale;
					proj.position.Z += oz * enemy->modelScale;
				}
			}
		}

		// If the template requests an attached spawn, mark and position the projectile accordingly.
		if (data.spawnAttached) {
			proj.isAttached = true;
			
			// Use the actor's transform position as the attach offset (so editor positioning works)
			proj.data.attachOffset.X = proj.actorData.transform.posX;
			proj.data.attachOffset.Y = proj.actorData.transform.posY;
			proj.data.attachOffset.Z = proj.actorData.transform.posZ;
			
			// Zero out the visual offset so rendering doesn't apply it a second time
			proj.actorData.transform.posX = 0.0f;
			proj.actorData.transform.posY = 0.0f;
			proj.actorData.transform.posZ = 0.0f;
			
			proj.position.X = origin.X + proj.data.attachOffset.X;
			proj.position.Y = origin.Y + proj.data.attachOffset.Y;
			proj.position.Z = origin.Z + proj.data.attachOffset.Z;
			proj.velocity = { 0.0f, 0.0f, 0.0f };

			// initialize visual scaling state for chargable projectiles
			if (data.scaleOnCharge) {
				proj.visualScale = data.initialScale;
				float actorScale = (proj.actorData.transform.scaleX > 0.0f) ? proj.actorData.transform.scaleX : 1.0f;
				proj.targetVisualScale = actorScale;
				proj.chargeTimer = 0.0f;

				// Apply initial visual scale to actorData so renderer shows scaled model
				proj.actorData.transform.scaleX = proj.visualScale;
				proj.actorData.transform.scaleY = proj.visualScale;
				proj.actorData.transform.scaleZ = proj.visualScale;
			}
			
			// Start spline motion on spawn so it animates while attached
			if (proj.actorData.movement.type == Editor::MovementType::Spline) {
				proj.motionController.Play(proj.actorData.movement.splineMotionName, {0.0f, 0.0f, 0.0f}, proj.actorData.movement.totalDuration);
			}
		}

		// 方向ベクトルの計算（ターゲット方向）
		float dx = target.X - origin.X;
		float dy = target.Y - origin.Y;
		float dz = target.Z - origin.Z;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (dist < 0.001f) dist = 1.0f; // ゼロ除算防止

		float nx = dx / dist;
		float ny = dy / dist;
		float nz = dz / dist;

		// Actor の MovementModule から速度を取得（デフォルト 5.0f）
		float speed = (std::max)(1.0f, proj.actorData.movement.speed);

		if (data.isHoming) {
			// ホーミング: 初期方向はターゲット方向、速度は Actor から
			proj.velocity = { nx * speed, ny * speed, nz * speed };
		} else {
			// Actor の MovementModule に基づく
			switch (proj.actorData.movement.type) {
			case Editor::MovementType::Linear:
			case Editor::MovementType::None:
			default:
				// 直線: ターゲット方向 × Actor の speed
				proj.velocity = { nx * speed, ny * speed, nz * speed };
				break;

			case Editor::MovementType::PingPong:
				// PingPong: ターゲット方向に飛ぶが、range で折り返す
				proj.velocity = { nx * speed, ny * speed, nz * speed };
				proj.traveledDistance = 0.0f;
				proj.pingPongDirection = 1.0f;
				break;

			case Editor::MovementType::Spline:
				// Spline: MotionManagerを使ってスプライン軌道を再生する
				proj.velocity = { nx * speed, ny * speed, nz * speed }; // 向き(左右反転判定)用に保持
				proj.motionController.Play(proj.actorData.movement.splineMotionName, origin, proj.actorData.movement.totalDuration);
				break;
			}
		}

		proj.InitCollider();

		// コリジョンコールバック: プレイヤーに当たったら消える、地面に当たったら消える
     proj.collider->onCollisionCallback = [id = proj.id](Collider* other, [[maybe_unused]] const Lumina::Math::F32x3& pushOut) {
			if (other->GetMyType() == COL_Player) {
               Player* player = static_cast<Player*>(other->GetUserData());
				auto* mgr = ProjectileManager::GetInstance();
				for (auto& p : const_cast<std::vector<Projectile>&>(mgr->GetAll())) {
					if (p.id == id) {
                        if (player != nullptr) {
							player->GetStatusComponent().TakeDamage(static_cast<float>((std::max)(1, p.data.damage)));
						}
						p.isDead = true;
						break;
					}
				}
			}
			else if (other->GetMyType() == COL_Ground) {
				auto* mgr = ProjectileManager::GetInstance();
				for (auto& p : const_cast<std::vector<Projectile>&>(mgr->GetAll())) {
					if (p.id == id) {
						p.isDead = true;
						break;
					}
				}
			}
		};

		projectiles_.push_back(std::move(proj));
	}

	// ============================
	//  更新
	// ============================

	void ProjectileManager::Update(float deltaTime, const Lumina::Math::F32x3& playerPosition) {
		for (auto& proj : projectiles_) {
			if (proj.isDead) continue;

			// 寿命チェック
			proj.aliveTime += deltaTime;
			if (proj.aliveTime >= proj.data.lifetime) {
				proj.isDead = true;
				continue;
			}

           // 移動処理（位置の更新フラグ）
			bool manuallyUpdatePosition = true;

			// If this projectile is attached to an enemy, follow that enemy's position
			// and skip the usual movement logic.
            if (proj.isAttached && proj.ownerEnemyId != 0) {
				auto* enemy = Game::EnemyManager::GetInstance()->GetInstance(proj.ownerEnemyId);
				if (enemy) {
					// update charge timer and visual scale if configured
					if (proj.data.scaleOnCharge) {
						proj.chargeTimer += deltaTime;
						float dur = (proj.data.chargeGrowDuration > 0.0f) ? proj.data.chargeGrowDuration : 1.0f;
						float t = (std::min)(1.0f, proj.chargeTimer / dur);
						float prevScale = proj.visualScale;
                // apply a smoothstep easing so growth feels nicer (ease in/out)
				float te = t * t * (3.0f - 2.0f * t); // smoothstep
				proj.visualScale = proj.data.initialScale + (proj.targetVisualScale - proj.data.initialScale) * te;
						if (std::abs(prevScale - proj.visualScale) > 0.01f) {
							// update collider to match new visual size
							proj.InitCollider();
						}
						// Ensure actorData transform matches visualScale so renderer displays correct size
						proj.actorData.transform.scaleX = proj.visualScale;
						proj.actorData.transform.scaleY = proj.visualScale;
						proj.actorData.transform.scaleZ = proj.visualScale;
					}

					proj.position.X = enemy->position.X + (enemy->facingRight ? proj.data.attachOffset.X : -proj.data.attachOffset.X);
					proj.position.Y = enemy->position.Y + proj.data.attachOffset.Y;
					proj.position.Z = enemy->position.Z + proj.data.attachOffset.Z;
					
					// Apply spline motion relative to the attached point
					if (proj.actorData.movement.type == Editor::MovementType::Spline && proj.motionController.IsPlaying()) {
						Lumina::Math::F32x3 dir = enemy->facingRight ? Lumina::Math::F32x3{1.0f, 0.0f, 0.0f} : Lumina::Math::F32x3{-1.0f, 0.0f, 0.0f};
						Lumina::Math::F32x3 prevOffset = proj.motionController.GetLastLocalOffset();
						(void)proj.motionController.Update(deltaTime, dir);
						Lumina::Math::F32x3 localOffset = proj.motionController.GetLastLocalOffset();
						
						proj.position.X += localOffset.X;
						proj.position.Y += localOffset.Y;
						proj.position.Z += localOffset.Z;

						// Auto-rotate based on actual position relative to the boss to simulate a weapon swing (pointing outwards)
						float relX = proj.position.X - enemy->position.X;
						float relY = proj.position.Y - enemy->position.Y;
						if (relX * relX + relY * relY > 0.0001f) {
							float angle = std::atan2(relY, relX);
							// Because the cube is vertical (Y is longest), an angle of 0 (moving right) means it should point right (-90 deg rotZ)
							proj.actorData.transform.rotZ = angle * 180.0f / 3.14159265f - 90.0f;
						}
					}
					// If it's an idle equipment not moving, use the base rotation from ActorData but apply facing flip if needed
					else if (proj.isEquipment) {
						if (!enemy->facingRight) {
							// If facing left, visually flip the Z rotation so it mirrors the holding pose
							proj.actorData.transform.rotZ = -proj.actorData.transform.rotZ;
						}
					}
					
					proj.UpdateCollider();
					continue;
				}
			}

			// ホーミングの場合
			if (proj.data.isHoming) {
				float speed = (std::max)(1.0f, proj.actorData.movement.speed);
				float dx = playerPosition.X - proj.position.X;
				float dy = playerPosition.Y - proj.position.Y;
				float dz = playerPosition.Z - proj.position.Z;
				float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
				if (dist > 0.01f) {
					float nx = dx / dist;
					float ny = dy / dist;
					float nz = dz / dist;

					float str = proj.data.homingStrength * deltaTime;
					proj.velocity.X += (nx * speed - proj.velocity.X) * str;
					proj.velocity.Y += (ny * speed - proj.velocity.Y) * str;
					proj.velocity.Z += (nz * speed - proj.velocity.Z) * str;

					// 速度を一定に保つ
					float spd = std::sqrt(
						proj.velocity.X * proj.velocity.X +
						proj.velocity.Y * proj.velocity.Y +
						proj.velocity.Z * proj.velocity.Z
					);
					if (spd > 0.01f) {
						float ratio = speed / spd;
						proj.velocity.X *= ratio;
						proj.velocity.Y *= ratio;
						proj.velocity.Z *= ratio;
					}
				}
			} else {
				// Actor MovementType に基づく挙動更新
				switch (proj.actorData.movement.type) {
				case Editor::MovementType::Linear:
				case Editor::MovementType::None:
				default:
					// 等速直線運動（velocity 変更なし）
					break;

				case Editor::MovementType::Spline:
					// スプライン再生中なら絶対座標を更新
					if (proj.motionController.IsPlaying()) {
						proj.position = proj.motionController.Update(deltaTime, proj.velocity);
						manuallyUpdatePosition = false; // 位置はMotionControllerによって制御される
					} else {
						// 再生が終了したら現在の位置で止まる、あるいは死ぬ
						// とりあえず止まる
						proj.velocity = { 0.0f, 0.0f, 0.0f };
					}
					break;

				case Editor::MovementType::PingPong: {
					// PingPong: range を超えたら方向反転
					float speed = std::sqrt(
						proj.velocity.X * proj.velocity.X +
						proj.velocity.Y * proj.velocity.Y +
						proj.velocity.Z * proj.velocity.Z
					);
					proj.traveledDistance += speed * deltaTime;
					if (proj.traveledDistance >= proj.actorData.movement.range) {
						proj.traveledDistance = 0.0f;
						proj.pingPongDirection *= -1.0f;
						proj.velocity.X *= -1.0f;
						proj.velocity.Y *= -1.0f;
						proj.velocity.Z *= -1.0f;
					}
					break;
				}
				}
			}

			// 位置更新（スプライン以外の場合）
			if (manuallyUpdatePosition) {
				proj.position.X += proj.velocity.X * deltaTime;
				proj.position.Y += proj.velocity.Y * deltaTime;
				proj.position.Z += proj.velocity.Z * deltaTime;
			}

			// Z軸を常に0にする（2Dゲームなので）
			proj.position.Z = 0.0f;

			// コライダー更新
			proj.UpdateCollider();
		}
	}

	// ============================
	//  コリジョン登録
	// ============================

	void ProjectileManager::RegisterCollidersTo(CollisionManager& cm) {
		for (auto& proj : projectiles_) {
			if (proj.isDead) continue;
			if (proj.collider) {
				cm.SetColliders(proj.collider.get());
			}
		}
	}

	// ============================
	//  クリーンアップ
	// ============================

	void ProjectileManager::RemoveDeadProjectiles() {
		projectiles_.erase(
			std::remove_if(projectiles_.begin(), projectiles_.end(),
				[](const Projectile& p) { return p.isDead; }),
			projectiles_.end()
		);
	}

	void ProjectileManager::RemoveEquipment(uint32_t ownerEnemyId) {
		for (auto& p : projectiles_) {
			if (!p.isDead && p.ownerEnemyId == ownerEnemyId && p.isEquipment) {
				p.isDead = true;
			}
		}
	}

	void ProjectileManager::ClearAll() {
		projectiles_.clear();
		nextId_ = 1;
	}
}
