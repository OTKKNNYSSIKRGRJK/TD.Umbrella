//module Game.Scene.Clear;
//
//import <vector>;
//import <cmath>;
//import <numbers>;
//import <algorithm>;
//
//import : Impl;
//
//import Game.Scene.Title;
//import Game.Scene.StageSelection;
//
//import GlobalVars;
//
//namespace Game::Scene {
//	namespace {
//		constexpr Lumina::Vec2 FractalOffsets[]{
//			{ -1.0f, -1.0f },
//			{ -1.0f, 1.0f },
//			{ 1.0f, -1.0f },
//			{ 1.0f, 1.0f },
//		};
//		constexpr float FractalScale{ 0.75f };
//		constexpr float FractalRotate{ std::numbers::pi_v<float> * 0.75f };
//
//		void FractalIterate(
//			std::vector<ClearFractalAnimation>& arr_
//		) {
//			std::vector<ClearFractalAnimation> appending;
//			std::vector<ClearFractalAnimation> appending2;
//			for (auto& anim : arr_) {
//				if (anim.Timer < 1.0f) {
//					appending2.emplace_back(anim);
//				}
//				else if (anim.IterationTime < 3) {
//					for (int i = 0; i < 4; ++i) {
//						auto& newAnim = appending.emplace_back();
//						newAnim.ResetTimer();
//						newAnim.InitialPosition = anim.FinalPosition;
//						auto const offset = Lumina::Vec2{
//							FractalOffsets[i].x * anim.FinalScale.x,
//							FractalOffsets[i].y * anim.FinalScale.y
//						};
//						float const cos_Theta{ std::cos(anim.FinalRotation) };
//						float const sin_Theta{ std::sin(anim.FinalRotation) };
//						newAnim.FinalPosition =
//							newAnim.InitialPosition +
//							Lumina::Vec2{
//								offset.x * cos_Theta - offset.y * sin_Theta,
//								offset.x * sin_Theta + offset.y * cos_Theta
//						};
//						newAnim.InitialScale = {};
//						newAnim.FinalScale = anim.FinalScale * FractalScale * FractalScale;
//						newAnim.InitialRotation = anim.FinalRotation;
//						newAnim.FinalRotation = newAnim.InitialRotation + FractalRotate;
//						newAnim.IterationTime = anim.IterationTime + 1;
//						newAnim.Center = anim.Center;
//					}
//
//					anim.ResetTimer();
//					anim.InitialPosition = anim.FinalPosition;
//					anim.InitialRotation = anim.FinalRotation;
//					anim.FinalRotation += FractalRotate;
//					anim.InitialScale = anim.FinalScale;
//					anim.FinalScale *= FractalScale;
//					++anim.IterationTime;
//					appending2.emplace_back(anim);
//				}
//				else if (anim.IterationTime == 3) {
//					anim.ResetTimer();
//					anim.InitialPosition = anim.FinalPosition;
//					anim.FinalPosition *= 0.75f;
//					anim.InitialRotation = anim.FinalRotation;
//					anim.FinalRotation += std::numbers::pi_v<float> * 2.0f;
//					anim.InitialScale = anim.FinalScale;
//					anim.FinalScale = {};
//					++anim.IterationTime;
//					appending2.emplace_back(anim);
//				}
//			}
//			arr_.swap(appending);
//			arr_.insert(arr_.cend(), appending2.cbegin(), appending2.cend());
//		}
//
//		void UpdateFractalAnimation(
//			std::vector<ClearFractalAnimation>& arr_,
//			std::vector<Lumina::Mat4>& arr_Mat_,
//			float omega_
//		) {
//			arr_Mat_.clear();
//
//			float const cos_Omega{ std::cos(omega_) };
//			float sin_Omega{ std::sin(omega_) };
//
//			auto update{
//				[&] (Lumina::Mat4& m_, ClearFractalAnimation& anim_) -> void {
//					anim_.Update();
//					float const theta = anim_.Rotation();
//					float const cos_Theta{ std::cos(theta) };
//					float const sin_Theta{ std::sin(theta) };
//					Lumina::Vec2 const scale = anim_.Scale();
//					float const a{ scale.x * cos_Theta };
//					float const b{ scale.x * sin_Theta };
//					float const c{ scale.y * (-sin_Theta) };
//					float const d{ scale.y * cos_Theta };
//					Lumina::Vec2 const pos = anim_.Position();
//					m_ = {
//						a * cos_Omega - b * sin_Omega, a * sin_Omega + b * cos_Omega, 0.0f, 0.0f,
//						c * cos_Omega - d * sin_Omega, c * sin_Omega + d * cos_Omega, 0.0f, 0.0f,
//						0.0f, 0.0f, 1.0f, 0.0f,
//						pos.x * cos_Omega - pos.y * sin_Omega + anim_.Center.x,
//						pos.x * sin_Omega + pos.y * cos_Omega + anim_.Center.y,
//						0.0f, 1.0f,
//					};
//				}
//			};
//
//			for (auto& anim : arr_) {
//				update(arr_Mat_.emplace_back(), anim);
//			}
//		}
//	}
//
//	namespace {
//		constexpr float Inv = 1.0f / 72.0f;
//		constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };
//	}
//
//	void ClearImpl::Update() {
//		float t = Inv * (NextIterationTime_ + (IterationTime_ & 1) * 36.0f);
//		float const easedT = (t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f));
//		float rot = std::numbers::pi_v<float> * 0.125f * Inv;
//		SceneRotation_ += rot;
//		UpdateFractalAnimation(
//			Arr_FractalAnimation_,
//			Arr_MeshLocalToWorld_,
//			SceneRotation_
//		);
//
//		BackgroundCenter_Prev_ = BackgroundCenter_;
//
//		//// UI
//		//{
//		//	if (IterationTime_ == 1) {
//		//		auto buttonFadeIn{
//		//			[&, this](Lumina::Sprite& button_, int buttonID_) -> void {
//		//				button_.RGBA_.w = ((SelectedButton_ == buttonID_) ? (0.5f) : (0.0625f)) * easedT;
//		//				button_.Translate_.x = 1400.0f + -easedT * 200.0f;
//		//				button_.Translate_.x -= !!(SelectedButton_ == buttonID_) * 10.0f;
//		//			}
//		//		};
//		//		buttonFadeIn(UI_StartButton_, 0);
//		//		buttonFadeIn(UI_ManualButton_, 1);
//		//		buttonFadeIn(UI_ExitButton_, 2);
//		//	}
//		//	else if (IterationTime_ > 1) {
//		//		auto setTranslate{
//		//			[this](Lumina::Sprite& button_, int buttonID_) -> void {
//		//				button_.Translate_.x += (SelectedButton_ == buttonID_) ? (-0.5f) : (0.5f);
//		//				button_.Translate_.x = std::clamp(button_.Translate_.x, 1190.0f, 1200.0f);
//		//			}
//		//		};
//		//		auto setColor{
//		//			[this](Lumina::Sprite& button_, int buttonID_) -> void {
//		//				button_.RGBA_.w *= (SelectedButton_ == buttonID_) ? (1.05f) : (0.95f);
//		//				button_.RGBA_.w = std::clamp(button_.RGBA_.w, 0.0625f, 0.75f);
//		//			}
//		//		};
//		//		setTranslate(UI_StartButton_, 0);
//		//		setTranslate(UI_ManualButton_, 1);
//		//		setTranslate(UI_ExitButton_, 2);
//		//		setColor(UI_StartButton_, 0);
//		//		setColor(UI_ManualButton_, 1);
//		//		setColor(UI_ExitButton_, 2);
//		//	}
//		//}
//
//		// Input
//		{
//			if (Count_FadeIn_ < 0) {
//				auto const& keyboard = Input_->Keyboard();
//				auto updateKeyState{
//					[&](
//						uint32_t& keyState_,
//						Lumina::WinApp::KEY keyCode_
//					) -> void {
//						keyState_ = (keyState_ << 1U) + !!keyboard.IsPressed(keyCode_);
//					}
//				};
//				auto isKeyJustPressed{
//					[&](uint32_t keyState_) constexpr -> bool {
//						return ((keyState_ & 1U) && !(keyState_ & 2U));
//					}
//				};
//				updateKeyState(KeyState_Space_, Lumina::WinApp::KEY::SPACE);
//				if (isKeyJustPressed(KeyState_Space_)) {
//					if (GlobalVars::ReturnToStart) {
//						/*Lumina::SceneManager::Instance().Load<Game::Scene::StageSelection>(
//							"Scene::StageSelection",
//							*DXContext_,
//							const_cast<Lumina::AssetManager const&>(*AssetManager_),
//							*CmdList_Main_,
//							*Input_
//						);
//						Lumina::SceneManager::Instance().Activate<Lumina::Scene::FLAG::UPDATE>("Scene::StageSelection");
//						Lumina::SceneManager::Instance().Activate<Lumina::Scene::FLAG::RENDER>("Scene::StageSelection");*/
//
//						Lumina::SceneManager::Instance().Load<Game::Scene::Title>(
//							"Scene::Title",
//							*DXContext_,
//							const_cast<Lumina::AssetManager const&>(*AssetManager_),
//							*CmdList_Main_,
//							*Input_
//						);
//						Lumina::SceneManager::Instance().Activate<Lumina::Scene::FLAG::UPDATE>("Scene::Title");
//						Lumina::SceneManager::Instance().Activate<Lumina::Scene::FLAG::RENDER>("Scene::Title");
//
//						Lumina::SceneManager::Instance().Deactivate<Lumina::Scene::FLAG::UPDATE>("Scene::InGame");
//						Lumina::SceneManager::Instance().Deactivate<Lumina::Scene::FLAG::RENDER>("Scene::InGame");
//					}
//
//					Count_FadeOut_ = 1;
//				}
//			}
//		}
//
//		/*if (GlobalVars::ReturnToStageSelection) {
//			if (
//				!IsInGameSceneUnloaded_ &&
//				!Lumina::SceneManager::Instance().IsActive<Lumina::Scene::FLAG::UPDATE>("Scene::InGame")
//			) {
//				Lumina::SceneManager::Instance().Unload("Scene::InGame");
//				IsInGameSceneUnloaded_ = 1;
//			}
//		}*/
//
//		auto& rndEngine{ Lumina::Random::Generator() };
//
//		if (NextIterationTime_ >= 36) {
//			FractalIterate(Arr_FractalAnimation_);
//			++IterationTime_;
//
//			auto& anim = Arr_FractalAnimation_.emplace_back();
//			{
//				anim.ResetTimer();
//				anim.InitialPosition = { 0.0f, 0.0f };
//				anim.FinalPosition = { 0.0f, 0.0f };
//				anim.InitialScale = { 0.0f, 0.0f };
//				float const scale = static_cast<float>(rndEngine() % 70) + 30.0f;
//				anim.FinalScale = { scale, scale };
//				anim.InitialRotation = 0.0f;
//				anim.FinalRotation = 0.0f;
//				anim.IterationTime = 0;
//				float const rad = static_cast<float>(rndEngine() % 10) + 10.0f;
//				anim.Center = {
//					rad * 32.0f * std::cos(SceneRotation_ * 5.0f) + 640.0f,
//					rad * 18.0f * std::sin(SceneRotation_ * 5.0f) + 360.0f
//				};
//			}
//
//			NextIterationTime_ = 0;
//		}
//		++NextIterationTime_;
//
//		{
//			static float sparkleTimeFactor{ 0.0f };
//			static int sparkleTimeFactor2{ 144 };
//			static constexpr float inv_48{ 1.0f / 48.0f };
//			sparkleTimeFactor += 0.75f;
//
//			if (sparkleTimeFactor2 >= 144) {
//				float const rad = static_cast<float>(rndEngine() % 20) + 2.5f;
//				Lumina::Float2 const center = {
//					rad * 24.0f * std::cos(SceneRotation_ * 2.4f) + 640.0f,
//					rad * 13.5f * std::sin(SceneRotation_ * 3.6f) + 360.0f
//				};
//
//				for (int i = 0; i < 96; ++i) {
//					float const theta{ i * std::numbers::pi_v<float> * inv_48 };
//					float const vx{ std::cos(theta) };
//					float const vy{ std::sin(theta) };
//					Particle sparkle{};
//					{
//						float v = 5.0f;
//						if (i & 1) {
//							v *= 0.25f + 0.75f * std::abs(std::cos(theta * 2.0f + rad));
//						}
//						else {
//							v *= 0.125f + 0.375f * std::abs(std::sin(theta * 2.0f + rad));
//						}
//						sparkle.Translate = { center.x, center.y, 0.0f };
//						sparkle.Velocity = { vx * v, vy * v - 1.5f, 0.0f };
//						sparkle.Scale.x = 50.0f;
//						sparkle.Scale.y = 50.0f;
//						sparkle.Life = 216.0f;
//						auto const rgb_Base = Lumina::Utils::Color::Convert(
//							Lumina::Utils::Color::HSV{
//								rndEngine() * Inv_0xFFFFFFFF * 45.0f + 15.0f,
//								rndEngine() * Inv_0xFFFFFFFF * 0.5f + 0.5f,
//								0.75f
//							}
//						);
//						auto const rgb_Gaming = Lumina::Utils::Color::Convert(
//							Lumina::Utils::Color::HSV{
//								rndEngine() * Inv_0xFFFFFFFF * 45.0f +
//								sparkleTimeFactor +
//								i * 360.0f * inv_48,
//								rndEngine() * Inv_0xFFFFFFFF * 0.3f + 0.3f,
//								0.95f
//							}
//						);
//						sparkle.RenderData.RGBA = {
//							rgb_Base.R * (0.1f + rgb_Gaming.R * 0.9f),
//							rgb_Base.G * (0.1f + rgb_Gaming.G * 0.9f),
//							rgb_Base.B * (0.1f + rgb_Gaming.B * 0.9f),
//							0.0f
//						};
//						sparkle.RenderData.DiffuseID = 2U;
//						sparkle.RenderData.DiffuseAtlasID = (i & 1) ? (2U) : (3U);
//					}
//					CircularSparkles_->Emit(std::move(sparkle));
//				}
//				sparkleTimeFactor2 = 0;
//			}
//			++sparkleTimeFactor2;
//		}
//
//		List_PointLight_.Clear();
//		List_Matrix_World_LightSphere_.Clear();
//
//		Lumina::List<Particle>::Iterator it_Sparkle{CircularSparkles_->InstanceList()};
//		for (it_Sparkle.Begin(); !it_Sparkle.End(); it_Sparkle.Next()) {
//			auto const& sparkle{ *it_Sparkle };
//
//			if (!List_PointLight_.IsFull()) {
//				auto& light{ List_PointLight_.New() };
//
//				light.WorldPosition = {
//					sparkle.Translate.x,
//					sparkle.Translate.y,
//					sparkle.Translate.z,
//					1.0f
//				};
//				light.RGB = {
//					sparkle.RenderData.RGBA.x,
//					sparkle.RenderData.RGBA.y,
//					sparkle.RenderData.RGBA.z
//				};
//				light.Intensity = sparkle.RenderData.RGBA.w * sparkle.Scale.x * 15.0f;
//
//				auto& lightSphere{ List_Matrix_World_LightSphere_.New() };
//
//				float const radius{ 120.0f };
//				lightSphere = {
//					radius, 0.0f, 0.0f, 0.0f,
//					0.0f, radius, 0.0f, 0.0f,
//					0.0f, 0.0f, radius, 0.0f,
//					light.WorldPosition.x,
//					light.WorldPosition.y,
//					light.WorldPosition.z,
//					1.0f,
//				};
//			}
//		}
//
//		{
//			static float boxEffectTimeFactor{ 0.0f };
//			boxEffectTimeFactor += 0.75f;
//
//			static constexpr Lumina::Mat4 offsets{
//				-1.0f, -1.0f, 0.0f, 1.0f,
//				-1.0f, 1.0f, 0.0f, 1.0f,
//				1.0f, -1.0f, 0.0f, 1.0f,
//				1.0f, 1.0f, 0.0f, 1.0f,
//			};
//
//			for (auto const& m : Arr_MeshLocalToWorld_) {
//				auto const transformedOffsets{ offsets * m };
//				auto const& anim{ Arr_FractalAnimation_[&m - Arr_MeshLocalToWorld_.data()] };
//				auto const scale{ anim.Scale() };
//				auto const rgb_Base = Lumina::Utils::Color::Convert(
//					Lumina::Utils::Color::HSV{
//						rndEngine() * Inv_0xFFFFFFFF * 90.0f - 45.0f * std::cos(boxEffectTimeFactor) - 45.0f,
//						rndEngine() * Inv_0xFFFFFFFF * 0.5f + 0.5f,
//						0.45f
//					}
//				);
//				auto const rgb_Gaming = Lumina::Utils::Color::Convert(
//					Lumina::Utils::Color::HSV{
//						rndEngine() * Inv_0xFFFFFFFF * 45.0f +
//						boxEffectTimeFactor +
//						std::atan2(m[3][1] - BackgroundCenter_.y, m[3][0] - BackgroundCenter_.x) *
//						std::numbers::inv_pi_v<float> * 360.0f,
//						rndEngine() * Inv_0xFFFFFFFF * 0.3f + 0.5f,
//						0.95f
//					}
//				);
//				Lumina::Utils::Color::RGB const rgb{
//					(0.05f + rgb_Base.R * 0.95f) * (0.2f + rgb_Gaming.R * 0.8f),
//					(0.05f + rgb_Base.G * 0.95f) * (0.2f + rgb_Gaming.G * 0.8f),
//					rgb_Base.B * (0.1f + rgb_Gaming.B * 0.9f),
//				};
//				for (int i = 0; i < 4; ++i) {
//					if (!List_PointLight_.IsFull()) {
//						auto& light{ List_PointLight_.New() };
//
//						light.WorldPosition = {
//							transformedOffsets[i][0],
//							transformedOffsets[i][1],
//							0.0f,
//							1.0f
//						};
//						light.RGB = { rgb.R, rgb.G, rgb.B };
//						light.Intensity = scale.x * 0.015625f;
//
//						auto& lightSphere{ List_Matrix_World_LightSphere_.New() };
//						float const radius{ scale.x * 0.75f };
//						lightSphere = {
//							radius, 0.0f, 0.0f, 0.0f,
//							0.0f, radius, 0.0f, 0.0f,
//							0.0f, 0.0f, radius, 0.0f,
//							light.WorldPosition.x,
//							light.WorldPosition.y,
//							light.WorldPosition.z,
//							1.0f,
//						};
//					}
//				}
//			}
//		}
//
//		std::vector<uint32_t> lightIndices{};
//		Lumina::List<Lumina::PointLight>::Iterator it_Light{ List_PointLight_ };
//		for (it_Light.Begin(); !it_Light.End(); it_Light.Next()) {
//			lightIndices.emplace_back(it_Light.Index());
//		}
//		DeferredLighting_->Update(List_PointLight_, List_Matrix_World_LightSphere_, lightIndices);
//
//		{
//			static float bloomFactor = 0.0f;
//			bloomFactor += 0.01f;
//			ClearPostProcessingConstants_.BloomRadius = 0.5f + 0.125f * std::sin(bloomFactor * 2.0f);
//			ClearPostProcessingConstants_.BloomIntensity = 0.875f + 0.125f * std::sin(bloomFactor * 1.75f);
//			ClearPostProcessingConstants_.BloomAttenuation = 0.25f - 0.125f * std::sin(bloomFactor * 1.5f);
//		}
//		UB_PostProcessingConstants_.Store(
//			&ClearPostProcessingConstants_,
//			sizeof(ClearPostProcessingConstants),
//			0LLU
//		);
//
//		{
//			UIFadeIn_ += 0.05f;
//			float const factor = std::sin(UIFadeIn_);
//
//			auto const rgb_Base = Lumina::Utils::Color::Convert(
//				Lumina::Utils::Color::HSV{
//					rndEngine() * Inv_0xFFFFFFFF * 3.0f + 7.5f + factor * 15.0f,
//					rndEngine() * Inv_0xFFFFFFFF * 0.1f + 0.1f,
//					0.75f
//				}
//			);
//
//			UI_Label_StageClear_.Scale_.x = 720.0f * (1.0f + factor * 0.025f) * 1.25f;
//			UI_Label_StageClear_.Scale_.y = 180.0f * (1.0f + factor * 0.025f) * 1.25f;
//			UI_Label_StageClear_.RGBA_.x = rgb_Base.R;
//			UI_Label_StageClear_.RGBA_.y = rgb_Base.G;
//			UI_Label_StageClear_.RGBA_.z = rgb_Base.B;
//
//			UI_Label_PressSpaceKey_.Scale_.x = 720.0f * (1.0f + factor * 0.0125f);
//			UI_Label_PressSpaceKey_.Scale_.y = 180.0f * (1.0f + factor * 0.0125f);
//			UI_Label_PressSpaceKey_.RGBA_.x = rgb_Base.R;
//			UI_Label_PressSpaceKey_.RGBA_.y = rgb_Base.G;
//			UI_Label_PressSpaceKey_.RGBA_.z = rgb_Base.B;
//		}
//
//		if (Count_FadeIn_ >= 0) {
//			{
//				float const t{ static_cast<float>(72 - Count_FadeIn_) / 72.0f };
//				float const factor = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
//				UI_Label_StageClear_.Translate_.y = 390.0f - 60.0f * factor;
//				UI_Label_StageClear_.Scale_.x *= (0.9f + 0.1f * factor);
//				UI_Label_StageClear_.Scale_.y *= (0.9f + 0.1f * factor);
//				UI_Label_StageClear_.RGBA_.w = 0.5f * factor * factor;
//			}
//			{
//				float const t{ static_cast<float>(std::max<int>(60 - Count_FadeIn_, 0)) / 60.0f };
//				float const factor = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
//				UI_Label_PressSpaceKey_.Translate_.y = 510.0f - 60.0f * factor;
//				UI_Label_PressSpaceKey_.Scale_.x *= (0.8f + 0.2f * factor);
//				UI_Label_PressSpaceKey_.Scale_.y *= (0.8f + 0.2f * factor);
//				UI_Label_PressSpaceKey_.RGBA_.w = 0.125f * factor * factor;
//			}
//			--Count_FadeIn_;
//		}
//
//		if (Count_FadeOut_ >= 72) {
//			Lumina::SceneManager::Instance().Deactivate<Lumina::Scene::FLAG::UPDATE>("Scene::Clear");
//			Lumina::SceneManager::Instance().Deactivate<Lumina::Scene::FLAG::RENDER>("Scene::Clear");
//
//			BackgroundCenter_ = { 640.0f, 360.0f };
//
//			IterationTime_ = 0;
//			NextIterationTime_ = 0;
//
//			KeyState_Space_ = 0U;
//
//			Count_FadeOut_ = 0;
//			ClearPostProcessingConstants_.IsFadingOut = 0U;
//			UIFadeIn_ = 0.0f;
//			Count_FadeIn_ = 72;
//		}
//		if (Count_FadeOut_ > 0) {
//			++Count_FadeOut_;
//		}
//	}
//
//	void Clear::Update() {
//		Impl_->Update();
//	}
//}