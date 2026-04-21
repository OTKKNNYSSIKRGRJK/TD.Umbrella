export module Game.Editor.EnemyEditor;

import <memory>;
import <string>;
import <map>;
import <vector>;
import <array>;

import Lumina;
import Game.ProjectileManager;

export namespace Game::Editor {

	// 当たり判定ポリゴンの頂点（敵の原点からの相対座標）
	struct CollisionVertex {
		float x = 0.0f;
		float y = 0.0f;
	};

	struct Node {
		int id = 0;
		std::string name = "State";
		std::string state = "Idle";
		float x = 0.0f;
		float y = 0.0f;
		std::string animationName = "";
		// Debug: bound motion and node index for runtime hooks (e.g. walk trigger)
		std::string boundMotion = "";
		int boundMotionNodeIndex = -1;
		std::string boundBool = "";
	};

	struct Link {
		int from = 0;
		int to = 0;
		std::string condition = "Always";
	};

	// サイズ段階ごとのステータス（小・中・大）
	struct SizeTier {
		int hp = 100;
		float power = 1.0f;
		float scale = 0.5f;  // 3Dモデルのスケール倍率
	};

	struct EnemyData {
		std::string name = "NewEnemy";
		int hp = 100;
		float power = 1.0f;
		std::string gltfPath = "Models/Enemy/default.gltf";

		// --- サイズ段階 (0=Small, 1=Medium, 2=Large) ---
		std::array<SizeTier, 3> sizeTiers = {{
			{  35, 0.8f, 0.3f },  // Small
			{  70, 1.0f, 0.5f },  // Medium
			{ 140, 1.8f, 0.7f },  // Large
		}};

		std::map<std::string, std::string> animationMap = {
			{"Idle", ""}, {"Walk", ""}, {"Attack", ""}
		};

		// 各アクションに対応するモーション（座標移動）名
		std::map<std::string, std::string> motionMap = {
			{"Idle", ""}, {"Walk", ""}, {"Attack", ""}
		};

		// --- 当たり判定（ポリゴン頂点リスト） ---
		// 頂点を順番に結んだ多角形が当たり判定になる
		std::vector<CollisionVertex> collisionVertices;

		// --- AI Parameters ---
		float aggroRadius = 15.0f;       // 索敵範囲
		float attackRange = 2.0f;        // 攻撃可能距離
		float moveSpeed = 3.0f;          // 移動速度
		float attackCooldown = 1.5f;     // 攻撃間隔（秒）
		float retreatThreshold = 0.2f;   // 撤退するHP割合 (0.0 ~ 1.0)
		float patrolRadius = 10.0f;      // 巡回範囲
		float aggressiveness = 0.5f;     // 攻撃的傾向 (0.0 ~ 1.0)

		// --- 攻撃タイプ ---
		enum class AttackType { Melee, Ranged };
		AttackType attackType = AttackType::Melee;

		// --- 遠距離攻撃用プロジェクタイル設定 ---
		Game::ProjectileData projectile;

		std::vector<Node> nodes;
		std::vector<Link> links;

		void Reset() {
			name = "NewEnemy";
			hp = 100;
			power = 1.0f;
			gltfPath = "Models/Enemy/default.gltf";
			sizeTiers = {{
				{  35, 0.8f, 0.3f },
				{  70, 1.0f, 0.5f },
				{ 140, 1.8f, 0.7f },
			}};
			for (auto& [key, val] : animationMap) val = "";
			motionMap = { {"Idle", ""}, {"Walk", ""}, {"Attack", ""} };
			collisionVertices.clear();
			aggroRadius = 15.0f;
			attackRange = 2.0f;
			moveSpeed = 3.0f;
			attackCooldown = 1.5f;
			retreatThreshold = 0.2f;
			patrolRadius = 10.0f;
			aggressiveness = 0.5f;
			attackType = AttackType::Melee;
			projectile = Game::ProjectileData{};
			nodes.clear();
			links.clear();
		}
	};

	class EnemyEditor {
	public:
		void Initialize();
		void Update();
		void LoadEnemy(EnemyData& enemy, const std::string& filename);

	private:
		void DrawEditorUI();
		void DrawCollisionEditor();
		void SaveEnemy(const EnemyData& enemy);
		std::vector<std::string> ExtractAnimationNames(const std::string& gltfPath);

	public:
		void ExtractMeshWireframe(const std::string& gltfPath);
		const std::string& GetCachedMeshGltfPath() const { return cachedMeshGltfPath_; }
		const std::vector<std::array<float, 3>>& GetCachedMeshPositions() const { return cachedMeshPositions_; }
		const std::vector<std::array<int, 2>>& GetCachedMeshEdges() const { return cachedMeshEdges_; }
		const std::vector<std::array<int, 3>>& GetCachedMeshFaces() const { return cachedMeshFaces_; }

	private:
		EnemyData editingEnemy_{};

		// アニメーション名キャッシュ（gltfPath変更時のみ再取得）
		std::string cachedGltfPath_;
		std::vector<std::string> cachedAnimationNames_;

		// メッシュワイヤーフレームキャッシュ
		std::string cachedMeshGltfPath_;
		std::vector<std::array<float, 3>> cachedMeshPositions_;  // 3D頂点座標
		std::vector<std::array<int, 2>> cachedMeshEdges_;        // エッジ（頂点インデックスペア）
		std::vector<std::array<int, 3>> cachedMeshFaces_;        // 三角形ポリゴン（頂点インデックス3つ）

		// 当たり判定エディタ状態
		int draggedVertexIndex_ = -1;    // ドラッグ中の頂点インデックス
		float collisionZoom_ = 3.0f;     // キャンバスのズーム倍率

		// ワイヤーフレームビューモード (0=正面XY, 1=側面ZY, 2=上面XZ)
		int meshViewMode_ = 0;
		bool showMeshWireframe_ = true;

		// キャンバス移動オフセット
		float canvasOffsetX_ = 0.0f;
		float canvasOffsetY_ = 0.0f;
	};

	export class EnemyActionEditor {
	public:
		void Initialize();
		void Update();
		void LoadEnemy(EnemyData& enemy, const std::string& filename);

		// --- Public accessors for Inspector ---
		int GetSelectedNodeId() const { return nodeEditor_selectedNodeId_; }
		Node* FindNodeById(int id) {
			for (auto& n : editingEnemy_.nodes) {
				if (n.id == id) return &n;
			}
			return nullptr;
		}
	private:
		void DrawEditorUI();
		void DrawNodeEditor();
		void SaveEnemy(const EnemyData& enemy);
		std::vector<std::string> ExtractAnimationNames(const std::string& gltfPath);

		// --- State Machine Runtime ---
		void EvaluateStateMachine();
		bool CheckLinkCondition(const Link& link);
		bool HasOutgoingTransition(int nodeId) const;
		bool HasTimeDrivenTransition(int nodeId) const;
		void DrawStateMachineControlUI();
		void DrawLinkConditionList();
		void DrawEditorLogUI();

		// --- Undo ---
		void PushUndoState();
		void Undo();
		bool CanUndo() const { return !undoStack_.empty(); }

		// --- Log ---
		void AddLog(const std::string& msg);

	private:
		// Active editing enemy (holds current visible data). Persisted per-file in perFileEnemies_.
		EnemyData editingEnemy_{};
		std::string cachedGltfPath_;
		std::vector<std::string> cachedAnimationNames_;

		// Per-file storage so each JSON keeps its own nodes, links and runtime state
		struct PerFileRuntime {
			std::vector<std::string> cachedAnimationNames;
			int currentStateId = -1;
			float currentStateElapsedTime = 0.0f;
			int previousStateId = -1;
			float transitionFlashTimer = 0.0f;
			bool firstNodeStarted = false;
			std::vector<EnemyData> undoStack; // keep per-file undo history
			std::map<std::string, bool> runtimeBoolFlags; // keep per-file boolean flags
		};

		std::map<std::string, EnemyData> perFileEnemies_;
		std::map<std::string, PerFileRuntime> perFileRuntimes_;
		std::string activeFileName_; // includes ".json" suffix when set

		float canvasOffsetX_ = 0.0f;
		float canvasOffsetY_ = 0.0f;

		// --- Node Editor Status ---
		int currentStateId_ = -1;
		float currentStateElapsedTime_ = 0.0f;
		int nodeEditor_selectedNodeId_ = -1;
		int nodeEditor_linkStartId_ = -1;
		int nodeEditor_contextNodeId_ = -1;
		int nodeEditor_pendingDeleteNodeId_ = -1;
		int linkEditor_contextLinkIndex_ = -1;
		int linkEditor_pendingDeleteLinkIndex_ = -1;
		bool nodeDragActive_ = false;
		float nodeDragOffsetX_ = 0.0f;
		float nodeDragOffsetY_ = 0.0f;
		bool nodeLinkDragActive_ = false;
		float nodeCanvasWidth_ = 1220.0f;
		float nodeCanvasHeight_ = 180.0f;
		bool nodeCanvasResizing_ = false;
		float nodeCanvasResizeStartMouseX_ = 0.0f;
		float nodeCanvasResizeStartMouseY_ = 0.0f;
		float nodeCanvasStartWidth_ = 0.0f;
		float nodeCanvasStartHeight_ = 0.0f;
		float pendingNewNodeScreenX_ = 0.0f;
		float pendingNewNodeScreenY_ = 0.0f;

		// --- State Machine Runtime ---
		int previousStateId_ = -1;
		float transitionFlashTimer_ = 0.0f;

		// --- Manual Start ---
		bool requireManualStart_ = true;
		bool firstNodeStarted_ = false;
		bool lockStateMachineAfterStartFirstNode_ = false;
		bool userRequestedStart_ = false;

		// --- Runtime Bool Flags (for BOOL: link conditions) ---
		std::map<std::string, bool> runtimeBoolFlags_;

		// --- Undo ---
		std::vector<EnemyData> undoStack_;
		size_t undoStackMax_ = 64;

		// --- Editor Log ---
		std::vector<std::string> editorLog_;
		size_t editorLogMax_ = 512;
	};
}
