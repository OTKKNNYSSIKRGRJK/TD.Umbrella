module ExperienceComponent;

import <cmath>;

void ExperienceComponent::Initialize(uint32_t initialLevel) {
    currentLevel_ = initialLevel;
    currentXp_ = 0;
}

uint32_t ExperienceComponent::GetNextLevelXp() const noexcept {
    // 公式: a * (Level ^ b) を計算
    // 例: Lv1の時は 100 * (1^2) = 100. Lv2の時は 100 * (2^2) = 400
    return static_cast<uint32_t>(baseMultiplier_ * std::pow(static_cast<float>(currentLevel_), exponent_));
}

bool ExperienceComponent::AddExperience(uint32_t amount) {
    currentXp_ += amount;
    bool leveledUp = false;

    // 獲得したXPが、次のレベルへの必要量を超えている限りループ（一気に複数Lv上がる対応）
    while (currentXp_ >= GetNextLevelXp()) {
        currentXp_ -= GetNextLevelXp(); // 必要分を消費
        currentLevel_++;
        leveledUp = true;
    }

    return leveledUp;
}