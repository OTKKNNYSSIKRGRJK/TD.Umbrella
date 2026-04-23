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

	private:
		BGMManager() = default;
		~BGMManager() = default;
	};

}
