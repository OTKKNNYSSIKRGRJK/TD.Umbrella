export module Game.Editor.ObjMotionEditor;

import <string>;
import <vector>;
import <array>;
import <map>;

import Game.Editor.EnemyEditor;

export namespace Game::Editor {
	struct ObjMotionKeyframe {
		float time = 0.0f;
		float px = 0.0f;
		float py = 0.0f;
		float pz = 0.0f;
		float rx = 0.0f;
		float ry = 0.0f;
		float rz = 0.0f;
		float sx = 1.0f;
		float sy = 1.0f;
		float sz = 1.0f;
	};

	struct ObjMotionData {
		std::string name = "NewMotion";
		float duration = 1.0f;
		bool isLoop = true;
		std::string targetModelPath = "Models/Enemy/default.obj";
		std::vector<ObjMotionKeyframe> keyframes;

		void Reset() {
			name = "NewMotion";
			duration = 1.0f;
			isLoop = true;
			targetModelPath = "Models/Enemy/default.obj";
			keyframes.clear();
			ObjMotionKeyframe kf;
			kf.time = 0.0f; kf.px = 0.0f; kf.py = 0.0f; kf.pz = 0.0f; kf.rx = 0.0f; kf.ry = 0.0f; kf.rz = 0.0f; kf.sx = 1.0f; kf.sy = 1.0f; kf.sz = 1.0f;
			keyframes.push_back(kf);
			kf.time = 1.0f;
			keyframes.push_back(kf);
		}
	};

	class ObjMotionEditor {
	public:
		void Initialize();
		void Update();
		void LoadMotion(ObjMotionData& motion, const std::string& filename);

		const std::string& GetCachedMeshObjPath() const { return cachedMeshObjPath_; }
		const std::vector<std::array<float, 3>>& GetCachedMeshPositions() const { return cachedMeshPositions_; }
		const std::vector<std::array<int, 2>>& GetCachedMeshEdges() const { return cachedMeshEdges_; }
		const std::vector<std::array<int, 3>>& GetCachedMeshFaces() const { return cachedMeshFaces_; }

	private:
		void DrawEditorUI();
		void DrawPreviewCanvas();
		void DrawTimelineUI();
		void DrawKeyframeProperties();
		void SaveMotion(const ObjMotionData& motion);
		void ExtractMeshWireframe(const std::string& objPath);
		void AddKeyframe(float time);

		void LoadEnemy(const std::string& filename);

		void PushUndo();
		void Undo();
		void Redo();

	private:
		ObjMotionData editingMotion_{};
		int selectedKeyframeIndex_ = -1;
		
		std::vector<ObjMotionData> undoStack_;
		std::vector<ObjMotionData> redoStack_;

		float currentTime_ = 0.0f;
		bool isPlaying_ = false;

		std::string cachedMeshObjPath_;
		std::vector<std::array<float, 3>> cachedMeshPositions_;
		std::vector<std::array<int, 2>> cachedMeshEdges_;
		std::vector<std::array<int, 3>> cachedMeshFaces_;

		std::string editingEnemyFile_ = "";
		std::string currentActionName_ = "";
		EnemyData cachedEnemyData_{};

		float canvasOffsetX_ = 0.0f;
		float canvasOffsetY_ = 0.0f;
		float zoom_ = 3.0f;
		int meshViewMode_ = 0; // 0=Front, 1=Side, 2=Top
	};
}
