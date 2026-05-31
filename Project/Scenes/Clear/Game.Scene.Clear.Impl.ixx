export module Game.Scene.Clear : Impl;

import <memory>;

import <array>;
import <vector>;

import Lumina.Core.Math;
import Lumina.D3D12;
import Lumina.D3D12.Aux;
import Lumina.D3D12.Aux.View;
import Lumina.D3D12.Context;
import Lumina.DeferredLighting;
import Lumina.Sprite;
import ParticleSystem;
import Lumina.Primitive;

namespace {
	constexpr float Delta_Time{ 1.0f / 72.0f };
}

namespace Game::Scene::Impl {
	struct ClearPostProcessingConstants {
		float BloomRadius;
		float BloomIntensity;
		float BloomAttenuation;
		uint32_t IsFadingOut;
		float BlurRadius;
		float BlurIntensity;
		float BlurAttenuation;
	};

	struct ClearFractalAnimation {
		float Easing1() const {
			return Timer * Timer * Timer;
		}

		void ResetTimer() {
			Timer = 0.0f;
		}

		Lumina::Math::F32x2 Position() const {
			float t = Easing1();
			return InitialPosition * (1.0f - t) + FinalPosition * t;
		}

		Lumina::Math::F32x2 Scale() const {
			float t = Easing1();
			return InitialScale * (1.0f - t) + FinalScale * t;
		}

		float Rotation() const {
			float t = Easing1();
			return InitialRotation * (1.0f - t) + FinalRotation * t;
		}

		void Update() {
			Timer += Delta_Time;
			if (Timer > 1.0f) { Timer = 1.0f; }
		}

		float Timer = 0.0f;

		Lumina::Math::F32x2 InitialPosition;
		Lumina::Math::F32x2 FinalPosition;
		Lumina::Math::F32x2 InitialScale;
		Lumina::Math::F32x2 FinalScale;
		Lumina::Math::F32x2 Center;
		float InitialRotation;
		float FinalRotation;

		int IterationTime;
	};

	export class Clear {
	public:
		void Update();
		void Render();

	public:
		template<typename...ArgTypes>
		void Initialize(typename ArgTypes const&...args_);

	public:
		~Clear() noexcept;

	private:
		Lumina::Math::F32x4x4<> WorldToNDC_{};
		Lumina::Math::F32x2 BackgroundCenter_{ 640.0f, 360.0f };
		Lumina::Math::F32x2 BackgroundCenter_Prev_{};

		Lumina::D3D12::CommandAllocator CmdAllocator_{};
		Lumina::D3D12::CommandList CmdList_;
		Lumina::D3D12::CommandList const* CmdList_Main_;

		int IterationTime_ = 0;
		int NextIterationTime_ = 0;

		//----	Sprite							----//
		//----	------	------	------	------	----//

		std::unique_ptr<Lumina::SpriteRenderer> SpriteRenderer_{ nullptr };
		Lumina::Sprite UI_Label_StageClear_;
		Lumina::Sprite UI_Label_PressSpaceKey_;

		Lumina::D3D12::GraphicsPSO PSO_Sprite_{};
		Lumina::D3D12::Shader VS_Sprite_{};
		Lumina::D3D12::Shader PS_Sprite_{};

		Lumina::D3D12::GraphicsPSO PSO_SpriteUI_{};
		Lumina::D3D12::Shader PS_SpriteUI_{};

		//----	Particle						----//
		//----	------	------	------	------	----//

		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> CircularSparkles_{ nullptr };
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> TitleCaptionEffect_{ nullptr };
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> ButtonOnFocusEffect_{ nullptr };
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> ButtonOnUnfocusEffect_{ nullptr };
		std::unique_ptr<Lumina::ParticleSystem<Lumina::Particle>> ButtonOnClickEffect_{ nullptr };

		Lumina::D3D12::RootSignature RS_ParticleSystem_{};
		Lumina::D3D12::Shader VS_BasicParticle_{};
		Lumina::D3D12::Shader PS_BasicParticle_{};
		Lumina::D3D12::GraphicsPSO GraphicsPSO_BasicParticle_AdditiveMode_{};

		Lumina::D3D12::UploadBuffer UB_OrthoProj_{};
		Lumina::D3D12::UploadBuffer UB_PostProcessingConstants_{};

		Lumina::D3D12::DescriptorHeap LocalHeap_OrthoProj_{};

		Lumina::D3D12::DescriptorTable GlobalTable_SRV_ImageTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_CanvasTexture_;
		Lumina::D3D12::DescriptorTable GlobalTable_CBV_PostProcessing_;

		Lumina::D3D12::Canvas Canvas_Geometry_;
		Lumina::D3D12::Canvas Canvas_Background_Merge_;
		//Lumina::D3D12::Canvas Canvas_Background_PostProcessing_{};
		//Lumina::D3D12::Canvas Canvas_PostProcessing_{};

		Lumina::D3D12::RenderPass DeferredGeometryPass_{};
		Lumina::D3D12::RenderPass MergePass_{};
		Lumina::D3D12::RenderPass PostProcessingPass_{};

		std::unique_ptr<Lumina::DeferredLighting> DeferredLighting_{};
		Lumina::List<Lumina::PointLight> List_PointLight_{};
		Lumina::List<Lumina::Math::F32x4x4<>> List_Matrix_World_LightSphere_{};
		Lumina::D3D12::DescriptorHeap LocalHeap_ScreenToWorld_{};
		Lumina::D3D12::UploadBuffer UB_ScreenToWorld_{};

		std::vector<ClearFractalAnimation> Arr_FractalAnimation_{};
		float SceneRotation_;

		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager0_{ nullptr };
		std::unique_ptr<Lumina::PrimitiveManager> PrimitiveManager1_{ nullptr };

		uint32_t KeyState_Space_{ 0U };

		float UIFadeIn_{ 0.0f };
		int Count_FadeIn_{ 72 };
		int Count_FadeOut_{ -1 };
		int IsInGameSceneUnloaded_{ 0 };
		ClearPostProcessingConstants ClearPostProcessingConstants_{};

		Lumina::D3D12::UploadBuffer UB_Dummy_{};
		Lumina::D3D12::DescriptorHeap LocalHeap_Dummy_{};

	private:
		enum ImageTextureID : uint32_t {
			IT_StageClear = 0U,
			IT_PressSpaceKey = 1U,
			IT_Particles = 2U,
			IT_Square = 3U,
		};

		enum MeshShaderAssetID : uint32_t {
			MSA_Box = 0U,
		};
		enum MeshMaterialID : uint32_t {
			MM_Box = 0U,
		};
	};
}