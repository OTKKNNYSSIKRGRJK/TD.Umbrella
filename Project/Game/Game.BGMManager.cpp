module Game.BGMManager;

import Game.Editor.AudioEditor;
import Lumina.Main;
import Lumina.ResourceManager;
import <optional>;
import <map>;
import <string>;

namespace Game {

	// キャッシュ用データ群
	static Game::Editor::AudioData s_bgmData;
	static bool s_isLoaded = false;
	
	// キャッシュしたストリーム群 (filePath -> stream)
	static std::map<std::string, Lumina::AudioStreamHandle> s_streamCache;
	
	static std::optional<Lumina::AudioStreamPlayerHandle> s_currentBGMHandle;

	void BGMManager::PlaySceneBGM(const std::string& sceneName) {
		// 初回呼び出し時のみJSONを1回だけ読み込む
		if (!s_isLoaded) {
			Game::Editor::AudioEditor editor;
			editor.LoadAudio(s_bgmData, "Assets/Data/SceneBGM.json");
			s_isLoaded = true;
		}

		// 今鳴っているBGMがあれば止める
		StopCurrentBGM();

		// JSON内に該当のシーンデータがあれば再生を開始
		if (s_bgmData.bgmMap.contains(sceneName)) {
			const auto& config = s_bgmData.bgmMap[sceneName];
			
			if (config.filePath.empty()) return;

			auto& audioContext = Lumina::Context::Instance().ResourceContext().Audio();
			
			// キャッシュからロード、存在しなければ新規ロード
			Lumina::AudioStreamHandle stream;
			if (s_streamCache.contains(config.filePath)) {
				stream = s_streamCache[config.filePath];
			} else {
				stream = audioContext.LoadFromFile(config.filePath);
				s_streamCache[config.filePath] = stream;
			}
			
			s_currentBGMHandle = audioContext.Play(stream, config.isLoop, config.volume);
		}
	}

	void BGMManager::StopCurrentBGM() {
		if (s_currentBGMHandle.has_value()) {
			auto& audioContext = Lumina::Context::Instance().ResourceContext().Audio();
			audioContext.Stop(s_currentBGMHandle.value());
			s_currentBGMHandle.reset();
		}
	}
}
