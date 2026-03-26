module ManaComponent;

import <algorithm>;

import Lumina.Core.Math;
import Game.MathUtils;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
}

void ManaComponent::Update(float deltaTime) {
	float manaDuration = 15.0f;
	currentMana_ += deltaTime * manaDuration;
	
	// 上限を設定
	currentMana_ = currentMana_ > maxMana_ ? maxMana_ : currentMana_;
}