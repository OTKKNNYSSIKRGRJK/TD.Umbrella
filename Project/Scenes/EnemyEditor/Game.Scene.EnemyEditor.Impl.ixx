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

		void Reset() {
			name = "NewEnemy";
			hp = 100;
			power = 1.0f;
			gltfPath = "Models/Enemy/default.gltf";
			for (auto& [key, val] : animationMap) val = "";
		}
	};

	class EnemyEditor {
	public:
		void Initialize();
		void Update();

	private:
		void DrawEditorUI();
		void SaveEnemy(const EnemyData& enemy);
		void LoadEnemy(EnemyData& enemy, const std::string& filename);

	private:
		EnemyData editingEnemy_{};
	};
}
