export module StatusComponent;

import <algorithm>;

export class StatusComponent {
public:
    // コンストラクタ（初期ステータスを設定）
    StatusComponent(float maxHp, float attack, float defense)
        : maxHp_(maxHp), currentHp_(maxHp), attack_(attack), defense_(defense) {
    }

    ~StatusComponent() = default;
public:

    ///////////////////////////////////
    //   アクション（ダメージ・回復）
    ///////////////////////////////////

    // ダメージを受ける処理
    void TakeDamage(float incomingDamage);

    void Update(float deltaTime) {
        if (invincibilityTimer_ > 0.0f) {
            invincibilityTimer_ -= deltaTime;
        }
    }

    bool IsInvincible() const { return invincibilityTimer_ > 0.0f; }
    void SetInvincible(float time) { invincibilityTimer_ = time; }

    // 回復する処理
    void Heal(float amount);

    // 生死判定
    bool IsDead() const { return currentHp_ <= 0.0f; }

    ///////////////////////////////////
    //   Get・Set関係
    ///////////////////////////////////

    float GetHp() const { return currentHp_; }
    float GetMaxHp() const { return maxHp_; }
    float GetAttack() const { return attack_; }
    float GetDefense() const { return defense_; }

    // ※マナ消費で攻撃力を上げたりする時に使う
    void SetAttack(float attack) { attack_ = attack; }
    void SetDefense(float defense) { defense_ = defense; }

    // ヒットストップ
    void SetHitStop(float hitStop) { hitStop_ = hitStop; }
    float GetHitStop() const { return hitStop_; }

    // 攻撃のインスタンスID（同一アクションでの重複ヒット防止用）
    void IncrementAttackInstanceId() { attackInstanceId_++; }
    uint32_t GetAttackInstanceId() const { return attackInstanceId_; }

    void ApplyLevelBonus(uint32_t level);
private:
    float maxHp_;
    float currentHp_;
    float attack_;
    float defense_;
    float hitStop_ = 0.0f;
    uint32_t attackInstanceId_ = 0;
    float invincibilityTimer_ = 0.0f;
};