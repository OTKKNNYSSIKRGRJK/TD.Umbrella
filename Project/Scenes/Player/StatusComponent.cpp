module StatusComponent;

void StatusComponent::TakeDamage(float incomingDamage) {
    if (invincibilityTimer_ > 0.0f) return;

    // 例: 攻撃力から防御力を引く
    float actualDamage = (std::max)(0.0f, incomingDamage - defense_);

    currentHp_ -= actualDamage;
    invincibilityTimer_ = 1.0f; // 1秒間の無敵を付与

    // HPが0を下回らないようにする
    if (currentHp_ < 0.0f) {
        currentHp_ = 0.0f;
    }
}

// 回復する処理
void StatusComponent::Heal(float amount) {
    currentHp_ += amount;
    if (currentHp_ > maxHp_) {
        currentHp_ = maxHp_;
    }
}

void StatusComponent::ApplyLevelBonus(uint32_t level) {
    // 例: レレベル1ごとに最大HPが20、攻撃力が5、防御力が0.5ずつ上がるとする
    float baseHp = 100.0f;
    float baseAtk = 10.0f;
    float baseDef = 1.0f;

    maxHp_ = baseHp + (level - 1) * 20.0f;
    attack_ = baseAtk + (level - 1) * 5.0f;
    defense_ = baseDef + (level - 1) * 0.25f;

    // レベルアップしたお祝いに、HPを全回復してあげる
    currentHp_ = maxHp_;
}