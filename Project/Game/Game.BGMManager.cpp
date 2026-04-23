module Game.BGMManager;

import Game.Editor.AudioEditor;
import Lumina.Main;
import Lumina.ResourceManager;



namespace Game {

	// キャッシュ用データ群
	static Game::Editor::AudioData s_bgmData;
	static bool s_isLoaded = false;
	
	static Lumina::AudioStreamPlayerHandle* s_currentBGMHandle = nullptr;

	void BGMManager::PlaySceneBGM(const std::string& sceneName) {
		// 初回呼び出し時のみJSONを1回だけ読み込む
		if (!s_isLoaded) {
			Game::Editor::AudioEditor editor;
			editor.LoadAudio(s_bgmData, "SceneBGM.json");
			s_isLoaded = true;
		}

		// 今鳴っているBGMがあれば止める
		StopCurrentBGM();

		// JSON内に該当のシーンデータがあれば再生を開始
		if (s_bgmData.bgmMap.contains(sceneName)) {
			const auto& config = s_bgmData.bgmMap[sceneName];
			
			if (config.filePath.empty()) return;

			auto& audioContext = Lumina::Context::Instance().ResourceContext().Audio();
			auto stream = audioContext.LoadFromFile(config.filePath);
			if (s_currentBGMHandle == nullptr) {
				s_currentBGMHandle = new Lumina::AudioStreamPlayerHandle();
			}
			*s_currentBGMHandle = audioContext.Play(stream, config.isLoop, config.volume);
		}
	}

	void BGMManager::StopCurrentBGM() {
		if (s_currentBGMHandle != nullptr) {
			auto& audioContext = Lumina::Context::Instance().ResourceContext().Audio();
			audioContext.Stop(*s_currentBGMHandle);
			delete s_currentBGMHandle;
			s_currentBGMHandle = nullptr;
		}
	}
}
