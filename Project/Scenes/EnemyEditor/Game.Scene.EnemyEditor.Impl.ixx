export module Game.Editor.EnemyEditor;

import <memory>;
import <string>;
import <map>;
import <vector>;

import Lumina;

export namespace Game::Editor {

	struct EnemyData {
		std::string name = "NewEnemy";
		int hp = 100;
		float power = 1.0f;
		std::string gltfPath = "Models/Enemy/default.gltf";

		std::map<std::string, std::string> animationMap = {
			{"Idle", ""}, {"Walk", ""}, {"Attack", ""}
		};

		// 各アクションに対応するモーション（座標移動）名
		std::map<std::string, std::string> motionMap = {
			{"Idle", ""}, {"Walk", ""}, {"Attack", ""}
		};

		// --- AI Parameters ---
		float aggroRadius = 15.0f;       // 索敵範囲
		float attackRange = 2.0f;        // 攻撃可能距離
		float moveSpeed = 3.0f;          // 移動速度
		float attackCooldown = 1.5f;     // 攻撃間隔（秒）
		float retreatThreshold = 0.2f;   // 撤退するHP割合 (0.0 ~ 1.0)
		float patrolRadius = 10.0f;      // 巡回範囲
		float aggressiveness = 0.5f;     // 攻撃的傾向 (0.0 ~ 1.0)

		void Reset() {
			name = "NewEnemy";
			hp = 100;
			power = 1.0f;
			gltfPath = "Models/Enemy/default.gltf";
			for (auto& [key, val] : animationMap) val = "";
			motionMap = { {"Idle", ""}, {"Walk", ""}, {"Attack", ""} };
			aggroRadius = 15.0f;
			attackRange = 2.0f;
			moveSpeed = 3.0f;
			attackCooldown = 1.5f;
			retreatThreshold = 0.2f;
			patrolRadius = 10.0f;
			aggressiveness = 0.5f;
		}
	};

	class EnemyEditor {
	public:
		void Initialize();
		void Update();
		void LoadEnemy(EnemyData& enemy, const std::string& filename);

	private:
		void DrawEditorUI();
		void SaveEnemy(const EnemyData& enemy);
		std::vector<std::string> ExtractAnimationNames(const std::string& gltfPath);

	private:
		EnemyData editingEnemy_{};

		// アニメーション名キャッシュ（gltfPath変更時のみ再取得）
		std::string cachedGltfPath_;
		std::vector<std::string> cachedAnimationNames_;
	};
}
