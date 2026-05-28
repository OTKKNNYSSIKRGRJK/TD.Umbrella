module;

#include <cmath>
#include <cstdlib>
#include <algorithm>

module Game.ExpOrbManager;

import Lumina.Primitive;

namespace Game {
	std::unique_ptr<ExpOrbManager> ExpOrbManager::instance_ = nullptr;

	ExpOrbManager* ExpOrbManager::GetInstance() {
		if (instance_ == nullptr) {
			instance_ = std::unique_ptr<ExpOrbManager>(new ExpOrbManager());
		}
		return instance_.get();
	}

	void ExpOrbManager::Spawn(const Lumina::Math::F32x3& position, uint32_t amount) {
		// 1つの死亡でいくつかのオーブに分割してスポーンする
		uint32_t numOrbs = 3 + (rand() % 3); // 3〜5個
		uint32_t xpPerOrb = amount / numOrbs;
		uint32_t remainder = amount % numOrbs;

		for (uint32_t i = 0; i < numOrbs; ++i) {
			ExpOrb orb;
			orb.position = position;
			
			// 飛び散る初期速度
			float angle = ((float)(rand() % 360)) * 3.14159f / 180.0f;
			float speed = 2.0f + ((float)(rand() % 100) / 100.0f) * 4.0f; // 2.0 〜 6.0
			
			orb.velocity.X = std::cos(angle) * speed;
			orb.velocity.Y = 4.0f + ((float)(rand() % 100) / 100.0f) * 4.0f; // 上向きに跳ねる
			orb.velocity.Z = 0.0f;
			
			orb.xpAmount = xpPerOrb + (i == 0 ? remainder : 0);
			orb.timer = 0.0f;
			orb.isDead = false;

			orbs_.push_back(orb);
		}
	}

	void ExpOrbManager::Update(float deltaTime, const Lumina::Math::F32x3& playerPosition, Player* player) {
		for (auto& orb : orbs_) {
			if (orb.isDead) continue;

			orb.timer += deltaTime;

			if (orb.timer < 0.4f) {
				// 初期バーストフェーズ（放物線移動）
				orb.velocity.Y -= 9.8f * deltaTime;
				orb.velocity.X *= 0.95f; // 水平減衰
				
				orb.position.X += orb.velocity.X * deltaTime;
				orb.position.Y += orb.velocity.Y * deltaTime;
			}
			else {
				// 引き寄せフェーズ
				float dx = playerPosition.X - orb.position.X;
				float dy = (playerPosition.Y + 1.0f) - orb.position.Y; // プレイヤーの胸元あたりを目指す
				float dist = std::sqrt(dx * dx + dy * dy);

				if (dist < 0.8f) {
					// プレイヤーが獲得
					if (player) {
						player->GainXp(orb.xpAmount);
					}
					orb.isDead = true;
				}
				else {
					// プレイヤーに向かって加速
					float speed = 12.0f + (orb.timer - 0.4f) * 15.0f; // 時間経過でどんどん加速
					speed = std::min(speed, 35.0f); // 最大速度制限

					orb.velocity.X = (dx / dist) * speed;
					orb.velocity.Y = (dy / dist) * speed;

					orb.position.X += orb.velocity.X * deltaTime;
					orb.position.Y += orb.velocity.Y * deltaTime;
				}
			}
		}

		// 死亡したオーブを除去
		orbs_.erase(
			std::remove_if(orbs_.begin(), orbs_.end(), [](const ExpOrb& orb) { return orb.isDead; }),
			orbs_.end()
		);
	}

	void ExpOrbManager::Draw(Lumina::PrimitiveManager* primitiveManager, const Lumina::Math::F32x4x4<>& viewProj) {
		if (!primitiveManager || orbs_.empty()) return;

		// 経験値オーブの色（明るい黄色）
		Lumina::F32x4 color{ 1.0f, 0.95f, 0.2f, 1.0f };
		float hw = 0.015f; // ひし形の幅
		float hh = 0.020f; // ひし形の高さ

		for (const auto& orb : orbs_) {
			if (orb.isDead) continue;

			// 3D座標 -> Clip空間への変換
			Lumina::Math::F32x4 pos(orb.position.X, orb.position.Y, orb.position.Z, 1.0f);
			Lumina::Math::F32x4 clipPos(
				pos.X() * viewProj[0].Get(0) + pos.Y() * viewProj[1].Get(0) + pos.Z() * viewProj[2].Get(0) + pos.W() * viewProj[3].Get(0),
				pos.X() * viewProj[0].Get(1) + pos.Y() * viewProj[1].Get(1) + pos.Z() * viewProj[2].Get(1) + pos.W() * viewProj[3].Get(1),
				pos.X() * viewProj[0].Get(2) + pos.Y() * viewProj[1].Get(2) + pos.Z() * viewProj[2].Get(2) + pos.W() * viewProj[3].Get(2),
				pos.X() * viewProj[0].Get(3) + pos.Y() * viewProj[1].Get(3) + pos.Z() * viewProj[2].Get(3) + pos.W() * viewProj[3].Get(3)
			);

			if (clipPos.W() > 0.1f) {
				float ndcX = clipPos.X() / clipPos.W();
				float ndcY = clipPos.Y() / clipPos.W();

				// スケールを距離に応じて少し変える
				float scale = 1.0f / clipPos.W() * 8.0f;
				scale = std::clamp(scale, 0.4f, 1.5f);

				float cur_hw = hw * scale;
				float cur_hh = hh * scale;

				// 明示的に PrimitiveVertex を作成 (F32x4 への暗黙の型変換エラーを防ぐため initializer_list を直接渡す)
				Lumina::PrimitiveVertex vTop{ { ndcX, ndcY + cur_hh, 0.0f, 1.0f }, color, { 0.0f, 0.0f }, 0U };
				Lumina::PrimitiveVertex vBottom{ { ndcX, ndcY - cur_hh, 0.0f, 1.0f }, color, { 0.0f, 0.0f }, 0U };
				Lumina::PrimitiveVertex vLeft{ { ndcX - cur_hw, ndcY, 0.0f, 1.0f }, color, { 0.0f, 0.0f }, 0U };
				Lumina::PrimitiveVertex vRight{ { ndcX + cur_hw, ndcY, 0.0f, 1.0f }, color, { 0.0f, 0.0f }, 0U };

				// 三角形1（上半分）
				primitiveManager->BatchTriangle(vLeft, vTop, vRight);

				// 三角形2（下半分）
				primitiveManager->BatchTriangle(vLeft, vRight, vBottom);
			}
		}
	}

	void ExpOrbManager::Clear() {
		orbs_.clear();
	}
}
