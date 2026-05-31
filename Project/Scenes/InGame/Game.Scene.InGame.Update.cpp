module Game.Scene.InGame;

import : Impl;
import : Impl.Effect;

import <cmath>;
import <numbers>;
import <algorithm>;
import <string>;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

import Lumina.Core.Math;
import Lumina.Main;
import Lumina.OS.Windows.RawInput;
import Lumina.Utils.Data;
import nlohmann.json;
import Lumina.Utils.Color;

import Game.MotionManager;
import Game.EnemyManager;
import Game.ProjectileManager;

import Game.Events;
import Game.UIMenu;
import Lumina.Scene;
import Lumina.CG3D;
import Lumina.CG3D.Animation;

import Game.BGMManager;

#if defined(_DEBUG)
namespace {
	constexpr ImU32 MakeCol32(int r, int g, int b, int a) {
		return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r);
	}
}
#endif

namespace {
	constexpr float Inv_0xFFFFFFFF{ 1.0f / static_cast<float>(0xFFFFFFFFU) };
	constexpr float BossPresentationDuration{ 2.0f };
	constexpr float BossPresentationCameraZoom{ 6.0f };
	constexpr char BossEnemyName[]{ "Boss" };

	static Lumina::Math::F32x4x4<> INV_Viewport{
		1.0f / 640.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f / 360.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
	};
}

namespace Game::Scene::Impl {
   bool InGame::HasBossEncounterInCurrentArea() const {
		return std::any_of(
			playState_.CurrentArea.enemies.cbegin(),
			playState_.CurrentArea.enemies.cend(),
			[] (auto const& enemy_) {
				return enemy_.enemyName == BossEnemyName;
			}
		);
	}

	void InGame::StartBossEncounterPresentation() {
		playState_.IsBossPresentationActive = false;
		playState_.BossPresentationTimer = 0.0f;
		playState_.BossPresentationDuration = 0.0f;

		if (!HasBossEncounterInCurrentArea()) {
			return;
		}

		auto const bossIt = std::find_if(
			playState_.Enemies.cbegin(),
			playState_.Enemies.cend(),
			[] (auto const& enemy_) {
				return enemy_.BaseData.name == BossEnemyName;
			}
		);

		if (bossIt == playState_.Enemies.cend()) {
			return;
		}

		playState_.IsBossPresentationActive = true;
		playState_.BossPresentationTimer = BossPresentationDuration;
		playState_.BossPresentationDuration = BossPresentationDuration;
		playState_.BossPresentationFocusPosition = bossIt->Position;
		playState_.TransitionCooldownTimer = (std::max)(playState_.TransitionCooldownTimer, BossPresentationDuration);
		Event::CameraShakingTimer = (std::max)(Event::CameraShakingTimer, 20);
	}

	void InGame::CheckAndLoadArea(int areaIndex, int previousAreaIndex) {
		std::string filename = "area" + std::to_string(areaIndex) + ".json";
		areaEditor_.LoadArea(playState_.CurrentArea, filename);
		playState_.VisitedAreas.insert(areaIndex);
		
		try {
			// Load Terrain (which reads "Polygons" and "GroundPoints" stored inside area json)
			TerrainScreenData_ = std::make_unique<TerrainShapeCollection>();
			TerrainScreenData_->Initialize(
				Lumina::Utils::LoadFromFile<nlohmann::json>(filename, "Assets/Data/Terrain/")
			);
			
			Terrain_ = std::make_unique<TerrainShapeCollection>();
			TerrainScreenData_->ConvertToWorldCoordinate(
				*Terrain_,
				*Camera_,
				{ 0.0f, 0.0f, 1280.0f * 0.25f, 720.0f * 0.25f, 0.0f, 1.0f }
			);
			TerrainRenderer_->PrepareMesh(*Terrain_);
#if defined(_DEBUG)
			TerrainEditor_->SetShapes(*Terrain_);
#endif
		} catch (...) {
			// Fallback or empty terrain if file has no terrain data yet
			TerrainScreenData_ = std::make_unique<TerrainShapeCollection>();
			Terrain_ = std::make_unique<TerrainShapeCollection>();
		}
		
		// Reset player position when entering area
		playState_.Player.Position.Y = 0.0f;
		playState_.Player.Position.Z = 0.0f;
		playState_.Player.Velocity = {0.f, 0.f, 0.f};

		float playerScreenX = 100.0f;
		float playerScreenY = 0.0f;

		// Spawn location logic
		bool spawnedAtConnection = false;
		if (previousAreaIndex != -1) {
			for (const auto& conn : playState_.CurrentArea.connections) {
				if (conn.targetAreaIndex == previousAreaIndex && !(areaIndex == 0 && previousAreaIndex == 0)) {
					// Spawn at the center of the connection linking back to where we came from
					playerScreenX = conn.position.x;
					playerScreenY = conn.position.y;
					spawnedAtConnection = true;
					break;
				}
			}
		}

		if (!spawnedAtConnection && areaIndex == 0) {
			for (const auto& conn : playState_.CurrentArea.connections) {
				if (conn.targetAreaIndex == 0) {
					playerScreenX = conn.position.x;
					playerScreenY = conn.position.y;
					spawnedAtConnection = true;
					break;
				}
			}
		}

		if (TutorialManager_) {
			// エリア遷移時に現在アクティブなチュートリアルを中断
			TutorialManager_->Skip();

			// エリア番号に対応するイベントを発火
			TutorialManager_->FireEvent("area_enter_" + std::to_string(areaIndex));
		}

		if (!spawnedAtConnection) {
			playerScreenX = 100.0f; // Fallback / Start location
		}

		// Convert Player, Enemies, and Connections to World Coordinates
		if (Camera_) {
			auto const worldToHomogeneous_c = Camera_->View() * Camera_->Projection();
			auto tmp{ Lumina::Math::F32x4{ 0.0f, 0.0f, 0.0f, 1.0f } * worldToHomogeneous_c };
			tmp /= tmp.W();

			Lumina::F32 const inv_ViewportWidth{ 1.0f / (1280.0f * 0.25f)};
			Lumina::F32 const inv_ViewportHeight{ 1.0f / (720.0f * 0.25f)};
			
			auto const& inv_View{ Camera_->ViewInverse() };
			auto const inv_Proj{ Camera_->Projection().Inverse() };
			auto const ndcToWorld{ inv_Proj * inv_View };

			auto ScreenToWorld = [&](float sx, float sy) {
				Lumina::Math::F32x4 ndcPos{
					(sx * inv_ViewportWidth) * 2.0f - 1.0f,
					1.0f - (sy * inv_ViewportHeight) * 2.0f,
					tmp.Z(),
					1.0f
				};
				auto worldPos = ndcPos * ndcToWorld;
				worldPos /= worldPos.W();
				return std::make_pair(worldPos.X(), worldPos.Y());
			};

			// Apply Player Coordinates
			auto pPos = ScreenToWorld(playerScreenX, playState_.CurrentArea.height - playerScreenY);
			playState_.Player.Position.X = pPos.first;
			playState_.Player.Position.Y = pPos.second;
			playState_.Player.Position.Z = 0.0f;

			// Apply Enemies Coordinates
			for (auto& ep : playState_.CurrentArea.enemies) {
				auto ePos = ScreenToWorld(ep.position.x, playState_.CurrentArea.height - ep.position.y);
				ep.position.x = ePos.first;
				ep.position.y = ePos.second;
			}

			// Apply Connections Coordinates
			playState_.PortalColliders.clear();
			for (auto& conn : playState_.CurrentArea.connections) {
				auto connWPos = ScreenToWorld(conn.position.x, playState_.CurrentArea.height - conn.position.y);
				conn.position.x = connWPos.first;
				conn.position.y = connWPos.second;

				// 0→0 のスタート地点ゲートはポータルとして扱わない
				if (playState_.CurrentArea.index == 0 && conn.targetAreaIndex == 0) {
					playState_.PortalColliders.push_back(nullptr); // インデックスを合わせるためnull
					continue;
				}

				float wSizeX = 1.5f;
				float wSizeY = 1.5f;
				auto col = std::make_shared<ConvexCollider>();
				col->SetMyType(COL_Warp);
				col->SetYourType(COL_Player);
				std::vector<Lumina::Math::F32x3> verts = {
					{ connWPos.first - wSizeX, connWPos.second - wSizeY, -1.5f },
					{ connWPos.first + wSizeX, connWPos.second - wSizeY, -1.5f },
					{ connWPos.first + wSizeX, connWPos.second + wSizeY, -1.5f },
					{ connWPos.first - wSizeX, connWPos.second + wSizeY, -1.5f },
					{ connWPos.first - wSizeX, connWPos.second - wSizeY, 1.5f },
					{ connWPos.first + wSizeX, connWPos.second - wSizeY, 1.5f },
					{ connWPos.first + wSizeX, connWPos.second + wSizeY, 1.5f },
					{ connWPos.first - wSizeX, connWPos.second + wSizeY, 1.5f }
				};
				col->SetVertices(verts);
				col->UpdateAABB();
				playState_.PortalColliders.push_back(col);
			}
		} else {
			playState_.Player.Position.X = playerScreenX;
			playState_.Player.Position.Y = playerScreenY;
		}

		playState_.Enemies.clear();
		Game::EnemyManager::GetInstance()->ClearInstances();
		Game::ProjectileManager::GetInstance()->ClearAll();
		Game::ExpOrbManager::GetInstance()->Clear();
		EnemySkinnedInstances_.clear();

		int placementIndex = 0;
		for (auto& ep : playState_.CurrentArea.enemies) {
			// すでに倒されている敵ならスポーンしない
			if (playState_.DefeatedEnemies.contains({ playState_.CurrentArea.index, placementIndex })) {
				placementIndex++;
				continue;
			}

			PlayEnemy pe;
			enemyEditor_.LoadEnemy(pe.BaseData, ep.enemyName + ".json");
			pe.Position.X = ep.position.x;
			pe.Position.Y = ep.position.y; 
			pe.Position.Z = 0.0f;

			// サイズ段階に応じたステータスを適用
			int tierIdx = (std::max)(0, (std::min)(2, ep.sizeCategory));
			pe.BaseData.hp    = pe.BaseData.sizeTiers[tierIdx].hp;
			pe.BaseData.power = pe.BaseData.sizeTiers[tierIdx].power;
			pe.SizeTier       = tierIdx;
			pe.Scale          = pe.BaseData.sizeTiers[tierIdx].scale;

			pe.CurrentHP = pe.BaseData.hp;
			pe.IsDead = false;
			pe.FacingRight = ep.facingRight;
			pe.RenderFacingYaw = pe.FacingRight ? 0.0f : 3.14159265f;
			pe.PlacementIndex = placementIndex;
			playState_.Enemies.push_back(pe);

			// EnemyManager側にも生成
			auto* inst = Game::EnemyManager::GetInstance()->SpawnFromData(pe.BaseData, pe.Position, pe.FacingRight, pe.Scale, pe.SizeTier);
			if (inst) {
				inst->placementIndex = placementIndex;
			}
			placementIndex++;
		}

		if (Player_) {
			Player_->SetPosition({ playState_.Player.Position.X, playState_.Player.Position.Y, 0.0f });
			Event::RespawnPos = Player_->GetPosition();
			Player_->myVelocity_ = { 0.0f, 0.0f, 0.0f };
			Player_->externalVelocity_ = { 0.0f, 0.0f, 0.0f };
		}
		
		playState_.TransitionCooldownTimer = 0.5f; // Add delay
		if (previousAreaIndex != -1) {
			StartBossEncounterPresentation();
		}
		else {
			playState_.IsBossPresentationActive = false;
			playState_.BossPresentationTimer = 0.0f;
			playState_.BossPresentationDuration = 0.0f;
		}
		
		if (CollisionManager_) {
			CollisionManager_->Begin();
		}

		playState_.IsGoalReached = false;

		// ゲームフェーズをリセット
		Event::CurrentPhase = Event::GamePhase::InBattle;
		Event::PhaseTimer = 0.0f;
		Event::ElapsedBattleTime = 0.0f;
		Event::EnemiesDefeated = 0;
		Event::FallDeathCount = 0;
	}

#if defined(_DEBUG)
	void InGame::DrawPlayMode() {
		ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_FirstUseEver);
		ImGui::Begin("Player Info");
		if (Player_) {
			auto const& pos = Player_->GetPosition();
			ImGui::Text("Player 3D Position: %.2f, %.2f, %.2f", pos.X, pos.Y, pos.Z);
		}

		ImGui::Separator();
		ImGui::Text("Enemy Node States:");
		auto aliveEnemies = Game::EnemyManager::GetInstance()->GetAliveInstances();
		if (aliveEnemies.empty()) {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No enemies present.");
		} else {
			for (auto* enemy : aliveEnemies) {
				ImGui::Text("[%d] %s: State='%s', Timer=%.2f", 
					enemy->id, enemy->baseData.name.c_str(), 
					enemy->currentAction.c_str(), enemy->stateTimer);
			}
		}

		ImGui::End();
	}
#endif

	template<>
	void InGame::Update_<"Player">() {
		// チュートリアル入力制限の適用
		if (TutorialManager_ && TutorialManager_->IsActive()) {
			Player_->InputMask = TutorialManager_->GetAllowedInputs();
		} else {
			Player_->InputMask = 0xFFFF; // 全入力許可
		}

		Player_->Update(1.0f / 60.0f);

		// 経験値オーブの更新
		Game::ExpOrbManager::GetInstance()->Update(1.0f / 60.0f, Player_->GetPosition(), Player_.get());

		playState_.Player.Position.X = Player_->GetPosition().X;
		playState_.Player.Position.Y = Player_->GetPosition().Y;
		playState_.Player.Position.Z = Player_->GetPosition().Z;
	}

	template<>
	void InGame::Update_<"Enemies-1">(Lumina::F32&& deltaTime_) {
		Game::EnemyManager::GetInstance()->Update(
			deltaTime_,
			Player_->GetPosition()
		);
	}

	template<>
	void InGame::Update_<"Collision">() {
		// プロジェクタイル更新
		Game::ProjectileManager::GetInstance()->Update(1.0f / 60.0f, Player_->GetPosition());

		// 中身をclear
		CollisionManager_->Begin();

		// ここからColliderを設定
		Game::EnemyManager::GetInstance()->RegisterCollidersTo(*CollisionManager_);
		Game::ProjectileManager::GetInstance()->RegisterCollidersTo(*CollisionManager_);
		CollisionManager_->SetColliders(Player_->GetCollider());
		CollisionManager_->SetColliders(Player_->GetUmbrella().top_->GetCollider());
		CollisionManager_->SetColliders(Player_->GetSmashCollider());
		for (auto const& polygon : Terrain_->PolygonsData()) {
			CollisionManager_->SetColliders(polygon.Col.get());
		}
		auto const& groundColliders = Terrain_->GroundData().Colliders;
		for (auto const& col : groundColliders) {
			CollisionManager_->SetColliders(col.get());
		}
		
		for (auto const& pCol : playState_.PortalColliders) {
			if (pCol) {
				CollisionManager_->SetColliders(pCol.get());
			}
		}

		// Check!
		CollisionManager_->CheckAllCollisions();
	}

	template<>
	void InGame::Update_<"Enemies-2">() {

		const auto& enemyInstances = Game::EnemyManager::GetInstance()->GetAllInstances();
		
		// 死亡した敵を記録する
		for (const auto& inst : enemyInstances) {
			if (inst.isDead && inst.placementIndex != -1) {
				playState_.DefeatedEnemies.insert({ playState_.CurrentArea.index, inst.placementIndex });
			}
		}

		playState_.Enemies.clear();
		playState_.Enemies.reserve(enemyInstances.size());
		for (const auto& inst : enemyInstances) {
			PlayEnemy pe;
			pe.BaseData = inst.baseData;
			pe.Position = inst.position;
			pe.CurrentHP = inst.isDead ? 0 : inst.currentHP;
			pe.IsDead = inst.isDead;
			pe.HurtTimer = inst.hurtTimer;
			pe.FacingRight = inst.facingRight;
			pe.RenderFacingYaw = inst.renderFacingYaw;
			pe.RenderPitch = inst.renderPitch;
			pe.SizeTier = inst.sizeTier;
			pe.Scale = inst.modelScale;
			pe.Id = inst.id;
			pe.CurrentAction = inst.currentAction;
			pe.PlacementIndex = inst.placementIndex;
			// pull debug flag from behavior if available
			if (inst.behavior) {
				pe.WalkActive = inst.behavior->IsWalkActive();
				pe.MotionPlaying = inst.behavior->IsMotionPlaying();
				pe.ActiveNodeIndex = inst.behavior->GetActiveNodeIndex();
			} else {
				pe.WalkActive = false;
			}
			playState_.Enemies.push_back(std::move(pe));
		}
		
		// Clean up dead skinned instances
		std::erase_if(EnemySkinnedInstances_, [&enemyInstances](const auto& pair) {
			return std::find_if(enemyInstances.begin(), enemyInstances.end(), 
				[id = pair.first](const auto& inst) { return inst.id == id; }) == enemyInstances.end();
		});

		for (const auto& e : playState_.Enemies) {
			if (EnemySkinnedModels_.contains(e.BaseData.name)) {
				if (!EnemySkinnedInstances_.contains(e.Id)) {
					auto newInst = std::make_shared<SkinnedInstance>();
					newInst->Skeleton_ = Lumina::CG3D::CreateSkeleton(EnemySkinnedModels_[e.BaseData.name]->Collection_.Root);
					Lumina::CG3D::CreateSkinCluster(
						newInst->SkinCluster_,
						Lumina::Context::Instance().D3D12Context().Device(),
						Lumina::Context::Instance().D3D12Context().GlobalDescriptorHeap(),
						newInst->Skeleton_,
						EnemySkinnedModels_[e.BaseData.name]->Collection_.Meshes[0]
					);
					
					newInst->TransformsBuffer_.Initialize(Lumina::Context::Instance().D3D12Context().Device(), 256LLU);
					newInst->CBV_SceneTable_ = Lumina::Context::Instance().D3D12Context().GlobalDescriptorHeap().Allocate(1U);
					Lumina::D3D12::CBV::Create(Lumina::Context::Instance().D3D12Context().Device(), newInst->CBV_SceneTable_.CPUHandle(0U), newInst->TransformsBuffer_);
					
					EnemySkinnedInstances_[e.Id] = newInst;
				}
				
				auto& skinInst = EnemySkinnedInstances_[e.Id];
				auto& model = EnemySkinnedModels_[e.BaseData.name];
				
				int targetAnimIdx = 0;
				// Resolve the current action through animationMap if available,
				// so node-based enemies (e.g. Boss) map state names like "SwordSlashP1" -> "Attack"
				std::string resolvedAnim = e.CurrentAction;
				{
					auto it = e.BaseData.animationMap.find(e.CurrentAction);
					if (it != e.BaseData.animationMap.end() && !it->second.empty()) {
						resolvedAnim = it->second;
					}
				}
				if (resolvedAnim == "Walk" || resolvedAnim == "Chase") targetAnimIdx = 1;
				else if (resolvedAnim == "Attack" || resolvedAnim == "AttackCharge") targetAnimIdx = 2;
				else if (resolvedAnim == "Enrage") targetAnimIdx = (model->Animations_.size() > 3) ? 3 : 2;
				else if (resolvedAnim == "Death") targetAnimIdx = (model->Animations_.size() > 4) ? 4 : 0;
				
				if (targetAnimIdx >= model->Animations_.size()) targetAnimIdx = 0; // Fallback
				
				if (skinInst->currentAnimIndex_ != targetAnimIdx) {
					skinInst->currentAnimIndex_ = targetAnimIdx;
					skinInst->animTimer_ = 0.0f;
				}
				
				skinInst->animTimer_ += 1.0f / 60.0f * 1.5f;
				
				if (!model->Animations_.empty()) {
					auto& activeAnim = model->Animations_[skinInst->currentAnimIndex_];
					skinInst->animTimer_ = std::fmod(skinInst->animTimer_, activeAnim.DurationInSeconds);
					Lumina::CG3D::Update(skinInst->SkinCluster_, skinInst->Skeleton_, activeAnim, skinInst->animTimer_);
					
					// ルートモーションを抽出してEnemyManagerへフィードバック
					if (!skinInst->Skeleton_.ARR_Joint.empty()) {
						uint32_t rootID = skinInst->Skeleton_.ID_Root;
						auto animatedMat = skinInst->Skeleton_.ARR_Joint[rootID].SkeletonSpace;
						auto bindMat = skinInst->SkinCluster_.ARR_INV_BindPose[rootID].Inverse();
						
						// モデルスペースでの絶対的な並進の差分を計算
						Lumina::Math::F32x3 rootOffset = {
							animatedMat[3].Get(0) - bindMat[3].Get(0),
							animatedMat[3].Get(1) - bindMat[3].Get(1),
							animatedMat[3].Get(2) - bindMat[3].Get(2)
						};
						
						// スケールを適用
						rootOffset.X *= e.Scale;
						rootOffset.Y *= e.Scale;
						rootOffset.Z *= e.Scale;
						
						// EnemyManagerの該当インスタンスにオフセットを書き込む
						if (auto* instPtr = Game::EnemyManager::GetInstance()->GetInstance(e.Id)) {
							instPtr->rootMotionOffset = rootOffset;
						}
					}
				}
			}
		}
	}

	template<>
	void InGame::Update_<"[Debug] TerrainEditor">() {
		#if defined(_DEBUG)
		//TerrainEditor_->Update();
		#endif
	}

	template<>
	void InGame::Update_<"[Debug] Area">() {
		// エリアの移動処理
#if defined(_DEBUG)
		bool shouldProcessAreaTransition = (activeEditor_ == EditorTab::Play && playState_.IsPlaying);
#else
		bool shouldProcessAreaTransition = playState_.IsPlaying;
#endif
		if (shouldProcessAreaTransition) {
			if (playState_.TransitionCooldownTimer > 0.0f) {
				playState_.TransitionCooldownTimer -= 1.0f / 60.0f;
			}
			else {
				auto const& pos = playState_.Player.Position;
				auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
				auto const& keyboard{ inputMngr.Keyboard() };
				using Lumina::OS::Windows::KEY;

				for (const auto& conn : playState_.CurrentArea.connections) {
					// 0→0 のスタート地点ゲートはワープ判定しない
					if (playState_.CurrentArea.index == 0 && conn.targetAreaIndex == 0) continue;

					if (std::abs(pos.X - conn.position.x) <= 2.5f &&
						std::abs(pos.Y - conn.position.y) <= 3.0f) {

						// 初回ポータル接触チュートリアル
						if (!playState_.FirstPortalTouched && TutorialManager_) {
							if (TutorialManager_->FireEvent("first_portal_touch")) {
								playState_.FirstPortalTouched = true;
							}
						}

						if (keyboard.IsJustPressed(KEY::W) || inputMngr.Pad().IsHold(0x0001)) {
							int prevAreaIndex = playState_.CurrentArea.index;
							CheckAndLoadArea(conn.targetAreaIndex, prevAreaIndex);
							// エリアに応じたBGM切り替え（BGMが変わるときだけ更新される）
							if (conn.targetAreaIndex != 10) {
								Game::BGMManager::GetInstance()->PlaySceneBGM("InGame");
							} else {
								Game::BGMManager::GetInstance()->StopCurrentBGM();
							}
							break;
						}
					}
				}
			}
		}
	}

	namespace {
		bool IsUsingDebugCamera{ false };
	}

	template<>
	auto InGame::Update_<"Camera.Debug">() -> void {
		#if defined(_DEBUG)
		ImGui::Begin("Camera");
		{
			static Lumina::Math::F32x3 eye{ 0.0f, 5.0f, -30.0f };
			static Lumina::Math::F32x3 target{ 0.0f, 5.0f, 0.0f };
			ImGui::Checkbox("Use Debug Camera", &IsUsingDebugCamera);

			auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
			[[maybe_unused]] auto const& mouse{ inputMngr.Mouse() };

			ImGui::DragFloat3("Eye", &eye.X, 0.1f);
			ImGui::DragFloat3("Target", &target.X, 0.1f);
			Camera_->LookAt(eye, target, { 0.0f, 1.0f, 0.0f });
		}
		ImGui::End();
		#endif
	}

	template<>
	auto InGame::Update_<"Camera.NonDebug.Shake">(
		Lumina::Math::F32x3& newCameraPos_
	) -> void {
		auto angleInDeg = Lumina::Math::Random::Generator()() % 3;
		angleInDeg += (Lumina::Math::Random::Generator()() & 1) * 180;

		float const angleInRad = Lumina::Math::DegToRad(static_cast<float>(angleInDeg));
		Lumina::Math::F32x2 const dir{
			Lumina::Math::COS(angleInRad),
			Lumina::Math::SIN(angleInRad)
		};
		float const mag = std::exp(static_cast<float>(Event::CameraShakingTimer) / 15.0f) * 0.1f;
		newCameraPos_ += { dir.X* mag, dir.Y* mag, 0.0f };

		--Event::CameraShakingTimer;
	}

	template<>
	auto InGame::Update_<"Camera.NonDebug">() -> void {
		if (!playState_.IsPaused) {
			Lumina::Math::F32x3 newCameraPos{};
			Lumina::Math::F32x3 cameraPos{ Camera_Player_->WorldPosition() };
			auto const& playerPos = Player_->GetPosition();

			if (playState_.IsBossPresentationActive && playState_.BossPresentationDuration > 0.0f) {
				float const progress = 1.0f - playState_.BossPresentationTimer / playState_.BossPresentationDuration;
				float const bossFocusWeight = std::sin(progress * std::numbers::pi_v<float>);
				newCameraPos = {
					playerPos.X + (playState_.BossPresentationFocusPosition.X - playerPos.X) * bossFocusWeight,
					playerPos.Y + ((playState_.BossPresentationFocusPosition.Y + 2.0f) - playerPos.Y) * bossFocusWeight,
					-30.0f + BossPresentationCameraZoom * bossFocusWeight
				};
				Event::CameraShakingTimer = std::max<int>(Event::CameraShakingTimer, 2);
			}
			else {
				Lumina::F32 const dX_PlayerDirection{
					Player_->eyesDirection_.X > 0.0f ?
					1.0f :
					-1.0f
				};
				
				Lumina::Math::F32x3 const d{ cameraPos - playerPos };
				Lumina::F32 const dZ{
					std::max<Lumina::F32>(
						(d.X * d.X + d.Y + d.Y) * (-0.25f) + (-35.0f),
						-40.0f
					)
				};

				newCameraPos = {
					cameraPos.X * 0.97f + (playerPos.X + dX_PlayerDirection) * 0.03f,
					cameraPos.Y * 0.98f + (playerPos.Y + 5.0f) * 0.02f,
					cameraPos.Z * 0.95f + (playerPos.Z + dZ) * 0.05f,
				};
			}

			if (Event::CameraShakingTimer > 0) {
				Update_<"Camera.NonDebug.Shake">(newCameraPos);
			}

			Lumina::Math::F32x3 const& cameraTarget{ Camera_Player_->TargetWorldPosition() };
			Lumina::Math::F32x3 newCameraTarget{
				cameraTarget.X * 0.8f + playerPos.X * 0.2f,
				cameraTarget.Y * 0.94f + playerPos.Y * 0.06f,
				cameraTarget.Z * 0.95f + playerPos.Z * 0.05f,
			};

			Camera_Player_->LookAt(
				newCameraPos,
				newCameraTarget,
				{ 0.0f, 1.0f, 0.0f }
			);
		}
	}

	template<>
	void InGame::Update_<"Camera">() {
		#if defined(_DEBUG)
		Update_<"Camera.Debug">();
		#endif
		Update_<"Camera.NonDebug">();

		#if defined(_DEBUG)
		if (!IsUsingDebugCamera) {
			*WorldToHomogeneous_ = Camera_Player_->View() * Camera_Player_->Projection();
		}
		else {
			*WorldToHomogeneous_ = Camera_->View() * Camera_->Projection();
		}
		#else
		*WorldToHomogeneous_ = Camera_Player_->View() * Camera_->Projection();
		#endif

		*ScreenToWorld_ = INV_Viewport * WorldToHomogeneous_->Inverse();
		UB_ScreenToWorld_.Store(ScreenToWorld_.get(), sizeof(Lumina::Math::F32x4x4<>), 0LLU);
	}

	template<>
	void InGame::Update_<"[Debug] Editor">() {
		#if defined(_DEBUG)
		// メインメニューバー: エディタ切り替え
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Mode")) {
				if (ImGui::MenuItem("Play Prototype", nullptr, activeEditor_ == EditorTab::Play)) {
					activeEditor_ = EditorTab::Play;
					if (!playState_.IsPlaying) {
						playState_.IsPlaying = true;
						CheckAndLoadArea(0); // Load default area 0
					}
				}
				if (ImGui::MenuItem("Motion Editor", nullptr, activeEditor_ == EditorTab::Motion)) {
					activeEditor_ = EditorTab::Motion;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Obj Motion Editor", nullptr, activeEditor_ == EditorTab::ObjMotion)) {
					activeEditor_ = EditorTab::ObjMotion;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Area Editor", nullptr, activeEditor_ == EditorTab::Area)) {
					activeEditor_ = EditorTab::Area;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Enemy Editor", nullptr, activeEditor_ == EditorTab::Enemy)) {
					activeEditor_ = EditorTab::Enemy;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Enemy Action Editor", nullptr, activeEditor_ == EditorTab::EnemyAction)) {
					activeEditor_ = EditorTab::EnemyAction;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Actor Editor", nullptr, activeEditor_ == EditorTab::Actor)) {
					activeEditor_ = EditorTab::Actor;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Terrain Editor", nullptr, activeEditor_ == EditorTab::Terrain)) {
					activeEditor_ = EditorTab::Terrain;
					playState_.IsPlaying = false;
				}
				if (ImGui::MenuItem("Audio Editor", nullptr, activeEditor_ == EditorTab::Audio)) {
					activeEditor_ = EditorTab::Audio;
					playState_.IsPlaying = false;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// アクティブなエディタを描画
		switch (activeEditor_) {
		case EditorTab::Motion:
			MotionEditor::GetInstance()->NodeImGui();
			break;
		case EditorTab::ObjMotion:
			objMotionEditor_.Update();
			break;
		case EditorTab::Area:
			areaEditor_.Update();
			break;
		case EditorTab::Enemy:
			enemyEditor_.Update();
			break;
		case EditorTab::EnemyAction:
			enemyActionEditor_.Update();
			break;
		case EditorTab::Actor:
			actorEditor_.Update();
			break;
		case EditorTab::Terrain:
			if (TerrainEditor_) TerrainEditor_->Update();
			break;
		case EditorTab::Audio:
			audioEditor_.Update();
			break;
		case EditorTab::Play:
			DrawPlayMode();
			// Enemy HP ImGui removed

			// ミニマップ（エリア構成図）描画
			// if (playState_.IsPlaying) {
			// 	areaEditor_.DrawAreaMap(playState_.CurrentArea.index);
			// }
			break;
		default:
			break;
		}
		#endif
	}

	template<>
	void InGame::Update_<"[Debug] Manual">() {
		#if defined(_DEBUG)
		ImGui::Begin("Manual");

		ImGui::Text("Enter Key or GamePad Start : Player Respawn");

		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& pad{ inputMngr.Pad() };
		auto textColor = [](bool cond_) -> ImVec4 {
			if (cond_) {
				return ImVec4{ 1.0f, 0.3f, 0.3f, 1.0f };
			}
			else {
				return ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
			}
		};

		auto const playerWeaponStance = Player_->GetWeaponStance();
		auto const weaponState = Player_->GetUmbrella().top_->GetUmbrellaForm();
		//auto const& playerInput = Player_->GetInput();
		// 納刀
		if (playerWeaponStance == WeaponStance::Sheathed) {
			ImGui::SeparatorText("Sheathed (Noutou)");
			
			ImGui::TextColored(
				textColor(pad.GetLeftStickX() != 0.0f || pad.GetLeftStickY() != 0.0f),
				"Left Stick : Move"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x8000)),
				"Y : Battou"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x2000)),
				"B"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x1000)),
				"A : Jump"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x4000)),
				"X : Repair Umbrella"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0100)),
				"L Button : Use Mana"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0200)),
				"R Button"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0040)),
				"L2 Button"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0080)),
				"R2 Button"
			);
		}
		// 抜刀
		else {
			ImGui::SeparatorText("Drawn (Battou)");
			ImGui::TextColored(
				textColor(pad.GetLeftStickX() != 0.0f || pad.GetLeftStickY() != 0.0f),
				"Left Stick : Move"
			);

			switch (weaponState) {
				case UmbrellaForm::Closed:
					ImGui::TextColored(
						textColor(pad.IsHold(0x8000)),
						"Y : Attack"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x2000)),
						"B"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x1000)),
						"A : Jump"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x4000)),
						"X : Noutou"
					);
					break;
				case UmbrellaForm::Opened:
					ImGui::TextColored(
						textColor(pad.IsHold(0x8000)),
						"Y"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x2000)),
						"B : Reverse Umbrella"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x1000)),
						"A : Jump"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x4000)),
						"X : Close Umbrella"
					);
					break;
				case UmbrellaForm::Reverse:
					ImGui::TextColored(
						textColor(pad.IsHold(0x8000)),
						"Y (Nagaoshi) : Charge Attack"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x2000)),
						"B"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x1000)),
						"A : Jump"
					);
					ImGui::TextColored(
						textColor(pad.IsHold(0x4000)),
						"X : Reverse Umbrella Again"
					);
					break;
			}
			ImGui::TextColored(
				textColor(pad.IsHold(0x0100)),
				"L Button (Nagaoshi) : Use Mana"
			);
			ImGui::TextColored(
				textColor(pad.IsHold(0x0200)),
				"R Button"
			);
			if (weaponState == UmbrellaForm::Flying || weaponState == UmbrellaForm::AirStop) {
				ImGui::TextColored(
					textColor(pad.IsHold(0x0040)),
					"L2 Button : Recall Umbrella Top"
				);
			}
			else {
				ImGui::TextColored(
					textColor(pad.IsHold(0x0040)),
					"L2 Button : Aim"
				);
			}

			if (weaponState == UmbrellaForm::Closed) {
				ImGui::TextColored(
					textColor(pad.IsHold(0x0080)),
					"R2 Button: Open Umbrella"
				);
				
			}
			else if (weaponState == UmbrellaForm::Opened) {
				if (pad.IsHold(0x0040)) {
					ImGui::TextColored(
						textColor(pad.IsHold(0x0080)),
						"R2 Button: Shoot Umbrella Top"
					);
				}
				else {
					ImGui::TextColored(
						textColor(pad.IsHold(0x0080)),
						"R2 Button: Guard"
					);
				}
			}
		}
		ImGui::End();
		#endif
	}

	// ==============================
	//  ⑥ ポーズ・リスタート
	// ==============================
#if defined(_DEBUG)
	void InGame::DrawPauseMenu() {
		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		auto const& pad{ inputMngr.Pad() };
		using Lumina::OS::Windows::KEY;

		// ESC or Start ボタンでポーズ切り替え
		bool padStartNow = pad.IsHold(0x0010);
		bool padStartJust = padStartNow && !Event::PrevPadStart;
		Event::PrevPadStart = padStartNow;

		if (keyboard.IsJustPressed(KEY::ESC) || padStartJust) {
			Event::IsPaused = !Event::IsPaused;
		}

		if (!Event::IsPaused) return;

		// ポーズ画面オーバーレイ
		ImVec2 windowSize(400, 260);
		ImVec2 screenCenter(640.0f - windowSize.x * 0.5f, 360.0f - windowSize.y * 0.5f);
		ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Always);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::Begin("##PauseMenu", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

		ImGui::Spacing();
		ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("PAUSED").x) * 0.5f);
		ImGui::TextColored(ImVec4{ 1.0f, 0.85f, 0.2f, 1.0f }, "PAUSED");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		float btnWidth = 200.0f;
		float btnX = (windowSize.x - btnWidth) * 0.5f;

		ImGui::SetCursorPosX(btnX);
		if (ImGui::Button("Resume", ImVec2(btnWidth, 36))) {
			Event::IsPaused = false;
		}

		ImGui::Spacing();
		ImGui::SetCursorPosX(btnX);
		if (ImGui::Button("Restart Area", ImVec2(btnWidth, 36))) {
			Event::IsPaused = false;
			Event::ResetPhase();
			CheckAndLoadArea(playState_.CurrentArea.index);
		}

		ImGui::Spacing();
		ImGui::SetCursorPosX(btnX);
		if (ImGui::Button("Return to Title", ImVec2(btnWidth, 36))) {
			Event::IsPaused = false;
			Event::ResetPhase();
			playState_.IsPlaying = false;
			auto& sceneMngr{ Lumina::SceneManager::Instance() };
			sceneMngr.Deactivate("InGame");
			sceneMngr.Load<"Title">();
			sceneMngr.Activate("Title");
		}

		ImGui::End();
	}
#endif

	// ==============================
	//  ⑦ 落下時の対処
	// ==============================
	void InGame::HandleFallDeath() {
		if (!Player_ || !playState_.IsPlaying) return;
		auto const& pos = Player_->GetPosition();

		if (pos.Y < Event::FallDeathThresholdY) {
			++Event::FallDeathCount;

			// HPを少し減らす (落下ペナルティ)
			Player_->GetStatusComponent().TakeDamage(10.0f);
			playState_.PrevPlayerHp = Player_->GetStatusComponent().GetHp();

			// リスポーン地点へ戻す
			Player_->SetPosition(Event::RespawnPos);
			Player_->externalVelocity_ = { 0.0f, 0.0f, 0.0f };
			Player_->myVelocity_ = { 0.0f, 0.0f, 0.0f };

			// カメラシェイク演出
			Event::CameraShakingTimer = 10;
		}
	}

	// ==============================
	//  ⑤ ゲームフェーズ管理UI
	// ==============================
	void InGame::DrawGamePhaseUI() {
		float dt = 1.0f / 60.0f;

		switch (Event::CurrentPhase) {
		case Event::GamePhase::Startup:
		{
			// カウントダウン演出を廃止し、即座にInBattleへ移行
			Event::CurrentPhase = Event::GamePhase::InBattle;
			Event::PhaseTimer = 0.0f;
			break;
		}
		case Event::GamePhase::InBattle:
		{
			Event::ElapsedBattleTime += dt;

			// 全敵撃破で勝利
			int aliveCount = 0;
			for (const auto& e : playState_.Enemies) {
				if (!e.IsDead) ++aliveCount;
			}
			if (!playState_.Enemies.empty() && aliveCount == 0) {
				Event::CurrentPhase = Event::GamePhase::Win;
				Event::PhaseTimer = 0.0f;
			}

			// プレイヤー死亡で敗北
			if (Player_ && Player_->GetStatusComponent().IsDead()) {
				Event::CurrentPhase = Event::GamePhase::Lose;
				Event::PhaseTimer = 0.0f;
			}
			break;
		}
		case Event::GamePhase::Win:
		{
			Event::PhaseTimer += dt;
			// Area Clear ImGui removed
			break;
		}
		case Event::GamePhase::Lose:
		{
			Event::PhaseTimer += dt;

			// 初回のみメニューを表示
			if (!GameOverMenu_.IsVisible()) {
				GameOverMenu_.Setup(
					{
						// Retry (緑系)
						{ { 0.15f, 0.4f, 0.15f, 0.8f }, { 0.2f, 0.8f, 0.3f, 1.0f } },
						// Return to Title (青系)
						{ { 0.15f, 0.15f, 0.4f, 0.8f }, { 0.3f, 0.3f, 0.9f, 1.0f } }
					},
					{ 0.9f, 0.15f, 0.15f, 0.9f } // タイトル矩形: 赤
				);
				GameOverMenu_.Show();
			}

			int decided = GameOverMenu_.Update(dt);
			if (decided == 0) {
				// Retry (Fade Out)
				GameOverMenu_.Hide();
				playState_.ScreenFadeState = 1;
				playState_.ScreenFadeNextAction = 1;
			}
			else if (decided == 1) {
				// Return to Title (Fade Out)
				GameOverMenu_.Hide();
				playState_.ScreenFadeState = 1;
				playState_.ScreenFadeNextAction = 2;
			}
			break;
		}
		}
	}

	// ==============================
	//  ⑩ Enemy HP UI (ゲーム画面上のHPバー)
	// ==============================
#if defined(_DEBUG)
	void InGame::DrawEnemyHPBars() {
		if (!playState_.IsPlaying || !Camera_Player_) return;

		auto const& viewProj = *WorldToHomogeneous_;
		float vpW = 1280.0f;
		float vpH = 720.0f;

		// オーバーレイウィンドウ
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(vpW, vpH));
		ImGui::Begin("##EnemyHPOverlay", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		for (const auto& enemy : playState_.Enemies) {
			if (enemy.IsDead) continue;

			// ワールド座標 → NDC → スクリーン座標
			Lumina::Math::F32x4 worldPos{
				enemy.Position.X,
				enemy.Position.Y + 2.0f * enemy.Scale, // 頭の上に表示
				enemy.Position.Z,
				1.0f
			};
			auto clip = worldPos * viewProj;
			if (clip.W() <= 0.0f) continue; // カメラ後方

			float ndcX = clip.X() / clip.W();
			float ndcY = clip.Y() / clip.W();

			float screenX = (ndcX * 0.5f + 0.5f) * vpW;
			float screenY = (1.0f - (ndcY * 0.5f + 0.5f)) * vpH;

			// 画面外なら描画しない
			if (screenX < -50.0f || screenX > vpW + 50.0f ||
				screenY < -50.0f || screenY > vpH + 50.0f) continue;

			float barWidth = 50.0f * enemy.Scale;
			barWidth = (std::max)(30.0f, (std::min)(80.0f, barWidth));
			float barHeight = 6.0f;
			float hpRatio = static_cast<float>(enemy.CurrentHP) /
				static_cast<float>((std::max)(1, enemy.BaseData.hp));
			hpRatio = (std::max)(0.0f, (std::min)(1.0f, hpRatio));

			float left = screenX - barWidth * 0.5f;
			float top = screenY - barHeight;

			// 背景 (暗いグレー)
			drawList->AddRectFilled(
				ImVec2(left - 1, top - 1),
				ImVec2(left + barWidth + 1, top + barHeight + 1),
				MakeCol32(20, 20, 20, 180), 2.0f);

			// HP バー
			ImU32 barColor;
			if (hpRatio > 0.5f)
				barColor = MakeCol32(50, 220, 80, 230);   // 緑
			else if (hpRatio > 0.25f)
				barColor = MakeCol32(240, 200, 40, 230);   // 黄
			else
				barColor = MakeCol32(230, 50, 50, 230);    // 赤

			drawList->AddRectFilled(
				ImVec2(left, top),
				ImVec2(left + barWidth * hpRatio, top + barHeight),
				barColor, 2.0f);
		}

		ImGui::End();
	}
#endif

	void InGame::Update() {
		float dt = 1.0f / 60.0f;

		// --- Screen Fade Logic ---
		if (playState_.ScreenFadeState == 1) { // FadeOut
			playState_.ScreenFadeAlpha += dt * playState_.ScreenFadeSpeed;
			if (playState_.ScreenFadeAlpha >= 1.0f) {
				playState_.ScreenFadeAlpha = 1.0f;
				
				// Execute the deferred action
				if (playState_.ScreenFadeNextAction == 1 || playState_.ScreenFadeNextAction == 2) {
					Event::ResetPhase();
					playState_.IsPlaying = true;
					if (Player_) {
						Player_->GetStatusComponent().Heal(Player_->GetStatusComponent().GetMaxHp());
						// 死亡ステートから復帰させる
						Player_->ChangeMovementState(Player_->idleState_.get());
						Player_->ChangeActionState(Player_->normalSheathedState_.get());
						Player_->externalVelocity_ = { 0.0f, 0.0f, 0.0f };
						Player_->myVelocity_ = { 0.0f, 0.0f, 0.0f };
						playState_.PrevPlayerHp = -1.0f;
					}
					if (TutorialManager_) {
						TutorialManager_->ResetProgress();
					}
					playState_.VisitedAreas.clear();
					playState_.DefeatedEnemies.clear();
					// チュートリアル用フラグをリセット
					playState_.FirstPortalTouched = false;
					playState_.FirstAirborneFired = false;
					playState_.ThrowTutorialFired = false;
					playState_.Area7Timer = 0.0f;
					CheckAndLoadArea(0);
					Game::BGMManager::GetInstance()->PlaySceneBGM("InGame");
				}
				
				int action = playState_.ScreenFadeNextAction;
				playState_.ScreenFadeNextAction = 0;
				playState_.ScreenFadeState = 2; // Transition to FadeIn
				playState_.ScreenFadeAlpha = 1.0f; // Ensure it starts fully black

				if (action == 2) { // Title
					auto& sceneMngr{ Lumina::SceneManager::Instance() };
					sceneMngr.Deactivate("InGame");
					sceneMngr.Load<"Title">();
					sceneMngr.Activate("Title");
					return;
				}
			}
			// Continue updating camera/lighting so the screen doesn't freeze weirdly, but skip game logic
			Update_<"Camera">();
			return; 
		} else if (playState_.ScreenFadeState == 2) { // FadeIn
			playState_.ScreenFadeAlpha -= dt * playState_.ScreenFadeSpeed;
			if (playState_.ScreenFadeAlpha <= 0.0f) {
				playState_.ScreenFadeAlpha = 0.0f;
				playState_.ScreenFadeState = 0; // Finish fade
			}
		}

/// dev-Kouda-4.1
        if (playState_.IsBossPresentationActive) {
			playState_.BossPresentationTimer -= 1.0f / 60.0f;
			if (playState_.BossPresentationTimer <= 0.0f) {
				playState_.IsBossPresentationActive = false;
				playState_.BossPresentationTimer = 0.0f;
				playState_.BossPresentationDuration = 0.0f;
			}
		}
///ここまで
		bool tutorialActive = false;
		if (TutorialManager_ && TutorialManager_->IsActive()) {
			tutorialActive = true;
		}

		auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
		auto const& keyboard{ inputMngr.Keyboard() };
		auto const& pad{ inputMngr.Pad() };
		using Lumina::OS::Windows::KEY;

		bool padStartNow = pad.IsHold(0x0010);
		bool padStartJust = padStartNow && !Event::PrevPadStart;
		Event::PrevPadStart = padStartNow;

		if (keyboard.IsJustPressed(KEY::ESC) || padStartJust || keyboard.IsJustPressed(KEY::P)) {
			playState_.IsPaused = !playState_.IsPaused;
			if (playState_.IsPaused) {
				playState_.PauseSelectedIndex = 0;
				playState_.PauseAnimationTimer = 0.0f;
				playState_.PrevPauseUpHeld = true;
				playState_.PrevPauseDownHeld = true;
				playState_.PrevPauseDecideHeld = true;
			}
		}

		// M キーまたはゲームパッドの BACK ボタンでミニマップ拡大表示トグル（ポーズ中は無効）
		bool padBackNow = pad.IsHold(0x0020); // 0x0020 = BACK ボタン
		bool padBackJust = padBackNow && !Event::PrevPadBack;
		Event::PrevPadBack = padBackNow;

		if (!playState_.IsPaused && (keyboard.IsJustPressed(KEY::M) || padBackJust)) {
			minimapExpanded_ = !minimapExpanded_;
		}

		if (playState_.IsPaused) {
			playState_.PauseAnimationTimer += 1.0f / 60.0f;
			bool upHeld = keyboard.IsPressed(KEY::W) || keyboard.IsPressed(KEY::ARROW_UP) || pad.IsHold(0x0001);
			bool downHeld = keyboard.IsPressed(KEY::S) || keyboard.IsPressed(KEY::ARROW_DOWN) || pad.IsHold(0x0002);
			
			bool upJust = upHeld && !playState_.PrevPauseUpHeld;
			bool downJust = downHeld && !playState_.PrevPauseDownHeld;
			
			playState_.PrevPauseUpHeld = upHeld;
			playState_.PrevPauseDownHeld = downHeld;

			if (upJust) {
				playState_.PauseSelectedIndex = (playState_.PauseSelectedIndex - 1 + 3) % 3;
			}
			if (downJust) {
				playState_.PauseSelectedIndex = (playState_.PauseSelectedIndex + 1) % 3;
			}

			bool decideHeld = keyboard.IsPressed(KEY::ENTER) || keyboard.IsPressed(KEY::SPACE) || pad.IsHold(0x1000);
			bool decideJust = decideHeld && !playState_.PrevPauseDecideHeld;
			playState_.PrevPauseDecideHeld = decideHeld;

			if (decideJust) {
				if (playState_.PauseSelectedIndex == 0) {
					// Resume
					playState_.IsPaused = false;
				} else if (playState_.PauseSelectedIndex == 1) {
					// Restart from the beginning (Area 0) -> Fade Out
					playState_.IsPaused = false;
					playState_.ScreenFadeState = 1;
					playState_.ScreenFadeNextAction = 1;
				} else if (playState_.PauseSelectedIndex == 2) {
					// Title -> Fade Out
					playState_.IsPaused = false;
					playState_.ScreenFadeState = 1;
					playState_.ScreenFadeNextAction = 2;
				}
			}
		}

		// ポーズ中、またはボス登場演出中はゲームロジック更新をスキップ
		if (!playState_.IsPaused && !playState_.IsBossPresentationActive) {
			float deltaTime = 1.0f / 60.0f;
			
			if (Event::HitStopTimer > 0.0f) {
				Event::HitStopTimer -= deltaTime;
				if (Event::HitStopTimer < 0.0f) {
					Event::HitStopTimer = 0.0f;
				}
				deltaTime = 0.0f; // 物理等の進行を停止
			}

			if (deltaTime > 0.0f) {
				// 前フレームで死亡したプロジェクタイルを破棄（Render終了後に安全に消去するためここで実行）
				Game::ProjectileManager::GetInstance()->RemoveDeadProjectiles();

				Update_<"Player">(); // プレイヤーはチュートリアル中も更新（内部で入力マスクあり）

				// 初回空中チュートリアル: プレイヤーが空中に入ったら発火（area0では表示しない）
				if (Player_ && TutorialManager_ && !playState_.FirstAirborneFired) {
					if (playState_.CurrentArea.index != 0 &&
						Player_->GetCurrentMovementState() == Player_->airborneState_.get()) {
						if (TutorialManager_->FireEvent("first_airborne")) {
							playState_.FirstAirborneFired = true;
						}
					}
				}

				// 傘投げチュートリアル: area7で0.5秒経過したときに無条件で発火（高優先度）
				if (Player_ && TutorialManager_ && !playState_.ThrowTutorialFired) {
					if (playState_.CurrentArea.index == 7) {
						playState_.Area7Timer += 1.0f / 60.0f;
						if (playState_.Area7Timer >= 0.5f) {
							if (TutorialManager_->FireEvent("umbrella_throw_ready")) {
								playState_.ThrowTutorialFired = true;
								// エリアロード時に傘がどこかへ飛んでしまっていた場合のみ、強制手元回収リセット
								auto const form = Player_->GetUmbrella().top_->GetUmbrellaForm();
								if (form == UmbrellaForm::Flying || form == UmbrellaForm::AirStop) {
									Player_->GetUmbrella().top_->ChangeState(new UmbrellaStates::Attached());
								}
							}
						}
					} else {
						playState_.Area7Timer = 0.0f;
					}
				}

				// 傘投げチュートリアルの進行を「傘の実体状態」で監視する
				if (Player_ && TutorialManager_ && TutorialManager_->IsActive()) {
					if (TutorialManager_->GetActiveSequenceId() == "ThrowUmbrella") {
						int const currentStep = TutorialManager_->GetCurrentStep();
						auto const umbrellaForm = Player_->GetUmbrella().top_->GetUmbrellaForm();

						if (currentStep == 0) {
							// 傘を開くチュートリアル（R2_OpenAnUmbrella.png）表示中
							// プレイヤーが傘を開いたら（Opened）、照準チュートリアル（L2_Aim.png）に進む
							if (umbrellaForm == UmbrellaForm::Opened) {
								TutorialManager_->AdvanceStep();
							}
						}
						else if (currentStep == 2) { // ※ step 1 (L2_Aim.png) はマネージャ側で自動進行
							// 射出チュートリアル（R2_Shoot.png）表示中
							// 画像が一瞬で切り替わらないよう「最低0.5秒表示」した上で、傘が実際に投げられて Flying/AirStop になったら進む
							if (TutorialManager_->GetTimer() >= 0.5f) {
								if (umbrellaForm == UmbrellaForm::Flying || umbrellaForm == UmbrellaForm::AirStop) {
									TutorialManager_->AdvanceStep();
								}
							}
						}
						else if (currentStep == 3) {
							// ワープチュートリアル（R2_Warp.png）表示中
							// プレイヤーがワープを実行し、傘が Flying/AirStop 以外の状態（＝アタッチ状態等）に戻ったら完了
							if (umbrellaForm != UmbrellaForm::Flying && umbrellaForm != UmbrellaForm::AirStop) {
								TutorialManager_->AdvanceStep();
							}
						}
					}
				}

				// プレイヤーの位置に基づいて地点イベントトリガーを判定
				if (Player_ && TutorialManager_) {
					TutorialManager_->UpdateLocationTriggers(playState_.CurrentArea.index, Player_->GetPosition());
				}
				
				Update_<"Enemies-1">(1.0f / 60.0f);
				
				Update_<"Collision">(); // 地形との当たり判定のため実行
				
				Update_<"Enemies-2">();

				// プレイヤーが敵や弾からダメージを受けた（HPが減少した）ことを検知し、チュートリアルイベントを発火
				if (Player_) {
					float currentHp = Player_->GetStatusComponent().GetHp();
					if (playState_.PrevPlayerHp < 0.0f) {
						playState_.PrevPlayerHp = currentHp;
					} else if (currentHp < playState_.PrevPlayerHp) {
						if (TutorialManager_) {
							TutorialManager_->FireEvent("first_damage_taken");
						}
						playState_.PrevPlayerHp = currentHp;
					} else {
						playState_.PrevPlayerHp = currentHp;
					}
				}
			}
			
			Update_<"[Debug] Area">();
		}

		// ツール系の更新はポーズ等に関わらず実行
		Update_<"[Debug] TerrainEditor">();

		// プレイヤー入力処理を終えた後でチュートリアルを進行させる
		// ポーズ中やボス登場演出中はチュートリアルも進めない
		if (tutorialActive && !playState_.IsPaused && !playState_.IsBossPresentationActive) {
			TutorialManager_->Update(1.0f / 60.0f, playState_.CurrentArea.index);
		}

/// dev-Takanaga-temporary
		Update_<"Camera">();
		Update_<"[Debug] Editor">();
		Update_<"[Debug] Manual">();

		// ⑦ 落下処理 & ⑤ ゲームフェーズUI（Release でも動作）
#if defined(_DEBUG)
		if (activeEditor_ == EditorTab::Play && playState_.IsPlaying) {
#else
		if (playState_.IsPlaying) {
#endif
			HandleFallDeath();
			DrawGamePhaseUI();
		}

		Update_<"Lighting">();

		Update_<"EffectVariables">();
		Update_<"Effect.Common">();

		Update_<"Effect.Player.Perpetual">();
		Update_<"Effect.Player.Move">();
		Update_<"Effect.Player.Jump">();
		Update_<"Effect.Player.Warp">();

		Update_<"Effect.Umbrella.Perpetual">();
		Update_<"Effect.Umbrella.Attack">();

		Update_<"Effect.Ambient.Raindrops">();
		Update_<"Effect.Ambient.Sparkle">();
		Update_<"Effect.Ambient.Portals">();

		constexpr float deltaTime{ 1.0f / 60.0f };
		Watercolor_->Update(deltaTime);
	}
}