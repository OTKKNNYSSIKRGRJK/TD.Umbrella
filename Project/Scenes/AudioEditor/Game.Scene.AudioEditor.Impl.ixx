export module Game.Editor.AudioEditor;

import <string>;
import <vector>;
import <map>;

import Lumina.ResourceManager;

export namespace Game::Editor {

	struct SceneBGMData {
		std::string filePath = "";
		bool isLoop = true;
		float volume = 1.0f;
	};

	struct AudioData {
		std::string name = "SceneBGM";
		std::map<std::string, SceneBGMData> bgmMap;

		void Reset() {
			name = "SceneBGM";
			bgmMap.clear();
		}
	};

	class AudioEditor {
	public:
		void Initialize();
		void Update();
		void LoadAudio(AudioData& audio, const std::string& filename);

	private:
		void DrawEditorUI();
		void SaveAudio(const AudioData& audio);

	private:
		AudioData editingAudio_{};
		
		std::string previewSceneName_ = "";
		Lumina::AudioStreamPlayerHandle* previewPlayerHandle_ = nullptr;
		bool isPreviewPlaying_ = false;
	};
}
