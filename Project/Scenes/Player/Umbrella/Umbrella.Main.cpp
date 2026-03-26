module Game.Umbrella : Main;

//	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//	Codes temporarily commented out should be revised afterwards carefully!
//	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

namespace Umbrella {

	////////////////////////
	///
	///  傘の「持ち手」の部分
	///
	///////////////////////
	void Handle::Initialize() {
		//obj_ = std::make_unique<ModelObject>();
		//p_fngine_ = f;
		//obj_->textureName_ = "GridLine";
		//obj_->modelName_ = "UmbrellaHandle";
		//obj_->Initialize(p_fngine_);

		baseJoint_.SetType(AttachmentType::UmbrellaHandle);
		baseJoint_.SetAcceptType(AttachmentType::PlayerHand | AttachmentType::PlayerBack);
		baseJoint_.SetInfo({0.0f,-0.5f,0.0f}, {0.0f,0.0f,0.0f});

		tipJoint_.SetAcceptType(AttachmentType::UmbrellaTopRoot);
		tipJoint_.SetType(AttachmentType::UmbrellaTip);
		tipJoint_.SetInfo({ 0.0f,0.5f,0.0f }, { 0.0f,0.0f,0.0f });
	}

	void Handle::Update([[maybe_unused]] float deltaTime) {
		
		baseJoint_.Update();
		//obj_->worldTransform_.mat_ = baseJoint_.GetMatrix();

		//tipJoint_.SetPos({ obj_->worldTransform_.mat_.m[3][0],obj_->worldTransform_.mat_.m[3][1] + 1.25f ,obj_->worldTransform_.mat_.m[3][2]});
		tipJoint_.Update(); 
	}

	void Handle::Draw() {
		//obj_->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(obj_->worldTransform_.mat_));
		//obj_->Draw();
	}

	////////////////////////
	///
	///  傘の「かさ」の部分
	///
	///////////////////////
	void Top::Initialize() {
		//obj_ = std::make_unique<ModelObject>();
		//p_fngine_ = f;
		//obj_->textureName_ = "GridLine";
		//obj_->modelName_ = "UmbrellaTopClose";
		//obj_->Initialize(p_fngine_);

		//openObj_ = std::make_unique<ModelObject>();
		//openObj_->textureName_ = "GridLine";
		//openObj_->modelName_ = "UmbrellaTop";
		//openObj_->Initialize(p_fngine_);

		rootJoint_.SetAcceptType(AttachmentType::UmbrellaTip);
		rootJoint_.SetType(AttachmentType::UmbrellaTopRoot);
		rootJoint_.SetInfo({ 0.0f,0.5f,0.0f }, { 0.0f,0.0f,0.0f });
	}

	void Top::Update(float deltaTime) {
		/*stateの更新処理*/
		if (currentState_) {
			currentState_->Update(deltaTime);
		}

		rootJoint_.Update();

		switch (form_) {
		case UmbrellaForm::Closed:
			//obj_->worldTransform_.mat_ = rootJoint_.GetMatrix();
			break;
		case UmbrellaForm::Opened:
			//openObj_->worldTransform_.mat_ = rootJoint_.GetMatrix();
			break;
		}
	}

	void Top::Draw() {
		switch (form_) {
		case UmbrellaForm::Closed:
			//obj_->LocalToWorld();
			//obj_->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(obj_->worldTransform_.mat_));
			//obj_->Draw();
			break;
		case UmbrellaForm::Opened:
			//openObj_->LocalToWorld();
			//openObj_->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(openObj_->worldTransform_.mat_));
			//openObj_->Draw();
			break;
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

	void Main::Update(float deltaTime) {
		handle_->Update(deltaTime);
		top_->Update(deltaTime);
	}

	void Main::Draw() {
		handle_->Draw();
		top_->Draw();
	}
}