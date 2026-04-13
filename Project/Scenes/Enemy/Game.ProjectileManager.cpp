module Game.ProjectileManager;

import <cmath>;
import <algorithm>;

import Game.MathUtils;
import Game.Player;

namespace Game {

	// ============================
	//  コライダー初期化・更新
	// ============================

	void Projectile::InitCollider() {
		collider = std::make_unique<ConvexCollider>();
		collider->SetMyType(COL_Enemy_Attack);
		collider->SetYourType(COL_Player | COL_Ground);
		collider->SetUserData(this);

		// data.colliderRadius を一辺とする小さな立方体
		float r = data.colliderRadius;
		std::vector<Lumina::Math::F32x3> verts = {
			{ -r, -r, -r },
			{  r, -r, -r },
			{  r,  r, -r },
			{ -r,  r, -r },
			{ -r, -r,  r },
			{  r, -r,  r },
			{  r,  r,  r },
			{ -r,  r,  r },
		};
		collider->SetVertices(verts);
		collider->SetWorldPosition(position);

		auto worldMat = Game::MathUtils::Translate(position);
		collider->SetWorldMatrix(worldMat);
		collider->UpdateAABB();
	}

	void Projectile::UpdateCollider() {
		if (!collider) return;
		collider->SetWorldPosition(position);
		auto worldMat = Game::MathUtils::Translate(position);
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

		// 方向ベクトルの計算
		float dx = target.X - origin.X;
		float dy = target.Y - origin.Y;
		float dz = target.Z - origin.Z;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (dist < 0.001f) dist = 1.0f; // ゼロ除算防止

		float nx = dx / dist;
		float ny = dy / dist;
		float nz = dz / dist;

		switch (data.trajectory) {
		case TrajectoryType::Straight:
			// 直線 : そのまま方向 × 速度
			proj.velocity = { nx * data.speed, ny * data.speed, nz * data.speed };
			break;

		case TrajectoryType::Parabola: {
			// 放物線 : X方向は水平速度、Y方向は到達に必要な初速を計算
			float horizontalDist = std::abs(dx);
			float flightTime = horizontalDist / (std::max)(data.speed, 0.1f);
			float vx = (dx > 0.0f ? 1.0f : -1.0f) * data.speed;
			// 必要な初速 vy = (dy + 0.5 * g * t^2) / t
			float vy = (dy + 0.5f * data.gravity * flightTime * flightTime) / (std::max)(flightTime, 0.01f);
			proj.velocity = { vx, vy, 0.0f };
			break;
		}

		case TrajectoryType::Homing:
			// ホーミング : 初期方向はターゲット方向
			proj.velocity = { nx * data.speed, ny * data.speed, nz * data.speed };
			break;
		}

		proj.InitCollider();

		// コリジョンコールバック: プレイヤーに当たったら消える、地面に当たったら消える
		proj.collider->onCollisionCallback = [id = proj.id](Collider* other, [[maybe_unused]] const Lumina::Math::F32x3& pushOut) {
			if (other->GetMyType() == COL_Player) {
				// プレイヤーにダメージを与える処理は Player 側の onCollision で行う
				// ここではプロジェクタイル側を死亡フラグセット
				auto* mgr = ProjectileManager::GetInstance();
				for (auto& p : const_cast<std::vector<Projectile>&>(mgr->GetAll())) {
					if (p.id == id) {
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

			// 弾道による挙動更新
			switch (proj.data.trajectory) {
			case TrajectoryType::Straight:
				// 直線: そのまま等速直線運動
				break;

			case TrajectoryType::Parabola:
				// 放物線: 重力を加える
				proj.velocity.Y -= proj.data.gravity * deltaTime;
				break;

			case TrajectoryType::Homing: {
				// ホーミング: プレイヤー方向に velocity をゆっくり向ける
				float dx = playerPosition.X - proj.position.X;
				float dy = playerPosition.Y - proj.position.Y;
				float dz = playerPosition.Z - proj.position.Z;
				float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
				if (dist > 0.01f) {
					float nx = dx / dist;
					float ny = dy / dist;
					float nz = dz / dist;

					float str = proj.data.homingStrength * deltaTime;
					proj.velocity.X += (nx * proj.data.speed - proj.velocity.X) * str;
					proj.velocity.Y += (ny * proj.data.speed - proj.velocity.Y) * str;
					proj.velocity.Z += (nz * proj.data.speed - proj.velocity.Z) * str;

					// 速度を一定に保つ（減速・加速しすぎないように）
					float spd = std::sqrt(
						proj.velocity.X * proj.velocity.X +
						proj.velocity.Y * proj.velocity.Y +
						proj.velocity.Z * proj.velocity.Z
					);
					if (spd > 0.01f) {
						float ratio = proj.data.speed / spd;
						proj.velocity.X *= ratio;
						proj.velocity.Y *= ratio;
						proj.velocity.Z *= ratio;
					}
				}
				break;
			}
			}

			// 位置更新
			proj.position.X += proj.velocity.X * deltaTime;
			proj.position.Y += proj.velocity.Y * deltaTime;
			proj.position.Z += proj.velocity.Z * deltaTime;

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

	void ProjectileManager::ClearAll() {
		projectiles_.clear();
		nextId_ = 1;
	}
}
