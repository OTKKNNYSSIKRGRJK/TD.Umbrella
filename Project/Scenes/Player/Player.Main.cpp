module;

//#include"Ground.h"
//#include "CameraSystem.h"

module Game.Player : Main;

import Game.MathUtils;

import Lumina.Main;
import Lumina.MeshManager;
import Lumina.D3D12.Aux.View;
import Game.MathUtils;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
	using namespace PlayerStates;
}

void Player::Initialize() {

	InitializeStates();
	InitializeComponents();

	inputHandler_.SetPlayer(this);

	rightHandJoint_.SetType(AttachmentType::PlayerHand);
	rightHandJoint_.SetAcceptType(AttachmentType::UmbrellaHandle);
	rightHandJoint_.SetInfo({ 0.0f,-0.0f,0.0f }, { 0.0f,0.0f,0.0f });

	backJoint_.SetType(AttachmentType::PlayerBack);
	backJoint_.SetAcceptType(AttachmentType::UmbrellaHandle);
	backJoint_.SetInfo({ 0.0f,-0.0f,0.0f }, { 0.0f,0.0f,0.0f });
	backJoint_.SetRot({0.0f,0.0f,Lumina::Math::DegToRad(135.0f)});

	umbrella_ = std::make_unique<Umbrella::Main>();
	umbrella_->Initialize();

	umbrella_->handle_->GetBaseJoint()->AttachTo(GetRightHandJoint());

	motionController_ = std::make_unique<MotionController>();

	// =====================
	// 【 当たり判定の設定 】
	// =====================

	// 1. コライダーの生成
	collider_ = std::make_unique<ConvexCollider>();
	collider_->SetUserData(this);

	// 2. 属性の設定（自分はPlayer、当たる相手はEnemyやEnemyの攻撃）
	collider_->SetMyType(COL_Player);
	collider_->SetYourType(COL_Enemy | COL_Enemy_Attack | COL_Ground);

	// 3. ローカル頂点データの設定（例：プレイヤーを囲む四角形やひし形など）
	std::vector<Vector3> localVertices = {
		{-1.0f, -1.0f, 0.0f}, // 左下
		{ 1.0f, -1.0f, 0.0f}, // 右下
		{-1.0f,  1.0f, 0.0f}, // 左上
		{ 1.0f,  1.0f, 0.0f}  // 右上
	};
	collider_->SetVertices(localVertices);

	// 4. 当たった時の処理（コールバック関数の登録）
	// ラムダ式を使って、このPlayerのメンバ関数や変数にアクセスできるようにする
	collider_->onCollisionCallback = [this](Collider* other, const Vector3& pushOut) {
		if (other->GetMyType() == COL_Ground) {
			#if defined(_DEBUG)
			ImGui::Text("Player to Ground Collision!!");
			#endif

			// =========================
			// 【 めり込み解消処理 】
			// =========================
			Vector3 actualPush = -pushOut;
			Position_ += actualPush;
			//obj_->worldTransform_.set_.Translation(Position_);

			Vector3 normal = actualPush;
			float len2{ Vector3::Dot(actualPush, actualPush) };
			if (len2 > 0.0f) {
				normal /= Lumina::Math::SQRT(len2);
			}

			// 足元に地面があるかのチェック
			if (normal.Y > 0.8f) {
				// 「落下中」または「立ち止まっている」時だけ着地判定
				// ジャンプ上昇中（> 0.0f）は坂に触れても着地しないようにする
				if (this->externalVelocity_.Y <= 0.0f) {
					#if defined(_DEBUG)
					ImGui::Text("Player to Ground Collision!! -> OKOKOKO");
					#endif

					this->onGround_ = true;

					// 地面の上なのでリセット
					if (this->externalVelocity_.Y < 0.0f) {
						this->externalVelocity_.Y = 0.0f;
					}
				}
			}

			//Ground* ground = static_cast<Ground*>(other->GetUserData());
			//if (ground != nullptr) {
			//
			//}
		}
		else if (other->GetMyType() == COL_Enemy_Attack) {

			// 1. 相手のコライダーから「持ち主（Enemy）」のポインタをもらう
			// ※ void* で返ってくるので、Enemy型にキャスト（変換）する
			//Enemy* enemy = static_cast<Enemy*>(other->GetUserData());

			// 2. 万が一キャストに失敗していないかチェック
			//if (enemy != nullptr) {
			//	// 3. 敵本体から攻撃力を取得して、ダメージを受ける！
			//	int damage = enemy->GetAttackPower();
			//	this->TakeDamage(damage); // プレイヤーのHPを減らす処理など
			//}
		}
	};

	WorldMatrix_ = std::make_unique<Matrix4x4>();
}

void Player::Update(float deltaTime) {
	// 移動量の初期化
	moveAmount_ = { 0.0f,0.0f,0.0f };

	// 入力関係の処理
	inputHandler_.HandleInput();

	// コンポーネントの更新
	if (mana_) {
		mana_->Update(deltaTime);
	}
	// ステートの更新
	if (currentMovementState_) {
		currentMovementState_->Update(deltaTime);
	}
	if (currentActionState_) {
		currentActionState_->Update(deltaTime);
	}

	// =========================
	// 【 コヨーテタイムの処理 】
	// =========================
	if (this->onGround_) {
		jumpCoyoteTimer_ = 0.0f;
	}
	else {
		jumpCoyoteTimer_ += deltaTime;
	}

	// ここから移動関係の処理
	moveAmount_ = (myVelocity_ + externalVelocity_) * deltaTime;
	//Position_ += moveAmount_;
	//obj_->worldTransform_.set_.Translation(obj_->worldTransform_.get_.Translation() + moveAmount_);

	// rightHandJoint_.SetRot( 手の回転 );
	rightHandJoint_.Update(); // 右手Joint自身の行列を計算

	Vector3 backPos = Position_;
	backPos.Y += 1.5f;
	backPos.Z += 1.0f;
	backJoint_.SetPos(backPos);
	backJoint_.Update();
	
	// 傘
	umbrella_->Update(deltaTime);

	// Colliderに設定
	collider_->SetWorldPosition(GetPosition());

	*WorldMatrix_ = Game::MathUtils::SRT(Scale_, EulerAngle_, Position_);
	collider_->SetWorldMatrix(*WorldMatrix_);

	collider_->UpdateAABB();

	#if defined(_DEBUG)
	Vector3 test = rightHandJoint_.GetPos();
	ImGui::DragFloat3("RHandJoint", &test.X);

	Vector3 colliderPos = collider_->GetWorldPosition();
	ImGui::DragFloat3("colliderPos", &colliderPos.X);

	// よくわからないのでとりまコメントアウトしちゃう
	//ImGuiManager::GetInstance()->DrawDrag("Player : External Speed", this->externalVelocity_);
	//ImGuiManager::GetInstance()->DrawDrag("Player : My Speed", this->myVelocity_);

	if (ImGui::TreeNodeEx("Mana")) {
		ImGui::Text("Use : Push LSHIFT");
		ImGui::Text("Mana is Use ? : ");
		ImGui::SameLine();
		ImGui::Text(inputData_.useMana ? "Using" : "Not Use");

		float mana = mana_->GetCurrentMana();
		ImGui::DragFloat("Amount", &mana);
		ImGui::TreePop();
	}
	#endif

	// 地面についているフラグを解除
	// ※ バグの原因になりそうな箇所
	this->onGround_ = false;
}

// メッシュバッチ自体はMeshManager::BatchBegin()とBatchEnd()の間に入れないといけないので
// Draw()の中からメッシュをバッチするのであればシーンのほうのPlayer::Draw()も
// BatchBegin()とBatchEnd()の間で呼び出さなくてはならない
void Player::Draw() {

	// メッシュバッチ・描画マネージャ
	auto& meshMngr{ Lumina::Context::Instance().MeshContext() };

	// 描画してほしいメッシュをバッチ
	// --- パラメータ ---
	// Lumina::MeshShaderAsset const* mesh_ : メッシュ（シーンのほうで読み込み）
	// uint32_t num_Instances_ : インスタンス数（今のパイプラインではインスタンシングやってないから1固定で）
	// D3D12_CPU_DESCRIPTOR_HANDLE localCBV_Material_ : メッシュマテリアルバッファのCBV
	// Matrix4x4 const& world_ : ワールド行列
	meshMngr.Batch(*Mesh_, 1U, MeshMaterialCBV_, *WorldMatrix_);

	umbrella_->Draw();
}

void Player::ChangeMovementState(PlayerStates::Base* newState) {
	if (currentMovementState_) {
		currentMovementState_->Exit();
	}

	currentMovementState_ = newState;

	if (currentMovementState_) {
		currentMovementState_->Enter();
	}
}

void Player::ChangeActionState(PlayerStates::Base* newState) {
	if (currentActionState_) {
		currentActionState_->Exit();
	}

	currentActionState_ = newState;

	if (currentActionState_) {
		currentActionState_->Enter();
	}
}

void Player::InitializeStates() {

	//// MovementStateの初期化 ////

	// 親のStateの生成
	groundedState_ = std::make_unique<Movement::Grounded>();groundedState_->SetInfo(this);
	airborneState_ = std::make_unique<Movement::Airborne>();airborneState_->SetInfo(this);

	// 子のStateの生成
	idleState_ = std::make_unique<Movement::Idle>();idleState_->SetInfo(this, groundedState_.get());
	walkingState_ = std::make_unique<Movement::Walking>();walkingState_->SetInfo(this, groundedState_.get());
	runningState_ = std::make_unique<Movement::Running>();runningState_->SetInfo(this, groundedState_.get());
	restrictedState_ = std::make_unique<Movement::Restricted>();restrictedState_->SetInfo(this, groundedState_.get());

	// 最初の設定
	currentMovementState_ = idleState_.get();

	//// ActionStateの初期化 ////

	sheatheWeaponState_ = std::make_unique<Action::SheatheWeapon>();sheatheWeaponState_->SetInfo(this);
	drawWeaponState_ = std::make_unique<Action::DrawWeapon>();drawWeaponState_->SetInfo(this);
	normalState_ = std::make_unique<Action::Normal>();normalState_->SetInfo(this);
	attackState_ = std::make_unique<Action::Attack>();attackState_->SetInfo(this);
	guardState_ = std::make_unique<Action::Guard>();guardState_->SetInfo(this);

	umbrellaOpenState_ = std::make_unique<Action::UmbrellaOpen>();umbrellaOpenState_->SetInfo(this);
	umbrellaCloseState_ = std::make_unique<Action::UmbrellaClose>();umbrellaCloseState_->SetInfo(this);
	umbrellaReverseState_ = std::make_unique<Action::UmbrellaReverse>();umbrellaReverseState_->SetInfo(this);

	// 最初の設定
	currentActionState_ = normalState_.get();
}

void Player::InitializeComponents() {
	//// ManaComponentの初期化 ////
	mana_ = std::make_unique<ManaComponent>(100.0f);

}
///////////////////
///
///   移動関係
///
///////////////////
void Player::AddForce(const Vector3& force) {
	externalVelocity_ += force;
}

void Player::Jump() {
	if (inputData_.isJump) {
		if (this->onGround_ || this->jumpCoyoteTimer_ < JUMP_COYOTE_MAX_TIME) {
			// Y軸に上向きの初速（ジャンプ力）を与える！
			float jumpPower = 7.0f; // 調整
			externalVelocity_.Y = jumpPower;// 初速
			// フラグの処理
			this->onGround_ = false;
			this->jumpCoyoteTimer_ = this->JUMP_COYOTE_MAX_TIME;

			// ステートを「空中」に切り替える！
			ChangeMovementState(airborneState_.get());
		}
	}
}

void Player::UmbrellaAttachBack() {
	umbrella_->handle_->GetBaseJoint()->AttachTo(GetBackJoint());
}

void Player::UmbrellaAttachRHand() {
	umbrella_->handle_->GetBaseJoint()->AttachTo(GetRightHandJoint());
}