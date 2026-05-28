export module Game.ExpOrbManager;

import <vector>;
import <memory>;
import Lumina;
import Lumina.Primitive;
import Game.Player;

export namespace Game {
	struct ExpOrb {
		Lumina::Math::F32x3 position{ 0.0f, 0.0f, 0.0f };
		Lumina::Math::F32x3 velocity{ 0.0f, 0.0f, 0.0f };
		uint32_t xpAmount = 0;
		float timer = 0.0f;
		bool isDead = false;
	};

	class ExpOrbManager {
	public:
		static ExpOrbManager* GetInstance();

		void Spawn(const Lumina::Math::F32x3& position, uint32_t amount);
		void Update(float deltaTime, const Lumina::Math::F32x3& playerPosition, Player* player);
		void Draw(Lumina::PrimitiveManager* primitiveManager, const Lumina::Math::F32x4x4<>& viewProj);
		void Clear();

		const std::vector<ExpOrb>& GetOrbs() const { return orbs_; }

	private:
		ExpOrbManager() = default;
		static std::unique_ptr<ExpOrbManager> instance_;
		std::vector<ExpOrb> orbs_;
	};
}
