export module Game.Events;

namespace Game::Event {
	export int CameraShakingTimer = 0;
	export void OnAttack() {
		CameraShakingTimer = 30;
	}
}