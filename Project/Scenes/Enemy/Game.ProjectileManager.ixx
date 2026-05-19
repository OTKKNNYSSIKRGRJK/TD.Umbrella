export module Game.ProjectileManager;

import <string>;
import <memory>;
import <vector>;
import <functional>;

import Lumina;
import Collider;
import CollisionManager;
import Game.Editor.ActorEditor;
import Game.MotionManager;

export namespace Game {

	/// <summary>
	/// プロジェクタイル（弾）のテンプレートデータ
	/// EnemyData から参照される
	/// 見た目と軌道（ホーミング以外）は ActorEditor で作成した Actor から取得
	/// </summary>
	struct ProjectileData {
		// Actorアセット名（actor_<name>.json の <name> 部分）
		std::string actorName = "";

		// ホーミングオーバーライド（Actor の MovementModule を無視して追尾する）
		bool isHoming = false;
		float homingStrength = 3.0f;     // Homing 用 追尾の強さ

		// パラメータ（Actor とは独立）
		int   damage = 10;               // ダメージ
		float lifetime = 5.0f;           // 弾の生存時間（秒）
		float colliderRadius = 0.15f;    // 当たり判定の半径（立方体で近似）
        // If true, the projectile is spawned attached to its owner (no initial velocity)
		// and will follow the owner's position until activated.
		bool spawnAttached = false;
		Lumina::Math::F32x3 attachOffset{ 0.0f, 0.0f, 0.0f };
        // Charge/attach visual scaling
		bool scaleOnCharge = false;         // whether to grow visually while attached
		float initialScale = 0.25f;         // starting visual scale when attached
		float chargeGrowDuration = 0.8f;    // seconds to reach target visual scale
	};

	/// <summary>
	/// 飛行中のプロジェクタイル1つ分
	/// </summary>
	struct Projectile {
		ProjectileData data;
		Editor::ActorData actorData;     // ロードされた Actor データ（見た目・軌道）

		uint32_t id = 0;
		Lumina::Math::F32x3 position{ 0.0f, 0.0f, 0.0f };
		Lumina::Math::F32x3 velocity{ 0.0f, 0.0f, 0.0f };
		float aliveTime = 0.0f;
		bool isDead = false;
		uint32_t ownerEnemyId = 0;   // 発射した敵のID（自分に当たらないように）

		// If true, this projectile is visually attached to its owner and
		// will follow the owner's position until activated (shot).
		bool isAttached = false;
		
		// If true, this projectile is considered persistent equipment
		bool isEquipment = false;

		// Visual scaling state used during charge/attach
		float visualScale = 1.0f;
		float targetVisualScale = 1.0f;
		float chargeTimer = 0.0f;

		// PingPong 用: 移動距離トラッキング
		float traveledDistance = 0.0f;
		float pingPongDirection = 1.0f;

		// Spline 用: 発射位置（スプライン原点オフセット）
		Lumina::Math::F32x3 splineOrigin{ 0.0f, 0.0f, 0.0f };
		MotionController motionController;

		// 当たり判定
		std::unique_ptr<ConvexCollider> collider;

		/// <summary>
		/// コライダーの初期化
		/// </summary>
		void InitCollider();

		/// <summary>
		/// コライダーの位置更新
		/// </summary>
		void UpdateCollider();
	};

	/// <summary>
	/// プロジェクタイルの一括管理クラス（シングルトン）
	/// 
	/// ・発射（Fire）
	/// ・毎フレーム更新（弾道計算、寿命管理）
	/// ・コリジョン登録
	/// ・描画用データの提供
	/// </summary>
	class ProjectileManager {
	public:
		static ProjectileManager* GetInstance();

		// Attempt to activate an attached projectile belonging to `ownerEnemyId`.
		// If an attached projectile exists, convert it into a normal flying projectile
		// aimed at `target` and return true. Otherwise return false.
		bool ActivateAttachedProjectile(uint32_t ownerEnemyId, const Lumina::Math::F32x3& target);

		// ============================
		//  発射
		// ============================

		/// <summary>
		/// 指定位置からターゲットに向けてプロジェクタイルを発射
		/// </summary>
		/// <param name="origin">発射位置</param>
		/// <param name="target">狙う位置（Homing では毎フレーム更新）</param>
		/// <param name="data">弾のテンプレートデータ</param>
		/// <param name="ownerEnemyId">発射者の敵ID</param>
		void Fire(
			const Lumina::Math::F32x3& origin,
			const Lumina::Math::F32x3& target,
			const ProjectileData& data,
			uint32_t ownerEnemyId = 0
		);

		// ============================
		//  更新
		// ============================

		/// <summary>
		/// 全プロジェクタイルを毎フレーム更新
		/// </summary>
		/// <param name="deltaTime">経過秒数</param>
		/// <param name="playerPosition">ホーミング用プレイヤー位置</param>
		void Update(float deltaTime, const Lumina::Math::F32x3& playerPosition);

		// ============================
		//  コリジョン登録
		// ============================

		/// <summary>
		/// 生存中の全プロジェクタイルのコライダーを CollisionManager に登録
		/// </summary>
		void RegisterCollidersTo(CollisionManager& cm);

		// ============================
		//  アクセサ
		// ============================

		/// <summary>
		/// 全プロジェクタイル取得（描画用）
		/// </summary>
		const std::vector<Projectile>& GetAll() const { return projectiles_; }

		/// <summary>
		/// 死亡済みプロジェクタイルを除去
		/// </summary>
		void RemoveDeadProjectiles();

		/// <summary>
		/// 全プロジェクタイルをクリア
		/// </summary>
		void ClearAll();

		/// <summary>
		/// Remove any projectile marked as equipment for the specified owner.
		/// </summary>
		void RemoveEquipment(uint32_t ownerEnemyId);

	public:
		~ProjectileManager() = default;

	private:
		ProjectileManager() = default;

		uint32_t GenerateId();

	private:
		static std::unique_ptr<ProjectileManager> instance_;
		std::vector<Projectile> projectiles_;
		uint32_t nextId_ = 1;
	};
}
