export module Game.Editor.ActorEditor;

import <memory>;
import <string>;
import <vector>;
import <array>;

import Lumina;
import Hermite;

export namespace Game::Editor {

	// ============================================================
	// Transform
	// ============================================================
	struct ActorTransform {
		float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
		float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
		float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;
	};

	// ============================================================
	// Visual
	// ============================================================
	struct ActorVisual {
		std::string meshPath = "";
		int materialIndex = 0;
	};

	// ============================================================
	// Collider
	// ============================================================
	enum class ColliderType { None, Box, Sphere };

	struct ActorCollider {
		ColliderType type = ColliderType::None;
		float sizeX = 1.0f, sizeY = 1.0f, sizeZ = 1.0f;
	};

	// ============================================================
	// Movement Module
	// ============================================================
	enum class MovementType { None, Linear, PingPong, Spline };
	enum class EasingType { Linear, Sine, EaseInOut };

	// 各ノードの通過タイミング（曲線データ自体はMotionEditorのファイルを参照）
	struct NodeTiming {
		float arrivalTime = 0.0f;               // この点に到達する時間（秒）
		EasingType easing = EasingType::Linear;  // この区間のイージング
	};

	struct MovementModule {
		MovementType type = MovementType::None;

		// Linear / PingPong 用
		float speed = 1.0f;
		float dirX = 1.0f, dirY = 0.0f, dirZ = 0.0f;
		float range = 5.0f;
		EasingType easing = EasingType::Linear;

		// Spline 用 — MotionEditorで作った曲線を名前で参照
		std::string splineMotionName = "";         // Assets/Data/Motion/ 内のファイル名
		float totalDuration = 3.0f;                // 全体の再生時間（秒）
		bool  loopSpline = false;                  // ループ再生
		std::vector<NodeTiming> nodeTimings;        // 各ノードの通過タイミング
	};

	// ============================================================
	// Interaction Module
	// ============================================================
	enum class InteractionType { None, DamageDealer, SolidPlatform, Trigger };

	struct InteractionModule {
		InteractionType type = InteractionType::None;
		float damageValue = 10.0f;
		float pushForce = 5.0f;
		std::string activationTriggerID = "";
	};

	// ============================================================
	// Lifecycle & Timing
	// ============================================================
	enum class SpawnTriggerType { Time, Distance, Event };

	struct LifecycleModule {
		SpawnTriggerType spawnTrigger = SpawnTriggerType::Time;
		float spawnValue = 0.0f;
		std::string spawnEventID = "";
		float lifetime = 10.0f;
		bool autoDestroyOffscreen = true;
	};

	// ============================================================
	// Actor Data
	// ============================================================
	struct ActorData {
		std::string name = "NewActor";
		ActorTransform transform{};
		ActorVisual visual{};
		ActorCollider collider{};
		MovementModule movement{};
		InteractionModule interaction{};
		LifecycleModule lifecycle{};

		void Reset() {
			name = "NewActor";
			transform = {};
			visual = {};
			collider = {};
			movement = {};
			interaction = {};
			lifecycle = {};
		}
	};

	// ============================================================
	// Actor Editor Class
	// ============================================================
	class ActorEditor {
	public:
		void Initialize();
		void Update();
		void LoadActor(ActorData& actor, const std::string& filename);

	private:
		void DrawEditorUI();
		void DrawPreviewCanvas();
		void SaveActor(const ActorData& actor);
		void ScanMotionFiles();
		void SyncNodeTimings();  // motionデータとnodeTimingsの数を同期

	private:
		ActorData editingActor_{};

		// プレビューキャンバス状態
		float previewZoom_ = 3.0f;
		float canvasOffsetX_ = 0.0f;
		float canvasOffsetY_ = 0.0f;

		// アニメーション時間 (Movement プレビュー用)
		float previewTime_ = 0.0f;
		bool  previewPlaying_ = false;

		// モーションファイルリスト
		std::vector<std::string> motionFiles_;

		// 読み込んだ曲線データのキャッシュ（表示・プレビュー用）
		std::string cachedMotionName_;
		std::vector<MathUtils::Spline::Node<Lumina::Math::F32x3>> cachedNodes_;
	};
}
