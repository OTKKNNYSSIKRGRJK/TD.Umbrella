export module Game.BGMManager;

import <string>;

export namespace Game {

	class BGMManager {
	public:
		static BGMManager* GetInstance() {
			static BGMManager instance;
			return &instance;
		}

		// 指定したシーン名のBGMを鳴らします（前回鳴っていたものは自動で止まります）
		void PlaySceneBGM(const std::string& sceneName);

		// 現在鳴っているBGMを停止します
		void StopCurrentBGM();

		// 指定した音声ファイルを1回だけ再生します（SE用）
		void PlayOneShot(const std::string& filePath, float volume = 1.0f);

		// BGMの遅延ループなどを更新します
		void Update(float deltaTime);

	private:
		BGMManager() = default;
		~BGMManager() = default;
	};

}
