export module Game.Player : Main;

import <memory>;

import : Common;
import : InputHandler;
import Game.Umbrella;
import Game.MotionManager;
import Game.Attachment;
import Collider;

import : States;

import ManaComponent;
import StatusComponent;

import Lumina.Core.Math;
import Lumina.MeshManager;
import Lumina.D3D12;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
}

export enum class WeaponStance {
	Sheathed,// 納刀
	Drawn,   // 抜刀
};

// ボタンの入力状態を表す列挙型
export enum class ButtonState {
	None,     // 0,0 : 押されていない
	Pressed,  // 0,1 : 押した瞬間 (Trigger)
	Held,     // 1,1 : 押しっぱなし (Hold)
	Released  // 1,0 : 離した瞬間 (Release)
};

inline bool IsButtonDown(ButtonState state) {
	return state == ButtonState::Pressed || state == ButtonState::Held;
}

inline bool IsButtonUp(ButtonState state) {
	return state == ButtonState::None || state == ButtonState::Released;
}

ButtonState UpdateButtonState(bool isPress, ButtonState previousState) {
	if (isPress) {
		// 今押されていて、前回押されていなかったら「押した瞬間」
		if (previousState == ButtonState::None || previousState == ButtonState::Released) {
			return ButtonState::Pressed;
		}
		// それ以外（前回も押されていた）なら「押しっぱなし」
		return ButtonState::Held;
	}
	else {
		// 今押されてなくて、前回押されていたら「離した瞬間」
		if (previousState == ButtonState::Pressed || previousState == ButtonState::Held) {
			return ButtonState::Released;
		}
		// それ以外（前回も押されてない）なら「何もなし」
		return ButtonState::None;
	}
}

export struct PlayerInputData {
	// ---------------------------------
	// アナログ入力（スティック類）
	// ---------------------------------
	Vector3 moveDirection = {0.0f,0.0f,0.0f};  // 左スティックの入力方向
	float aimingDirectionX = 0.0f; // 右スティックの左右入力
	float aimingDirectionY = 0.0f; // 右スティックの上下入力

	// ---------------------------------
	// デジタル入力（ボタン類）
	// ---------------------------------
	ButtonState attack = ButtonState::None;     // 攻撃ボタン (Pressed, Held, Releasedなど全てこれで判別)
	ButtonState jump = ButtonState::None;       // ジャンプボタン
	ButtonState evasion = ButtonState::None;    // 回避ボタン
	ButtonState sheathe = ButtonState::None;    // 納刀ボタン
	ButtonState guard = ButtonState::None;      // ガードボタン
	ButtonState reverse = ButtonState::None;    // リバースボタン
	ButtonState aim = ButtonState::None;        // 照準ボタン
	ButtonState shoot = ButtonState::None;      // 射撃ボタン
	ButtonState repair = ButtonState::None;     // 修理ボタン

	// ---------------------------------
	// 特殊なフラグ（トグルなど）
	// ---------------------------------
	bool useMana = false;
	bool debugRevive = false;
};

export class Player {
public:
	Player() {
		Scale_ = { 1.0f, 1.0f, 1.0f };
		EulerAngle_ = { 0.0f, 0.0f, 0.0f };
		Position_ = { 0.0f, 0.0f, 0.0f };

		InitializeStates();
	}
	~Player() {}
public:
	void Initialize();
	void Update(float deltaTime);
	void Draw();

	//////////////////////////
	/// 
	///   State関係
	/// 
	//////////////////////////
public:
	void ChangeMovementState(PlayerStates::Base* newState);
	void ChangeActionState(PlayerStates::Base* newState);
	bool onGround_ = false;
	PlayerStates::Base* GetCurrentMovementState() const { return currentMovementState_; }
	PlayerStates::Base* GetCurrentActionState() const { return currentActionState_; }
private:
	WeaponStance currentStance_ = WeaponStance::Drawn;

	// 使用しているステート
	PlayerStates::Base* currentMovementState_;
	PlayerStates::Base* currentActionState_;
	// 次に実行したいステート
	PlayerStates::Base* reservedActionState_ = nullptr;
public:
	// Movement State
	std::unique_ptr<PlayerStates::Movement::Grounded>groundedState_;
	std::unique_ptr<PlayerStates::Movement::Idle>idleState_;
	std::unique_ptr<PlayerStates::Movement::Walking>walkingState_;
	std::unique_ptr<PlayerStates::Movement::Running>runningState_;
	std::unique_ptr<PlayerStates::Movement::Airborne>airborneState_;
	std::unique_ptr<PlayerStates::Movement::Restricted>restrictedState_;// アクションの際に動きを制限するState

	// Action State
	std::unique_ptr<PlayerStates::Action::Dead>deadState_;
	std::unique_ptr<PlayerStates::Action::SheatheWeapon>sheatheWeaponState_;
	std::unique_ptr<PlayerStates::Action::DrawWeapon>drawWeaponState_;
	std::unique_ptr<PlayerStates::Action::NormalSheathed>normalSheathedState_;
	std::unique_ptr<PlayerStates::Action::NormalDrawn>normalDrawnState_;
	std::unique_ptr<PlayerStates::Action::Attack>attackState_;
	std::unique_ptr<PlayerStates::Action::Guard>guardState_;
	std::unique_ptr<PlayerStates::Action::ReverseCharge>reverseChargeState_;
	std::unique_ptr<PlayerStates::Action::ReverseAttack>reverseAttackState_;
	std::unique_ptr<PlayerStates::Action::ThrowUmbrella>throwUmbrellaState_;

	std::unique_ptr<PlayerStates::Action::UmbrellaOpen>umbrellaOpenState_;
	std::unique_ptr<PlayerStates::Action::UmbrellaClose>umbrellaCloseState_;
	std::unique_ptr<PlayerStates::Action::UmbrellaReverse>umbrellaReverseState_;
	std::unique_ptr<PlayerStates::Action::RepairUmbrella>repairUmbrellaState_;

public:// Get・Set
	WeaponStance GetWeaponStance() const noexcept { return currentStance_; }
	void SetWeaponStance(WeaponStance nextStance) { currentStance_ = nextStance; }
	// アクションを予約する
	void ReserveActionState(PlayerStates::Base* state) { reservedActionState_ = state; }
	// 予約を消費する
	PlayerStates::Base* ConsumeReservedAction() {
		PlayerStates::Base* state = reservedActionState_;
		reservedActionState_ = nullptr;
		return state;
	}
private:// 見る必要のない関数
	void InitializeStates();

	//////////////////////////////
	///
	///   移動関係の変数
	/// 
	//////////////////////////////
public:
	// 外部からの力を加える関数
	void AddForce(const Vector3& force);
	// ジャンプ
	void Jump();
	// ワープ
	void WarpToUmbrella();
public:
	// 移動制御用の変数
	Vector3 moveDirection_;// プレイヤーの移動したい方向
	Vector3 eyesDirection_;// プレイヤーの視線の方向
	// ※たぶん通常は同じ方向だが、ロックオンしているときは分かれる

	Vector3 myVelocity_;// 自発的な速度
	Vector3 externalVelocity_;// 外部からの速度(ノックバック、重力 etc)

	Vector3 moveAmount_;// 最終的な1フレームの移動量
	// ※加速度や速さはそれぞれのStateで各々作る

	// 【 ジャンプ 】
	float jumpCoyoteTimer_ = 0.0f;
	const float JUMP_COYOTE_MAX_TIME = 0.15f;

	//////////////////////////////
	///
	///   入力関係
	/// 
	//////////////////////////////
private:
	InputHandler inputHandler_;
	PlayerInputData inputData_;
	void ThrowUpdate(float deltaTime);
public:
	// Get・Set関係
	void SetInputData(const PlayerInputData& input) { inputData_ = input; }
	const PlayerInputData& GetInput()const { return inputData_; }
	// ====================
	// 照準・発射
	// ====================
public:
	void SetTargetPos(const Vector3& pos) { targetPos_ = pos; }
	Vector3 GetTargetPos()const { return targetPos_; }
private:
	Vector3 targetPos_;// ターゲットの位置

	//////////////////////////////
	///
	///   アタッチメント
	/// 
	//////////////////////////////
public:
	Attachment* GetRightHandJoint() { return &rightHandJoint_; }
	Attachment* GetBackJoint() { return &backJoint_; }

	void UmbrellaAttachRHand();
	void UmbrellaAttachBack();
private:
	Attachment rightHandJoint_;
	Attachment backJoint_;

	//////////////////////////////
	///
	///   傘
	/// 
	//////////////////////////////
public:
	Umbrella::Main& GetUmbrella() { return *umbrella_; }
private:
	std::unique_ptr<Umbrella::Main>umbrella_;

	//////////////////////////////
	///
	///   Component
	/// 
	//////////////////////////////
public:
	// Get関係
	ManaComponent& GetManaComponent() { return *mana_; }
	StatusComponent& GetStatusComponent() { return *status_; }
private:
	// ManaComponent
	std::unique_ptr<ManaComponent>mana_;
	// StatusComponent
	std::unique_ptr<StatusComponent>status_;

	// 無敵の時間
	float invincibilityTimer_ = 0.0f;

	//////////////////////////////
	///
	///   当たり判定
	/// 
	//////////////////////////////
public:
	Collider* GetCollider()const { return collider_.get(); }
	ConvexCollider* GetSmashCollider()const { return smashCollider_.get(); }
private:
	std::unique_ptr<ConvexCollider>collider_;
	std::unique_ptr<ConvexCollider> smashCollider_;
	//////////////////////////////
	///
	///   その他
	/// 
	//////////////////////////////
public:
	void SetMesh(Lumina::MeshShaderAsset const& mesh_) noexcept { Mesh_ = &mesh_; }
	void SetMeshMaterialCBV(D3D12_CPU_DESCRIPTOR_HANDLE cbv_) noexcept { MeshMaterialCBV_ = cbv_; }
private:
	//Fngine* p_fngine;

	//std::unique_ptr<ModelObject> obj_;
	Lumina::MeshShaderAsset const* Mesh_;
	D3D12_CPU_DESCRIPTOR_HANDLE MeshMaterialCBV_;

	Vector3 Scale_;
	Vector3 EulerAngle_;
	Vector3 Position_;

	std::unique_ptr<Matrix4x4> WorldMatrix_;

	// プレイヤーの行動を管理するクラス
public:	std::unique_ptr<MotionController> motionController_;

	  /*OnGroundの実装必要だわ。浮いちゃう*/

public:
	Vector3 const& GetPosition() const noexcept { return Position_; }
	void SetPosition(Vector3 const& pos_) { Position_ = pos_; }
	auto WorldMatrix() const noexcept -> Matrix4x4 const& { return *WorldMatrix_; }

private:
	void InitializeComponents();
};
