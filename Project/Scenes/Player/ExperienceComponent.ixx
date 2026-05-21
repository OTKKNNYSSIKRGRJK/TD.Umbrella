export module ExperienceComponent;

import <cstdint>;

export class ExperienceComponent {
public:
    ExperienceComponent() = default;
    ~ExperienceComponent() = default;

    // 初期化（初期レベルの設定など）
    void Initialize(uint32_t initialLevel = 1);

    // 経験値の獲得（敵を倒したときなどに呼ぶ）
    // レベルアップしたら true を返す
    bool AddExperience(uint32_t amount);

    // ゲッター
    uint32_t GetLevel() const noexcept { return currentLevel_; }
    uint32_t GetCurrentXp() const noexcept { return currentXp_; }
    uint32_t GetNextLevelXp() const noexcept; // 次のレベルに必要なXPを数列から計算

private:
    uint32_t currentLevel_ = 1;
    uint32_t currentXp_ = 0;

    // 数列のパラメーター
    const float baseMultiplier_ = 100.0f; // 公式の「a」
    const float exponent_ = 2.0f;         // 公式の「b」 (2乗)
};