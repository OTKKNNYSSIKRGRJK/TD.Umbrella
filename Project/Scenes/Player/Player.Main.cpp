module Game.Player : Main;

import Game.MathUtils;

import Lumina.Main;
import Lumina.MeshManager;
import Lumina.D3D12.Aux.View;
import nlohmann.json;
import Game.MathUtils;

import Lumina.CG3D;
import Lumina.CG3D.Animation;
import Game.Events.InGame;

import <fstream>;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;
	using namespace PlayerStates;
	using json = nlohmann::json;
}

AttackData::Database LoadAttackDatabase(const std::string& filepath) {
	AttackData::Database database;

	std::ifstream file(filepath);
	if (!file.is_open()) {
		// エラーハンドリング
		return database;
	}

	json j;
	file >> j;

	// "attacks" の中身をループして取り出す
	if (j.contains("attacks")) {
		for (auto& item : j["attacks"].items()) {
			std::string key = item.key();
			auto& val = item.value();

			AttackData::AttackData attack;

			attack.name = val.value("name", "");
			attack.motion = val.value("motion", "");
			attack.animationName = val.value("animation", "");
			attack.duration = val.value("duration", 0.0f);
			attack.damage = val.value("damage", 0.0f);
			attack.manaCost = val.value("manaCost", 0.0f);

			if (val.contains("transformEvent")) {
				attack.transformEvent.time = val["transformEvent"].value("time", 0.0f);
				attack.transformEvent.targetStance = val["transformEvent"].value("targetUmbrellaForm", "");
			}

			// 物理データ（省略されたらデフォルト値）
			if (val.contains("physics")) {
				attack.physics.velocityX = val["physics"].value("velocityX", 0.0f);
				attack.physics.velocityY = val["physics"].value("velocityY", 0.0f);
				attack.physics.gravityScale = val["physics"].value("gravityScale", 1.0f);
			}

			// 演出データ
			if (val.contains("feel")) {
				attack.feel.hitStop = val["feel"].value("hitStop", 0.0f);
				attack.feel.cameraShake = val["feel"].value("cameraShake", 0.0f);
			}

			// 派生ルート配列
			if (val.contains("branches")) {
				for (auto& branchJson : val["branches"]) {
					AttackData::AttackBranch branch;
					branch.type = branchJson.value("type", "Input");
					branch.input = branchJson.value("input", "");
					branch.timeMin = branchJson.value("timeMin", 0.0f);
					branch.timeMax = branchJson.value("timeMax", 999.0f);
					branch.nextAttack = branchJson.value("nextAttack", "");
					branch.consumeMana = branchJson.value("consumeMana", 0.0f);

					if (branchJson.contains("condition")) {
						branch.condition.minMana = branchJson["condition"].value("minMana", 0.0f);
					}

					attack.branches.push_back(branch);
				}
			}

			// データベースに登録（キーは "NormalCombo3" など）
			database[key] = attack;
		}
	}

	return database;
}

void Player::LoadAnimation() {
	[[maybe_unused]] auto& context{ Lumina::Context::Instance() };
	[[maybe_unused]] auto const& d3d12Context{ context.D3D12Context() };
	[[maybe_unused]] auto const& d3d12Device{ d3d12Context.Device() };

	PlayerSkinnedModel_ = std::make_unique<SkinnedModel>();
	PlayerSkinnedModel_->Collection_ = Lumina::CG3D::Import("Neki.gltf", "Assets/Neki");

	PlayerSkinnedModel_->VertexBuffer_.Initialize(
		d3d12Device,
		// バッファサイズ＝頂点サイズ×メッシュの頂点数
		sizeof(Lumina::CG3D::Mesh::Vertex) *
		PlayerSkinnedModel_->Collection_.Meshes[0].Vertices.size()
	);
	// 頂点バッファに頂点データを入れる
	PlayerSkinnedModel_->VertexBuffer_.Store(
		// データ
		PlayerSkinnedModel_->Collection_.Meshes[0].Vertices.data(),
		// データサイズ
		sizeof(Lumina::CG3D::Mesh::Vertex) *
		PlayerSkinnedModel_->Collection_.Meshes[0].Vertices.size(),
		// メモリオフセット　気にせんでええ
		0LLU
	);
	// 頂点バッファを使ってビューを作成
	// テンプレートに頂点の変数型を入れる
	PlayerSkinnedModel_->VBV_ =
		Lumina::D3D12::VBV::Create<Lumina::CG3D::Mesh::Vertex>(PlayerSkinnedModel_->VertexBuffer_);

	PlayerSkinnedModel_->IndexBuffer_.Initialize(
		d3d12Device,
		sizeof(Lumina::U32) *
		PlayerSkinnedModel_->Collection_.Meshes[0].Indices.size()
	);
	PlayerSkinnedModel_->IndexBuffer_.Store(
		PlayerSkinnedModel_->Collection_.Meshes[0].Indices.data(),
		sizeof(Lumina::U32) *
		PlayerSkinnedModel_->Collection_.Meshes[0].Indices.size(),
		0LLU
	);
	PlayerSkinnedModel_->IBV_ = Lumina::D3D12::IBV::Create(PlayerSkinnedModel_->IndexBuffer_);

	PlayerSkinnedInstance_ = std::make_unique<SkinnedInstance>();

	PlayerSkinnedInstance_->Skeleton_ =
		Lumina::CG3D::CreateSkeleton(PlayerSkinnedModel_->Collection_.Root);
	Lumina::CG3D::CreateSkinCluster(
		PlayerSkinnedInstance_->SkinCluster_,
		d3d12Device,
		d3d12Context.GlobalDescriptorHeap(),
		PlayerSkinnedInstance_->Skeleton_,
		// メッシュ
		PlayerSkinnedModel_->Collection_.Meshes[0]
	);

	PlayerSkinnedInstance_->MeshScale_ = { 1.0f, 1.0f, 1.0f };
	PlayerSkinnedInstance_->MeshRotate_ = { 0.0f, 0.0f, 0.0f };
	PlayerSkinnedInstance_->MeshTranslate_ = { 0.0f, 0.0f, 0.0f };

	auto animation_idle{ Lumina::CG3D::LoadAnimationFile("Idle_Drawn_H.gltf", "Assets/Neki") };
	auto animation_idleOpen{ Lumina::CG3D::LoadAnimationFile("Idle_Drawn_Open_H.gltf", "Assets/Neki") };
	auto animation_idleHoldingUmbrella{ Lumina::CG3D::LoadAnimationFile("IdleHoldingUmbrella.gltf", "Assets/Neki") };

	animDatabase_["Idle"] = animation_idle[1];
	animDatabase_["IdleOpen"] = animation_idleOpen[1];
	animDatabase_["IdleHoldingUmbrella"] = animation_idleHoldingUmbrella[0];

	auto animation_run{ Lumina::CG3D::LoadAnimationFile("Cool_Run.gltf", "Assets/Neki") };
	auto animation_forwardFlip{ Lumina::CG3D::LoadAnimationFile("Forward_Flip.gltf", "Assets/Neki") };

	animDatabase_["Run"] = animation_run[0];
	animDatabase_["ForwardFlip"] = animation_forwardFlip[0];

	auto animation_swinging{ Lumina::CG3D::LoadAnimationFile("Swinging.gltf", "Assets/Neki") };

	animDatabase_["Swinging"] = animation_swinging[5];

	auto animation_guard{ Lumina::CG3D::LoadAnimationFile("guard.gltf", "Assets/Neki") };

	animDatabase_["Guard"] = animation_guard[1];

	auto animation_atkX1{ Lumina::CG3D::LoadAnimationFile("ATKY1_H2.gltf", "Assets/Neki") };
	auto animation_atkX2{ Lumina::CG3D::LoadAnimationFile("ATKY2_H2.gltf", "Assets/Neki") };
	auto animation_atkX3{ Lumina::CG3D::LoadAnimationFile("ATKY3_H3.gltf", "Assets/Neki") };
	auto animation_atkRot{ Lumina::CG3D::LoadAnimationFile("Rotate_H.gltf", "Assets/Neki") };
	auto animation_airDiveAttack{ Lumina::CG3D::LoadAnimationFile("AirDiveAttack.gltf", "Assets/Neki") };
	
	animDatabase_["AtkX1"] = animation_atkX1[0];
	animDatabase_["AtkX2"] = animation_atkX2[0];
	animDatabase_["AtkX3"] = animation_atkX3[0];
	animDatabase_["AtkRot"] = animation_atkRot[0];
	animDatabase_["AirDiveAttack"] = animation_airDiveAttack[4];

	auto animation_reverseCharge{ Lumina::CG3D::LoadAnimationFile("ReverseCharge.gltf", "Assets/Neki") };
	auto animation_reverseChargeAttack{ Lumina::CG3D::LoadAnimationFile("ReverseChargeAttack.gltf", "Assets/Neki") };

	animDatabase_["ReverseCharge"] = animation_reverseCharge[1];
	animDatabase_["ReverseChargeAttack"] = animation_reverseChargeAttack[2];
}

void Player::Initialize() {

	Position_ = { 0.0f, 10.0f, 0.0f };

	attackDataBase_ = LoadAttackDatabase("Assets/Data/attacks.json");

	InitializeStates();
	InitializeComponents();

	inputHandler_.SetPlayer(this);

	rightHandJoint_.SetType(AttachmentType::PlayerHand);
	rightHandJoint_.SetAcceptType(AttachmentType::UmbrellaHandle);
	rightHandJoint_.SetInfo({ 0.0f,-0.0f,0.0f }, { 0.0f,0.0f,0.0f });

	backJoint_.SetType(AttachmentType::PlayerBack);
	backJoint_.SetAcceptType(AttachmentType::UmbrellaHandle);
	backJoint_.SetInfo({ 0.0f,-0.0f,-0.3f }, { 0.0f,0.0f,0.0f });
	backJoint_.SetRot({0.0f,0.0f,Lumina::Math::DegToRad(225.0f)});

	umbrella_ = std::make_unique<Umbrella::Main>();
	umbrella_->Initialize();

	umbrella_->handle_->GetBaseJoint()->AttachTo(GetBackJoint());

	motionController_ = std::make_unique<MotionController>();

	LoadAnimation();
	PlayAnimation("Idle", true);

	// =====================
	// 【 当たり判定の設定 】
	// =====================

	// 1. コライダーの生成
	collider_ = std::make_unique<ConvexCollider>();
	collider_->SetUserData(this);

	// 2. 属性の設定（自分はPlayer、当たる相手はEnemyやEnemyの攻撃）
	collider_->SetMyType(COL_Player);
	collider_->SetYourType(COL_Enemy | COL_Enemy_Attack | COL_Ground | COL_Umbrella_Ground | COL_Warp);

	// 3. ローカル頂点データの設定（例：プレイヤーを囲む四角形やひし形など）
	std::vector<Vector3> localVertices = {
		{-0.7f, -0.0f, 0.0f}, // 左下
		{ 0.7f, -0.0f, 0.0f}, // 右下
		{ 0.7f,  2.8f, 0.0f},  // 右上
		{ -0.7f,  2.8f, 0.0f }, // 左上
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
						
					}
					if (this->myVelocity_.Y < 0.0f) {
						
					}
					this->externalVelocity_.Y = 0.0f;
					this->myVelocity_.Y = 0.0f;
				}
			}

			//Ground* ground = static_cast<Ground*>(other->GetUserData());
			//if (ground != nullptr) {
			//
			//}
		}
		else if (other->GetMyType() == COL_Umbrella_Ground) {

			// =========================
			// 【 めり込み解消処理 】
			// =========================
			Vector3 actualPush = { -pushOut.X, -pushOut.Y, -pushOut.Z };


			Vector3 normal = actualPush;
			float length = sqrtf(normal.X * normal.X + normal.Y * normal.Y + normal.Z * normal.Z);
			if (length > 0.0f) {
				normal.X /= length;
				normal.Y /= length;
				normal.Z /= length;
			}

			// 足元に地面があるかのチェック
			if (normal.Y > 0.8f) {
				if (this->externalVelocity_.Y <= 0.0f) {
					Vector3 pos = Position_;
					pos.X += actualPush.X;
					pos.Y += actualPush.Y;
					pos.Z += actualPush.Z;
					Position_ = pos;
					this->onGround_ = true;

					// バウンドする
					//if (this->externalVelocity_.Y < 0.0f) {
					//	this->externalVelocity_.Y = normal.Y * 4.5f; // バウンドの強さを調整
					//}
				}
			}
		}
	};

	smashCollider_ = std::make_unique<ConvexCollider>();
	smashCollider_->SetMyType(COL_None);
	smashCollider_->SetYourType(COL_Enemy);
	smashCollider_->SetUserData(this);

	WorldMatrix_ = std::make_unique<Matrix4x4>();
}

void Player::Update(float deltaTime) {

	// 死ぬ
	if (this->status_->IsDead()) {
		ChangeMovementState(restrictedState_.get());
		ChangeActionState(deadState_.get());
	}

	// 移動量の初期化
	moveAmount_ = { 0.0f,0.0f,0.0f };

	// 入力関係の処理
	inputHandler_.HandleInput();
	ApplyInputMask();
	// コンポーネントの更新
	if (mana_) {
		mana_->Update(deltaTime);
	}
	if (status_) {
		status_->Update(deltaTime);
	}

	ThrowUpdate(deltaTime);

	if (IsButtonUp(inputData_.shoot)) {
		umbrella_->top_->SetPlayerPos(GetPosition());
		if (inputData_.aim == ButtonState::Held) {
			umbrella_->top_->StartRecall();
		}

		if (umbrella_->top_->IsRecalling()) {
			Vector3 toPlayer = GetPosition() - umbrella_->top_->GetRootJoint()->GetPos();

			// 2. 距離を測っておく（回収判定用）
			float distance = sqrt(toPlayer.X * toPlayer.X + toPlayer.Y * toPlayer.Y);

			if (distance < 1.0f) {
				umbrella_->top_->GetRootJoint()->AttachTo(umbrella_->handle_->GetTipJoint());
				umbrella_->top_->GetRootJoint()->SetInfo({ 0.0f,0.0f,0.0f }, { 0.0f,0.0f,0.0f });

				umbrella_->top_->ChangeState(new UmbrellaStates::Attached());
				umbrella_->top_->ChangeForm(UmbrellaForm::Closed);
			}
		}
	}

	// ステートの更新
	if (currentMovementState_) {
		currentMovementState_->Update(deltaTime);
	}
	if (currentActionState_) {
		currentActionState_->Update(deltaTime);
	}

	if (this->status_->IsDead())return;

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
	Position_ += moveAmount_;
	EulerAngle_.Y = eyesDirection_.X > 0.0f ? Lumina::Math::DegToRad(90.0f) : Lumina::Math::DegToRad(270.0f);

	collider_->SetWorldPosition(GetPosition());
	*WorldMatrix_ = Game::MathUtils::SRT(Scale_, EulerAngle_, Position_);

	UpdateAnimation();
	auto it = PlayerSkinnedInstance_->Skeleton_.IDX_Joint.find("Bone.024");

	// 見つかったかどうかチェック
	if (it != PlayerSkinnedInstance_->Skeleton_.IDX_Joint.end()) {
		rightHandJoint_.Update(); // 右手Joint自身の行列を計算
		rightHandJoint_.MultiplyMatrixToMe(PlayerSkinnedInstance_->Skeleton_.ARR_Joint[it->second].SkeletonSpace);
		rightHandJoint_.MultiplyMatrixToMe(*WorldMatrix_);
	}

	auto it2 = PlayerSkinnedInstance_->Skeleton_.IDX_Joint.find("Bone.003");
	// 見つかったかどうかチェック
	if (it2 != PlayerSkinnedInstance_->Skeleton_.IDX_Joint.end()) {
		backJoint_.Update(); // 右手Joint自身の行列を計算
		backJoint_.MultiplyMatrixToMe(PlayerSkinnedInstance_->Skeleton_.ARR_Joint[it2->second].SkeletonSpace);
		backJoint_.MultiplyMatrixToMe(*WorldMatrix_);
	}

	// 傘
	umbrella_->Update(deltaTime);

	// Colliderに設定
	Vector3 angle = EulerAngle_;
	angle.Y = 0.0f;
	*WorldMatrix_ = Game::MathUtils::SRT(Scale_, angle, Position_);
	collider_->SetWorldMatrix(*WorldMatrix_);

	// 仮 SmashCollider
	smashCollider_->SetWorldPosition(GetPosition());
	smashCollider_->SetWorldMatrix(*WorldMatrix_);

	if (warpTimer_ < MAX_WARPTIME) {
		warpTimer_ += deltaTime;
	}
	else if(warpTimer_ > MAX_WARPTIME) {
		smashCollider_->SetMyType(COL_None);
		smashCollider_->ClearVertices();
		warpTimer_ = MAX_WARPTIME;
	}

	*WorldMatrix_ = Game::MathUtils::SRT(Scale_, EulerAngle_, Position_);

	#if defined(_DEBUG)
	Vector3 test = rightHandJoint_.GetPos();
	ImGui::DragFloat3("RHandJoint", &test.X);

	// 右てのジョイントの行列を表示
	for (int i = 0;i < 4; i++) {
		ImGui::Text("RHandJoint Matrix %d: %f, %f, %f, %f", i,
			rightHandJoint_.GetMatrix()[i].Get(0),
			rightHandJoint_.GetMatrix()[i].Get(1),
			rightHandJoint_.GetMatrix()[i].Get(2),
			rightHandJoint_.GetMatrix()[i].Get(3)
		);
	}

	// 右手ボーンのマトリックス
	for (int i = 0; i < 4; i++) {
		ImGui::Text("RightHandBone Matrix Row %d: %f, %f, %f, %f", i,
			PlayerSkinnedInstance_->Skeleton_.ARR_Joint[it->second].SkeletonSpace[i].Get(0),
			PlayerSkinnedInstance_->Skeleton_.ARR_Joint[it->second].SkeletonSpace[i].Get(1),
			PlayerSkinnedInstance_->Skeleton_.ARR_Joint[it->second].SkeletonSpace[i].Get(2),
			PlayerSkinnedInstance_->Skeleton_.ARR_Joint[it->second].SkeletonSpace[i].Get(3)
		);
	}

	Vector3 test2 = backJoint_.GetPos();
	ImGui::DragFloat3("BackJoint", &test2.X);
	Vector3 backRot = backJoint_.GetRot();
	ImGui::DragFloat3("BackRot", &backRot.X, 0.1f);
	backJoint_.SetRot(backRot);

	Vector3 colliderPos = collider_->GetWorldPosition();
	ImGui::DragFloat3("colliderPos", &colliderPos.X);

	if (ImGui::Button("Take Damage")) {
		this->status_->TakeDamage(10.0f);
	}

	if (ImGui::Button("Take Experience")) {
		GainXp(10);
	}

	ImGui::Text("Level : %d", this->experience_->GetLevel());
	ImGui::Text("Current Xp : %d", this->experience_->GetCurrentXp());
	ImGui::Text("Next Xp : %d", this->experience_->GetNextLevelXp());
	ImGui::Text("HP : %f / %f", this->status_->GetHp(), this->status_->GetMaxHp());
	ImGui::Text("Umbrella Hp : %f", this->umbrella_->top_->GetStatusComponent().GetHp());
	ImGui::Text("External Velocity Y : %f", this->externalVelocity_.Y);
	ImGui::Text("My Velocity X : %f", this->myVelocity_.X);
	if (ImGui::Button("Take Damage(Umbrella)")) {
		this->umbrella_->top_->GetStatusComponent().TakeDamage(10.0f);
	}
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

	PlayerSkinnedInstance_->MeshScale_ = Scale_;
	PlayerSkinnedInstance_->MeshRotate_ = EulerAngle_;
	PlayerSkinnedInstance_->MeshTranslate_ = Position_;

	// * イベント発行
	// * 左右移動
	if (myVelocity_.X * myVelocity_.X + myVelocity_.Y * myVelocity_.Y > 0.2f) {
		Game::Event::InGame::OnPlayerMove event_OnPlayerMove{};
		event_OnPlayerMove.Player = this;
		event_OnPlayerMove.Velocity = myVelocity_;
		Lumina::Context::Instance().EventContext().TriggerEvent(std::move(event_OnPlayerMove));
	}
	if (inputData_.jump != ButtonState::None) {
		Game::Event::InGame::OnPlayerJump event_OnPlayerJump{};
		event_OnPlayerJump.Player = this;
		event_OnPlayerJump.Velocity = myVelocity_;
		Lumina::Context::Instance().EventContext().TriggerEvent(std::move(event_OnPlayerJump));
	}
	/*if (currentActionState_.attack == ButtonState::Pressed) {
		Lumina::Context::Instance().EventContext().TriggerEvent(
			std::move(Game::Event::InGame::OnPlayerAttack{})
		);
	}*/
}

// メッシュバッチ自体はMeshManager::BatchBegin()とBatchEnd()の間に入れないといけないので
// Draw()の中からメッシュをバッチするのであればシーンのほうのPlayer::Draw()も
// BatchBegin()とBatchEnd()の間で呼び出さなくてはならない
void Player::Draw() {
	if (status_->IsDead())return;

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

void Player::ReturnToMeUmbrella() {
	umbrella_->top_->GetRootJoint()->AttachTo(umbrella_->handle_->GetTipJoint());
	umbrella_->top_->GetRootJoint()->SetInfo({ 0.0f,0.0f,0.0f }, { 0.0f,0.0f,0.0f });

	umbrella_->top_->ChangeState(new UmbrellaStates::Attached());
	umbrella_->top_->ChangeForm(UmbrellaForm::Closed);
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
	deadState_ = std::make_unique<Action::Dead>();deadState_->SetInfo(this);
	sheatheWeaponState_ = std::make_unique<Action::SheatheWeapon>();sheatheWeaponState_->SetInfo(this);
	drawWeaponState_ = std::make_unique<Action::DrawWeapon>();drawWeaponState_->SetInfo(this);
	normalSheathedState_ = std::make_unique<Action::NormalSheathed>();normalSheathedState_->SetInfo(this);
	normalDrawnState_ = std::make_unique<Action::NormalDrawn>();normalDrawnState_->SetInfo(this);
	attackState_ = std::make_unique<Action::Attack>();attackState_->SetInfo(this);
	guardState_ = std::make_unique<Action::Guard>();guardState_->SetInfo(this);
	reverseChargeState_ = std::make_unique<Action::ReverseCharge>();reverseChargeState_->SetInfo(this);
	reverseAttackState_ = std::make_unique<Action::ReverseAttack>();reverseAttackState_->SetInfo(this);
	throwUmbrellaState_ = std::make_unique<Action::ThrowUmbrella>();throwUmbrellaState_->SetInfo(this);
	evasionState_ = std::make_unique<Action::Evasion>();evasionState_->SetInfo(this);

	umbrellaOpenState_ = std::make_unique<Action::UmbrellaOpen>();umbrellaOpenState_->SetInfo(this);
	umbrellaCloseState_ = std::make_unique<Action::UmbrellaClose>();umbrellaCloseState_->SetInfo(this);
	umbrellaReverseState_ = std::make_unique<Action::UmbrellaReverse>();umbrellaReverseState_->SetInfo(this);
	repairUmbrellaState_ = std::make_unique<PlayerStates::Action::RepairUmbrella>();repairUmbrellaState_->SetInfo(this);

	// 最初の設定
	currentActionState_ = normalSheathedState_.get();
}

void Player::InitializeComponents() {
	//// ManaComponentの初期化 ////
	mana_ = std::make_unique<ManaComponent>(100.0f);
	//// StatusComponentの初期化 ////
	// HP , Attack , Defence
	status_ = std::make_unique<StatusComponent>(100.0f, 20.0f, 1.0f);

	experience_ = std::make_unique<ExperienceComponent>();
	experience_->Initialize();
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
	if (inputData_.jump == ButtonState::Pressed) {
		if (this->onGround_ || this->jumpCoyoteTimer_ < JUMP_COYOTE_MAX_TIME) {
			// Y軸に上向きの初速（ジャンプ力）を与える！
			float jumpPower = 12.0f; // 調整
			externalVelocity_.Y = jumpPower;// 初速
			// フラグの処理
			this->onGround_ = false;
			this->jumpCoyoteTimer_ = this->JUMP_COYOTE_MAX_TIME;

			PlayAnimation("ForwardFlip", false);

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

///////////////////
///
///   照準・射撃
///
///////////////////
void Player::ThrowUpdate([[maybe_unused]]float deltaTime) {
	if (normalDrawnState_.get() == currentActionState_) {
		if (umbrella_->top_->GetUmbrellaForm() != UmbrellaForm::Flying && umbrella_->top_->GetUmbrellaForm() != UmbrellaForm::AirStop) {
			// 照準を押しているときは飛ばす方向を決めれる。
			// ただし、抜刀済みのみ
			float scalar = 5.0f;
			if (inputData_.aim == ButtonState::Pressed) {
				targetPos_.X = GetPosition().X + scalar;
				targetPos_.Y = GetPosition().Y;
			}
			else if (inputData_.aim == ButtonState::Held) {
				// ここは要改善
				targetPos_.X = GetPosition().X + (inputData_.aimingDirectionX * scalar);
				targetPos_.Y = GetPosition().Y + (inputData_.aimingDirectionY * scalar);
				targetPos_.Z = 0.0f;

				// 照準のときのみ射撃する
				if (inputData_.shoot == ButtonState::Pressed) {
					// ここで投げる処理
					ChangeActionState(throwUmbrellaState_.get());
				}
			}
		}
	}
}

void Player::WarpToUmbrella() {
	// 1. 傘の現在のワールド座標を取得
	Vector3 targetPos = umbrella_->top_->GetRootJoint()->GetWorldPos();
	targetPos.Z = 0.0f;

	// 2. プレイヤーの座標を傘の場所へ上書き
	this->SetPosition(targetPos);

	smashCollider_->ClearVertices();
	smashCollider_->SetMyType(COL_Player_Attack_SmashWave);
	float radius = 3.0f;
	float jakkanue = 0.7f;
	smashCollider_->SetVertices({
		{  0.0000f * radius,  1.0000f * radius + jakkanue, 0.0f }, // 1. 真上
		{ -0.9511f * radius,  0.3090f * radius + jakkanue, 0.0f }, // 2. 左上
		{ -0.5878f * radius, -0.8090f * radius + jakkanue, 0.0f }, // 3. 左下
		{  0.5878f * radius, -0.8090f * radius + jakkanue, 0.0f }, // 4. 右下
		{  0.9511f * radius,  0.3090f * radius + jakkanue, 0.0f }  // 5. 右上
	});

	warpTimer_ = 0.0f;

	// 3. 飛んでいた傘を手元に戻す（アタッチし直す）
	umbrella_->top_->GetRootJoint()->AttachTo(umbrella_->handle_->GetTipJoint());
	umbrella_->top_->GetRootJoint()->SetInfo({ 0.0f,0.0f,0.0f }, { 0.0f,0.0f,0.0f });

	this->externalVelocity_.Y = 0.0f;

	umbrella_->top_->ChangeState(new UmbrellaStates::Attached());
	umbrella_->top_->ChangeForm(UmbrellaForm::Closed);

	// 4. 空中状態にするなどの後処理
	ChangeMovementState(airborneState_.get());
	ChangeActionState(normalDrawnState_.get());
}

///////////////////
///
///   アニメーション
///
///////////////////

void Player::UpdateAnimation() {
	if (!currentAnim_) return;

	animTimer_ += 1.0f / 60.0f * 4.0f;

	if (currentAnimName_ == "Run") {
		animTimer_ -= 1.0f / 60.0f * 2.0f;
	}
	else if (currentAnimName_ == "AtkRot") {
		animTimer_ += 1.0f / 60.0f * 0.0f;
	}
	else if (currentAnimName_ == "AtkX3") {
		animTimer_ += 1.0f / 60.0f * 1.2f;
	}
	else if (currentAnimName_ == "ForwardFlip") {
		animTimer_ -= 1.0f / 60.0f * 1.0f;
	}
	else if(currentAnimName_ == "AirDiveAttack") {
		animTimer_ -= 1.0f / 60.0f * 1.0f;
	}
	else if (currentAnimName_ == "IdleOpen") {
		animTimer_ -= 1.0f / 60.0f * 3.0f;
	}
	else if (currentAnimName_ == "Swinging") {
		animTimer_ -= 1.0f / 60.0f * 3.0f;
	}

	if (currentAnimName_ == "Run") {
		animTimer_ -= 1.0f / 60.0f * 1.0f;
	}

	if (isLoop_) {
		// ループする場合は fmod で 0 ～ Duration に収める
		animTimer_ = std::fmod(animTimer_, currentAnim_->DurationInSeconds);
	}
	else {
		// ループしない場合は Duration で止める（これなら > 判定でOK）
		if (animTimer_ > currentAnim_->DurationInSeconds) {
			animTimer_ = currentAnim_->DurationInSeconds;
		}
	}

	Lumina::CG3D::Update(
		PlayerSkinnedInstance_->SkinCluster_,
		PlayerSkinnedInstance_->Skeleton_,
		*currentAnim_,
		animTimer_
	);
}

void Player::GainXp(uint32_t amount) {
	// 1. 経験値を追加して、レベルアップしたか判定
	if (experience_->AddExperience(amount)) {

		// 2. レベルアップした瞬間の現在レベルを取得
		uint32_t newLevel = experience_->GetLevel();

		// 3. StatusComponent にレベルを伝えて、ステータスを再計算させる！
		// (例: レベルに応じて最大HPや攻撃力のベース値を乗算/加算する)
		status_->ApplyLevelBonus(newLevel);

		// 演出：レベルアップエフェクトやSEを鳴らす！
		// EffectManager::Spawn("LevelUp", GetPosition());
	}
}

void Player::TakeDamage(float damege, const Vector3& pos) {
	Vector3 vector = pos - GetPosition();
	if (currentActionState_ == evasionState_.get()) {
		// 攻撃を喰らわない -> MISS

		return;
	}
	float actualDamage = damege;
	if (currentActionState_ == guardState_.get()) {
		// もしガード状態なら
		float dot = eyesDirection_.X * vector.X + eyesDirection_.Y * vector.Y;
		if (dot > 0.0f) {
			// ガードしている方向と同じなら
			actualDamage *= 0.0f;
			externalVelocity_.X = -vector.X * 1.5f;
		}
	}

	status_->TakeDamage(actualDamage);

	if (actualDamage > 0.0f) {
		externalVelocity_.X = -vector.X * 0.5f * actualDamage;
		externalVelocity_.Y = 0.05f;
	}
}

void Player::AirDiveAttack() {
	// 敵に当たったら、上方向に強制ポップ！
	float hopPower = 18.0f;
	myVelocity_.Y = hopPower;

	// 突進は終わったので、少しだけ左右慣性を残すかゼロにする
	myVelocity_.X = myVelocity_.X * -0.6f;

	// 浮いたので、アクションを通常（空中）状態に戻して、次の行動（開く等）を受け付ける
	ChangeActionState(normalDrawnState_.get());
	ChangeMovementState(airborneState_.get());

	PlayAnimation("ForwardFlip", false);
}