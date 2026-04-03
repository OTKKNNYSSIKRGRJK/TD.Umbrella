export module Game.Events;

import Lumina.Core.Math;

namespace Game::Event {
	export int CameraShakingTimer = 0;
	export void OnAttack() {
		CameraShakingTimer = 15;
	}
	export Lumina::Math::F32x3 RespawnPos = {};
}