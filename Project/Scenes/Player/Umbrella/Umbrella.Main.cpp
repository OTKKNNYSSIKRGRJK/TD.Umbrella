module Game.Umbrella : Main;

//	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//	Codes temporarily commented out should be revised afterwards carefully!
//	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

import Lumina.Core.Math;
import Lumina.Main;
import Lumina.MeshManager;
import Lumina.D3D12.Aux.View;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace {
	using Vector3 = Lumina::Math::F32x3;
}

namespace Umbrella {

	////////////////////////
	///
	///  傘の「持ち手」の部分
	///
	///////////////////////
	void Handle::Initialize() {
		
		baseJoint_.SetType(AttachmentType::UmbrellaHandle);
		baseJoint_.SetAcceptType(AttachmentType::PlayerHand | AttachmentType::PlayerBack);
		baseJoint_.SetInfo({0.0f,-0.2f,0.0f}, {0.0f,0.0f,0.0f});

		tipJoint_.SetType(AttachmentType::UmbrellaTip);
		tipJoint_.SetAcceptType(AttachmentType::UmbrellaTopRoot | AttachmentType::UmbrellaHandle);
		tipJoint_.SetInfo({ 0.0f,1.89f,0.0f }, { 0.0f,0.0f,0.0f });
		tipJoint_.AttachTo(&baseJoint_);
	}

	void Handle::Update([[maybe_unused]] float deltaTime) {
		
		baseJoint_.Update();

		tipJoint_.Update(); 

		Vector3 tipPos = tipJoint_.GetWorldPos();
		Vector3 basePos = baseJoint_.GetWorldPos();
		#if defined(_DEBUG)
		ImGui::DragFloat3("TipPos", &tipPos.X);
		ImGui::DragFloat3("BasePos", &basePos.X);
		#endif
	}

	void Handle::Draw() {
		// メッシュバッチ・描画マネージャ
		auto& meshMngr{ Lumina::Context::Instance().MeshContext() };

		// 描画してほしいメッシュをバッチ
		// --- パラメータ ---
		// Lumina::MeshShaderAsset const* mesh_ : メッシュ（シーンのほうで読み込み）
		// uint32_t num_Instances_ : インスタンス数（今のパイプラインではインスタンシングやってないから1固定で）
		// D3D12_CPU_DESCRIPTOR_HANDLE localCBV_Material_ : メッシュマテリアルバッファのCBV
		// Matrix4x4 const& world_ : ワールド行列
		meshMngr.Batch(*Mesh_, 1U, MeshMaterialCBV_, baseJoint_.GetMatrix());
	}

	////////////////////////
	///
	///  傘の「かさ」の部分
	///
	///////////////////////
	void Top::Initialize() {

		rootJoint_.SetAcceptType(AttachmentType::UmbrellaTip);
		rootJoint_.SetType(AttachmentType::UmbrellaTopRoot);
		rootJoint_.SetInfo({ 0.0f,0.0f,0.0f }, { 0.0f,0.0f,0.0f });

		// ======================
		// ステータス
		// ======================
		status_ = std::make_unique<StatusComponent>(100.0f, 10.0f, 10.0f);
		mana_ = std::make_unique<ManaComponent>(100.0f);
		mana_->ConsumeMana(mana_->GetCurrentMana());

		// ======================
		// 当たり判定
		// ======================
		collider_ = std::make_unique<ConvexCollider>();

		collider_->SetMyType(COL_None);
		collider_->SetYourType(COL_Enemy | COL_Player | COL_Ground);

		collider_->SetUserData(this);

		collider_->SetEnableHitHistory(true);

		collider_->onCollisionCallback = [this](Collider* other, const Vector3& outPush) {
			if (other->GetMyType() == COL_Ground) {
				Vector3 actualPush = -outPush;
				rootJoint_.SetPos(rootJoint_.GetPos() + actualPush);
			}
			/*
			* Enemy* enemy =
			* if(enemy){
			*	status_->TakeDamage(enemyの攻撃力);
			*	if(status_->IsDead()){
			*		isBroken_ = true;
			*		top_->ChangeState(new UmbrellaStates::Broken());// 壊れたステート
			*	}
			* }
			*/
		};

		UpdateColliderShape();
	}

	void Top::Update(float deltaTime) {
		/*stateの更新処理*/
		if (currentState_) {
			currentState_->Update(deltaTime);
		}
		UpdateColliderShape();

		switch (form_) {
		case UmbrellaForm::Closed:
			rootJoint_.SetRot({ 0.0f,0.0f,0.0f });
			#if defined(_DEBUG)
			ImGui::Text("Close");
			#endif
			break;
		case UmbrellaForm::Opened:
			rootJoint_.SetRot({ 0.0f,0.0f,0.0f });
			#if defined(_DEBUG)
			ImGui::Text("Opened");
			#endif
			break;
		case UmbrellaForm::Reverse:
			rootJoint_.SetRot({ Lumina::Math::DegToRad(180.0f),0.0f,0.0f });
			#if defined(_DEBUG)
			ImGui::Text("Reverse");
			#endif
			break;
		case UmbrellaForm::Flying:
			rootJoint_.SetRot({ 0.0f,0.0f,0.0f });
			#if defined(_DEBUG)
			ImGui::Text("Flying");
			#endif
			break;
		case UmbrellaForm::AirStop:
			#if defined(_DEBUG)
			ImGui::Text("AirStop");
			#endif
			break;
		}

		rootJoint_.Update();

		if (collider_->GetMyType() != COL_None) {
			
		}
		collider_->SetWorldPosition(rootJoint_.GetWorldPos());

		collider_->SetWorldMatrix(rootJoint_.GetMatrix());

		// 全てが終わったらFalse
		isRecalling_ = false;

		[[maybe_unused]] float mana = mana_->GetCurrentMana();
		#if defined(_DEBUG)
		ImGui::Text("Over Mana : %f", mana);
		#endif
	}

	void Top::Draw() {
		if (form_ == UmbrellaForm::Closed) {
			// メッシュバッチ・描画マネージャ
			auto& meshMngr{ Lumina::Context::Instance().MeshContext() };

			// 描画してほしいメッシュをバッチ
			// --- パラメータ ---
			// Lumina::MeshShaderAsset const* mesh_ : メッシュ（シーンのほうで読み込み）
			// uint32_t num_Instances_ : インスタンス数（今のパイプラインではインスタンシングやってないから1固定で）
			// D3D12_CPU_DESCRIPTOR_HANDLE localCBV_Material_ : メッシュマテリアルバッファのCBV
			// Matrix4x4 const& world_ : ワールド行列
			meshMngr.Batch(*Mesh_, 1U, MeshMaterialCBV_, rootJoint_.GetMatrix());
		}
		else {
			// メッシュバッチ・描画マネージャ
			auto& meshMngr{ Lumina::Context::Instance().MeshContext() };

			// 描画してほしいメッシュをバッチ
			// --- パラメータ ---
			// Lumina::MeshShaderAsset const* mesh_ : メッシュ（シーンのほうで読み込み）
			// uint32_t num_Instances_ : インスタンス数（今のパイプラインではインスタンシングやってないから1固定で）
			// D3D12_CPU_DESCRIPTOR_HANDLE localCBV_Material_ : メッシュマテリアルバッファのCBV
			// Matrix4x4 const& world_ : ワールド行列
			meshMngr.Batch(*MeshOpen_, 1U, MeshMaterialCBV_, rootJoint_.GetMatrix());
		}
	}

	void Top::ChangeState(UmbrellaStates::Base* newState) {
		if (currentState_) {
			currentState_->Exit();
			delete currentState_;
		}
		currentState_ = newState;
		if (currentState_) {
			currentState_->SetTop(this);
			currentState_->Enter();
		}
	}

	void Top::UpdateColliderShape() {
		std::vector<Vector3> vertices;
		collider_->ClearVertices();

		if (form_ == UmbrellaForm::Closed) {
			// 閉じた状態：細長い剣のような判定（ローカル座標で定義）
			// 幅0.2m、長さ1.5m(Y方向) の直方体の8頂点などを設定
			float scale = 2.5f;
			float w = 0.5f * scale;  // 半径1mくらいの広さ
			float h = 1.5f * scale;  // 厚み
			float y = -1.0f * scale;  // 持ち手から少し上の位置
			vertices = {
				{-w, y, -1.0f},{w, y, -1.0f},{w - 0.25f, y + h,-1.0f},{-w + 0.25f, y + h, -1.0f},
				{-w, y, 1.0f},{w, y, 1.0f},{w - 0.25f, y + h,1.0f},{-w + 0.25f, y + h, 1.0f}
			};
		}
		else if (form_ == UmbrellaForm::Opened || form_ == UmbrellaForm::Flying || form_ == UmbrellaForm::AirStop) {
			float w = 1.0f;  // 半径1mくらいの広さ
			float h = 0.9f;  // 厚み
			float y = 0.0f;  // 持ち手から少し上の位置
			vertices = {
				{-w, y, -1.0f},{w, y, -1.0f},{-w, y + h, -1.0f},{w, y + h,-1.0f},
				{-w, y, 1.0f},{w, y, 1.0f},{-w, y + h, 1.0f},{w, y + h,1.0f}
			};
		}
		else if (form_ == UmbrellaForm::Reverse) {
			// 逆さ状態：雨（マナ）を受け止めるための、上向きのお椀（または広い箱）のような判定
			// ※とりあえず、開いた傘と同じか、少し広めの直方体（板）にしておく
			float w = 1.0f;  // 半径1mくらいの広さ
			float h = 1.0f;  // 厚み
			float y = 0.0f;  // 持ち手から少し上の位置
			vertices = {
				{-w, y, -w}, { w, y, -w}, {-w, y,  w}, { w, y,  w},
				{-w, y + h,-w}, { w, y + h,-w}, {-w, y + h, w}, { w, y + h, w}
			};
		}

		// ConvexCollider に頂点をセットする関数（無ければ Collider.h に追加してください）
		collider_->SetVertices(vertices);
	}

	////////////////////////
	///
	///  傘の「メイン」の部分
	///
	///////////////////////
	void Main::Initialize() {
		handle_ = std::make_unique<Handle>();
		top_ = std::make_unique<Top>();

		handle_->Initialize();
		top_->Initialize();

		top_->GetRootJoint()->AttachTo(handle_->GetTipJoint());
		top_->ChangeState(new UmbrellaStates::Attached());
	}

	void Main::InitializeData() {
		top_->GetRootJoint()->AttachTo(handle_->GetTipJoint());
		top_->ChangeState(new UmbrellaStates::Attached());
		top_->ChangeForm(UmbrellaForm::Closed);
	}

	void Main::Update(float deltaTime) {
		handle_->Update(deltaTime);
		top_->Update(deltaTime);
	}

	void Main::Draw() {
		handle_->Draw();
		top_->Draw();
	}
}