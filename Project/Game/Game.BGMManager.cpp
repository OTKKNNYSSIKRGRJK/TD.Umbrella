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

	// 追加の遅延ループ制御用ステート
	static std::string s_currentSceneName = "";
	static bool s_isWaitingForLoop = false;
	static float s_loopTimer = 0.0f;

	void BGMManager::PlaySceneBGM(const std::string& sceneName) {
		// 初回呼び出し時のみJSONを1回だけ読み込む
		if (!s_isLoaded) {
			Game::Editor::AudioEditor editor;
			editor.LoadAudio(s_bgmData, "Assets/Data/SceneBGM.json");
			s_isLoaded = true;
		}

		// 同じBGMが既に再生中（または遅延待ち中）ならリスタートしない
		if (s_currentSceneName == sceneName) return;

		// 今鳴っているBGMがあれば止める
		StopCurrentBGM();

		s_currentSceneName = sceneName;
		s_isWaitingForLoop = false;
		s_loopTimer = 0.0f;

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
			
			// loopDelay が 0 のときはギャップレスのために XAudio2 のハードウェアループを使用
			bool useNativeLoop = config.isLoop && (config.loopDelay <= 0.0f);
			s_currentBGMHandle = audioContext.Play(stream, useNativeLoop, config.volume);
		}
	}

	void BGMManager::StopCurrentBGM() {
		if (s_currentBGMHandle.has_value()) {
			auto& audioContext = Lumina::Context::Instance().ResourceContext().Audio();
			audioContext.Stop(s_currentBGMHandle.value());
			s_currentBGMHandle.reset();
		}
		s_currentSceneName = "";
		s_isWaitingForLoop = false;
		s_loopTimer = 0.0f;
	}

	void BGMManager::PlayOneShot(const std::string& filePath, float volume) {
		if (filePath.empty()) return;

		auto& audioContext = Lumina::Context::Instance().ResourceContext().Audio();

		Lumina::AudioStreamHandle stream;
		if (s_streamCache.contains(filePath)) {
			stream = s_streamCache[filePath];
		} else {
			stream = audioContext.LoadFromFile(filePath);
			s_streamCache[filePath] = stream;
		}

		audioContext.Play(stream, false, volume);
	}

	void BGMManager::Update(float deltaTime) {
		if (s_currentSceneName.empty()) return;
		if (!s_bgmData.bgmMap.contains(s_currentSceneName)) return;

		const auto& config = s_bgmData.bgmMap[s_currentSceneName];
		
		// ループ指定がないか、または loopDelay が 0（ネイティブループ）の場合は更新処理不要
		if (!config.isLoop || config.loopDelay <= 0.0f) return;

		auto& audioContext = Lumina::Context::Instance().ResourceContext().Audio();

		if (s_isWaitingForLoop) {
			s_loopTimer += deltaTime;
			if (s_loopTimer >= config.loopDelay) {
				s_isWaitingForLoop = false;
				s_loopTimer = 0.0f;

				if (s_streamCache.contains(config.filePath)) {
					auto stream = s_streamCache[config.filePath];
					s_currentBGMHandle = audioContext.Play(stream, false, config.volume);
				}
			}
		} else {
			if (s_currentBGMHandle.has_value()) {
				// 曲が終了した（再生中でなくなった）かをチェック
				if (!audioContext.IsPlaying(s_currentBGMHandle.value())) {
					s_isWaitingForLoop = true;
					s_loopTimer = 0.0f;
					s_currentBGMHandle.reset();
				}
			}
		}
	}
}
