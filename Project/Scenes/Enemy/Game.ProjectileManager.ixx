export module Game.ProjectileManager;

import <string>;
import <memory>;
import <vector>;
import <functional>;

import Lumina;
import Collider;
import CollisionManager;

export namespace Game {

	/// <summary>
	/// 弾道タイプ
	/// </summary>
	enum class TrajectoryType {
		Straight,   // 直線
		Parabola,   // 放物線（重力あり）
		Homing,     // ホーミング（プレイヤー追尾）
	};

	/// <summary>
	/// プロジェクタイル（弾）のテンプレートデータ
	/// EnemyData から参照される
	/// </summary>
	struct ProjectileData {
		// 見た目
		std::string meshName = "cube";   // 使用するメッシュ名（将来拡張）
		float scale = 0.15f;             // 弾のスケール

		// 弾道
		TrajectoryType trajectory = TrajectoryType::Straight;

		// パラメータ
		float speed = 8.0f;              // 初速
		float gravity = 9.8f;            // Parabola 用 下向き重力
		float homingStrength = 3.0f;     // Homing 用 追尾の強さ
		int   damage = 10;               // ダメージ
		float lifetime = 5.0f;           // 弾の生存時間（秒）
		float colliderRadius = 0.15f;    // 当たり判定の半径（立方体で近似）
	};

	/// <summary>
	/// 飛行中のプロジェクタイル1つ分
	/// </summary>
	struct Projectile {
		ProjectileData data;
		uint32_t id = 0;
		Lumina::Math::F32x3 position{ 0.0f, 0.0f, 0.0f };
		Lumina::Math::F32x3 velocity{ 0.0f, 0.0f, 0.0f };
		float aliveTime = 0.0f;
		bool isDead = false;
		uint32_t ownerEnemyId = 0;   // 発射した敵のID（自分に当たらないように）

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
