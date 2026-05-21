#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

int main() {
    std::string gltfPath = "Assets\\Enemy\\Slime\\slime.gltf";
    std::ifstream ifs(gltfPath);
    if (!ifs.is_open()) {
        std::cout << "Failed to open file" << std::endl;
        return 1;
    }
    try {
        json gltfJson;
        ifs >> gltfJson;
        if (gltfJson.contains("animations") && gltfJson["animations"].is_array()) {
            for (size_t i = 0; i < gltfJson["animations"].size(); ++i) {
                const auto& anim = gltfJson["animations"][i];
                if (anim.contains("name") && anim["name"].is_string()) {
                    std::cout << "Found animation: " << anim["name"].get<std::string>() << std::endl;
                }
            }
        } else {
            std::cout << "No animations array" << std::endl;
        }
    } catch (std::exception& e) {
        std::cout << "Parse error: " << e.what() << std::endl;
    }
    return 0;
}
