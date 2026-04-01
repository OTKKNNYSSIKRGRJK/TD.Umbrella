export module Game.Editor.AreaEditor;

import <memory>;
import <vector>;
import <string>;
import <map>;

import nlohmann.json;

import Lumina;
import Lumina.Core.Math;
import Lumina.Utils.Camera;
import Lumina.Utils.Misc;


export namespace Game::Editor {

	struct Vertex {
		Lumina::Math::F32x3 Pos;

		Vertex() : Pos{} {}
		Vertex(Lumina::Math::F32x3 const& p) : Pos{ p } {}
		Vertex& operator=(Lumina::Math::F32x3 const& p) { Pos = p; return *this; }
	};

	struct Vector2 {
		float x = 0.0f;
		float y = 0.0f;
	};

	struct RectCollision {
		Vector2 position{ 0.0f, 0.0f };
		Vector2 size{ 32.0f, 32.0f };
	};

	struct AreaConnection {
		int targetAreaIndex = 0;
		RectCollision trigger{};
	};

	struct EnemyPlacement {
		std::string enemyName = "NewEnemy";
		Vector2 position{ 0.0f, 0.0f };
		bool facingRight = true;
	};

	struct CollisionPoint {
		Vector2 position{ 0.0f, 0.0f };
		float radius = 16.0f;
	};

	struct CollisionGroup {
		std::string name = "NewGroup";
		std::vector<CollisionPoint> points;
	};

	struct AreaData {
		int name = 0;
		int index = 0;
		int width = 1280;
		int height = 720;
		std::string backgroundMusic = "";
		std::vector<AreaConnection> connections;
		std::vector<EnemyPlacement> enemies;
		std::vector<CollisionGroup> collisionGroups;
		Vector2 editorPos = { 0.0f, 0.0f };
		nlohmann::json originalJson{};

		void Reset() {
			name = 0;
			index = 0;
			width = 1280;
			height = 720;
			backgroundMusic.clear();
			connections.clear();
			enemies.clear();
			collisionGroups.clear();
			editorPos = { 0.0f, 0.0f };
			originalJson = nlohmann::json::object();
		}
	};

	class AreaEditor {
	public:
		void Initialize();
		void Update();
		void LoadArea(AreaData& area, const std::string& filename);

		auto ConvertToWorldCoordinate(
			std::vector<Vertex>& worldPosVertices_,
			std::vector<Vertex> const& screenPosVertices_,
			Lumina::Utils::Camera const& camera_,
			Lumina::Utils::Viewport const& viewport_
		) const -> void;

	private:
		void DrawEditorUI();
		void SaveArea(const AreaData& area, bool autoSyncConnections = false);
		void DeleteArea(int areaIndex);


	private:
		AreaData editingArea_{};
		std::vector<std::string> recentFiles_{};
		std::vector<AreaData> allAreas_{};
		Vector2 cameraPos_ = { 0.0f, 0.0f };

		int draggingAreaIndex_ = -1;
		int draggingConnectionIndex_ = -1;
		int draggingEnemyIndex_ = -1;
		int draggingCollisionGroupIndex_ = -1;
		int draggingCollisionPointIndex_ = -1;
		Vector2 dragOffset_ = { 0.0f, 0.0f };

		// 敵JSONファイルリスト（ドロップダウン用）
		std::vector<std::string> enemyFiles_;
	};
}
