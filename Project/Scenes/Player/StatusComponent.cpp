module StatusComponent;

void StatusComponent::TakeDamage(float incomingDamage) {
    // 例: 攻撃力から防御力を引く
    float actualDamage = (std::max)(0.1f, incomingDamage - defense_);

    currentHp_ -= actualDamage;

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