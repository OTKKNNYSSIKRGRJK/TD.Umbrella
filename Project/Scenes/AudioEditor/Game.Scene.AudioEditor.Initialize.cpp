module Game.Editor.AudioEditor;

import <fstream>;
import <filesystem>;
import <string>;

import nlohmann.json;

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Game::Editor {

	void to_json(json& j, const SceneBGMData& s) {
		j = json{
			{"filePath", s.filePath},
			{"isLoop", s.isLoop},
			{"volume", s.volume}
		};
	}

	void from_json(const json& j, SceneBGMData& s) {
		if (j.contains("filePath")) j.at("filePath").get_to(s.filePath);
		if (j.contains("isLoop")) j.at("isLoop").get_to(s.isLoop);
		if (j.contains("volume")) j.at("volume").get_to(s.volume);
	}

	void to_json(json& j, const AudioData& a) {
		j = json{
			{"name", a.name},
			{"bgmMap", a.bgmMap}
		};
	}

	void from_json(const json& j, AudioData& a) {
		if (j.contains("name")) j.at("name").get_to(a.name);
		if (j.contains("bgmMap")) j.at("bgmMap").get_to(a.bgmMap);
	}

	void AudioEditor::Initialize() {
	}

	void AudioEditor::SaveAudio(const AudioData& audio) {
		std::string filename = audio.name + ".json";
		std::ofstream file(filename);
		if (file.is_open()) {
			json j = audio;
			file << j.dump(4);
		}
	}

	void AudioEditor::LoadAudio(AudioData& audio, const std::string& filename) {
		std::ifstream file(filename);
		if (file.is_open()) {
			try {
				json j;
				file >> j;
				audio = j.get<AudioData>();
			} catch (...) {
			}
		}
	}
}
