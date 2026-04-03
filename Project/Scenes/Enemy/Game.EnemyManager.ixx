export module Game.EnemyManager;

import <string>;
import <memory>;
import <unordered_map>;
import <vector>;
import <functional>;
import <algorithm>;

import Lumina;
import Game.Editor.EnemyEditor;
import Collider;
import CollisionManager;

export namespace Game {

	/// <summary>
	/// ゲーム内で実際に動く敵インスタンス
	/// </summary>
	struct EnemyInstance {
		// --- コンストラクタ / 代入演算子 ---
		EnemyInstance() = default;
		~EnemyInstance() = default;

		// ムーブ時もコライダーを再生成して 'this' キャプチャを更新する
		EnemyInstance(EnemyInstance&& other) noexcept
			: baseData(std::move(other.baseData))
			, id(other.id)
			, position(other.position)
			, velocity(other.velocity)
			, currentHP(other.currentHP)
			, isDead(other.isDead)
			, facingRight(other.facingRight)
			, sizeTier(other.sizeTier)
			, modelScale(other.modelScale)
			, hurtTimer(other.hurtTimer)
			, preAttackTimer(other.preAttackTimer)
			, attackTimer(other.attackTimer)
			, landingStunTimer(other.landingStunTimer)
			, attackWindupDuration(other.attackWindupDuration)
			, attackDuration(other.attackDuration)
			, burstSpeedMultiplier(other.burstSpeedMultiplier)
			, preferredCombatDistance(other.preferredCombatDistance)
			, strafeDirection(other.strafeDirection)
			, aiState(other.aiState)
			, attackCooldownTimer(other.attackCooldownTimer)
			, stateTimer(other.stateTimer)
			, currentAction(std::move(other.currentAction))
		{
			if (!other.colliders.empty()) {
				InitCollider();
			}
		}

		EnemyInstance& operator=(EnemyInstance&& other) noexcept {
			if (this != &other) {
				baseData = std::move(other.baseData);
				id = other.id;
				position = other.position;
				velocity = other.velocity;
				currentHP = other.currentHP;
				isDead = other.isDead;
				facingRight = other.facingRight;
				sizeTier = other.sizeTier;
				modelScale = other.modelScale;
				hurtTimer = other.hurtTimer;
				preAttackTimer = other.preAttackTimer;
				attackTimer = other.attackTimer;
				landingStunTimer = other.landingStunTimer;
				attackWindupDuration = other.attackWindupDuration;
				attackDuration = other.attackDuration;
				burstSpeedMultiplier = other.burstSpeedMultiplier;
				preferredCombatDistance = other.preferredCombatDistance;
				strafeDirection = other.strafeDirection;
				aiState = other.aiState;
				attackCooldownTimer = other.attackCooldownTimer;
				stateTimer = other.stateTimer;
				currentAction = std::move(other.currentAction);
				colliders.clear();
				if (!other.colliders.empty()) {
					InitCollider();
				}
			}
			return *this;
		}

		// コピー時はコライダーを除いてコピーし、後で InitCollider() で再生成する
		EnemyInstance(const EnemyInstance& other)
			: baseData(other.baseData)
			, id(other.id)
			, position(other.position)
			, velocity(other.velocity)
			, currentHP(other.currentHP)
			, isDead(other.isDead)
			, facingRight(other.facingRight)
			, sizeTier(other.sizeTier)
			, modelScale(other.modelScale)
			, hurtTimer(other.hurtTimer)
			, preAttackTimer(other.preAttackTimer)
			, attackTimer(other.attackTimer)
			, landingStunTimer(other.landingStunTimer)
			, attackWindupDuration(other.attackWindupDuration)
			, attackDuration(other.attackDuration)
			, burstSpeedMultiplier(other.burstSpeedMultiplier)
			, preferredCombatDistance(other.preferredCombatDistance)
			, strafeDirection(other.strafeDirection)
			, aiState(other.aiState)
			, attackCooldownTimer(other.attackCooldownTimer)
			, stateTimer(other.stateTimer)
			, currentAction(other.currentAction)
			// colliders は再生成する
		{
			if (!other.colliders.empty()) {
				InitCollider();
			}
		}

		EnemyInstance& operator=(const EnemyInstance& other) {
			if (this != &other) {
				baseData = other.baseData;
				id = other.id;
				position = other.position;
				velocity = other.velocity;
				currentHP = other.currentHP;
				isDead = other.isDead;
				facingRight = other.facingRight;
				sizeTier = other.sizeTier;
				modelScale = other.modelScale;
				hurtTimer = other.hurtTimer;
				preAttackTimer = other.preAttackTimer;
				attackTimer = other.attackTimer;
				landingStunTimer = other.landingStunTimer;
				attackWindupDuration = other.attackWindupDuration;
				attackDuration = other.attackDuration;
				burstSpeedMultiplier = other.burstSpeedMultiplier;
				preferredCombatDistance = other.preferredCombatDistance;
				strafeDirection = other.strafeDirection;
				aiState = other.aiState;
				attackCooldownTimer = other.attackCooldownTimer;
				stateTimer = other.stateTimer;
				currentAction = other.currentAction;
				colliders.clear();
				if (!other.colliders.empty()) {
					InitCollider();
				}
			}
			return *this;
		}

		// --- テンプレートデータ（EnemyEditorから読み込み） ---
		Editor::EnemyData baseData;

		// --- ランタイム状態 ---
		uint32_t id = 0;                              // ユニークID
		Lumina::Math::F32x3 position{ 0.0f, 0.0f, 0.0f };
		Lumina::Math::F32x3 velocity{ 0.0f, 0.0f, 0.0f };
		int currentHP = 0;
		bool isDead = false;
		bool facingRight = true;
		int sizeTier = 1;
		float modelScale = 1.0f;	// サイズ段階のスケール倍率
		float hurtTimer = 0.0f;
		float preAttackTimer = 0.0f;
		float attackTimer = 0.0f;
		float landingStunTimer = 0.0f;
		float attackWindupDuration = 0.4f;
		float attackDuration = 0.25f;
		float burstSpeedMultiplier = 1.0f;
		float preferredCombatDistance = 2.0f;
		float strafeDirection = 1.0f;
		// ガード: 同一フレーム中の重複ダメージを防ぐ
		bool recentlyDamagedThisFrame = false;

		// --- AI 状態 ---
		enum class AIState { Idle, Patrol, Chase, PreAttack, Attack, Retreat };
		AIState aiState = AIState::Idle;
		float attackCooldownTimer = 0.0f;
		float stateTimer = 0.0f;                      // 現在の状態維持タイマー

		// --- アニメーション ---
		std::string currentAction = "Idle";            // 現在のアクション名

		// --- 当たり判定（凸包分割された複数のConvexCollider） ---
		std::vector<std::unique_ptr<ConvexCollider>> colliders;

		/// <summary>
		/// baseData の値で初期化する
		/// </summary>
		void InitFromBase() {
			ApplySizeTier(sizeTier);
			isDead = false;
			hurtTimer = 0.0f;
			preAttackTimer = 0.0f;
			attackTimer = 0.0f;
			landingStunTimer = 0.0f;
			attackCooldownTimer = 0.0f;
			stateTimer = 0.0f;
			aiState = AIState::Idle;
			currentAction = "Idle";
			burstSpeedMultiplier = 1.0f;
			preferredCombatDistance = baseData.attackRange;
			attackWindupDuration = 0.4f;
			attackDuration = 0.25f;
			strafeDirection = 1.0f;
		}

		/// <summary>
		/// エディタで定義した collisionVertices (2D) から
		/// ConvexCollider (3D) を初期化する。
		/// 凹多角形の場合は Ear Clipping で三角形に分割し、
		/// 各三角形ごとに ConvexCollider を作成する。
		/// </summary>
		void InitCollider();

		/// <summary>
		/// 全コライダーのワールド座標を現在の position に更新する
		/// </summary>
		void UpdateCollider();

		void ApplySizeTier(int tier) {
			sizeTier = (std::max)(0, (std::min)(2, tier));
			currentHP = baseData.sizeTiers[sizeTier].hp;
			baseData.hp = currentHP;
			baseData.power = baseData.sizeTiers[sizeTier].power;
			modelScale = baseData.sizeTiers[sizeTier].scale;
		}
	};

	/// <summary>
	/// EnemyEditor で作成した敵データを包括的に管理するクラス
	///
	/// ・敵テンプレート（JSON）の一括読み込み・キャッシュ
	/// ・敵インスタンスのスポーン／破棄／全体管理
	/// ・毎フレーム更新（AI・タイマー・死亡判定）
	/// ・条件検索・コールバック
	/// </summary>
	class EnemyManager {
	public:
		static EnemyManager* GetInstance();

		// ============================
		//  テンプレート管理
		// ============================

		/// <summary>
		/// 指定ディレクトリ内の全 .json をテンプレートとして一括ロード
		/// （"area" で始まるファイルは除外）
		/// </summary>
		void LoadTemplates(const std::string& directoryPath);

		/// <summary>
		/// 単一ファイルからテンプレートをロード（上書き可）
		/// </summary>
		void LoadTemplate(const std::string& filePath);

		/// <summary>
		/// 名前でテンプレートを取得（なければ nullptr）
		/// </summary>
		const Editor::EnemyData* GetTemplate(const std::string& name) const;

		/// <summary>
		/// ロード済みテンプレート名一覧を返す
		/// </summary>
		std::vector<std::string> GetTemplateNames() const;

		/// <summary>
		/// 全テンプレートをクリア
		/// </summary>
		void ClearTemplates();

		// ============================
		//  インスタンス管理
		// ============================

		/// <summary>
		/// テンプレート名と初期位置を指定してスポーン
		/// </summary>
		/// <returns>スポーンした EnemyInstance への参照（失敗時は nullptr）</returns>
		EnemyInstance* Spawn(const std::string& templateName,
			const Lumina::Math::F32x3& position,
			bool facingRight = true, float scale = 1.0f);

		/// <summary>
		/// 既存の EnemyData を直接渡してスポーン
		/// </summary>
		EnemyInstance* SpawnFromData(const Editor::EnemyData& data,
			const Lumina::Math::F32x3& position,
			bool facingRight = true, float scale = 1.0f, int sizeTier = 1);

		/// <summary>
		/// ID で インスタンス取得
		/// </summary>
		EnemyInstance* GetInstance(uint32_t id);
		const EnemyInstance* GetInstance(uint32_t id) const;

		/// <summary>
		/// 全インスタンス取得（読み取り用）
		/// </summary>
		const std::vector<EnemyInstance>& GetAllInstances() const { return instances_; }

		/// <summary>
		/// 全インスタンス取得（書き込み可）
		/// </summary>
		std::vector<EnemyInstance>& GetAllInstances() { return instances_; }

		/// <summary>
		/// 生存中の敵のみ取得
		/// </summary>
		std::vector<EnemyInstance*> GetAliveInstances();

		/// <summary>
		/// 生存中の敵の数
		/// </summary>
		int GetAliveCount() const;

		/// <summary>
		/// 全インスタンスをクリア
		/// </summary>
		void ClearInstances();

		// ============================
		//  コリジョン登録
		// ============================

		/// <summary>
		/// 生存中の全敵コライダーを指定の CollisionManager に登録する
		/// 毎フレーム Begin() の後、CheckAllCollisions() の前に呼ぶ
		/// </summary>
		void RegisterCollidersTo(CollisionManager& cm);

		/// <summary>
		/// 死亡済みインスタンスの除去
		/// </summary>
		void RemoveDeadInstances();

		// ============================
		//  更新
		// ============================

		/// <summary>
		/// 全インスタンスを毎フレーム更新
		/// (AI、タイマー、死亡判定など)
		/// </summary>
		/// <param name="deltaTime">フレームの経過秒数</param>
		/// <param name="playerPosition">プレイヤーの現在位置</param>
		void Update(float deltaTime, const Lumina::Math::F32x3& playerPosition);

		// ============================
		//  ダメージ・インタラクション
		// ============================

		/// <summary>
		/// IDで指定した敵にダメージを与える
		/// </summary>
		/// <returns>true if enemy died from this hit</returns>
		bool DealDamage(uint32_t enemyId, int damage);

		/// <summary>
		/// 範囲内の敵全てにダメージを与える
		/// </summary>
		/// <param name="origin">攻撃の中心座標</param>
		/// <param name="radius">攻撃範囲の半径</param>
		/// <param name="damage">ダメージ量</param>
		/// <param name="facingRight">攻撃方向（trueで右方向のみ判定）</param>
		/// <param name="directional">方向制限をかけるか</param>
		/// <returns>倒した敵の数</returns>
		int DealAreaDamage(const Lumina::Math::F32x3& origin, float radius,
			int damage, bool facingRight = true, bool directional = false);

		// ============================
		//  コールバック
		// ============================

		using OnEnemyDeathCallback = std::function<void(const EnemyInstance&)>;

		/// <summary>
		/// 敵が死亡した時に呼ばれるコールバックを設定
		/// </summary>
		void SetOnEnemyDeathCallback(OnEnemyDeathCallback callback);

	public:
		~EnemyManager() = default;

	private:
		EnemyManager() = default;

		uint32_t GenerateId();

	private:
		static std::unique_ptr<EnemyManager> instance_;

		// テンプレートキャッシュ（名前 → EnemyData）
		std::unordered_map<std::string, Editor::EnemyData> templates_;

		// 現在のインスタンス
		std::vector<EnemyInstance> instances_;

		// ID カウンタ
		uint32_t nextId_ = 1;

		// コールバック
		OnEnemyDeathCallback onDeathCallback_;
	};
}
