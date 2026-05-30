module Game.Scene.InGame;

import : Impl;
import : Impl.Effect;

import <cmath>;

import Game.Events.InGame;
import Lumina.Utils.Color;
import Lumina.Core.Math;

namespace Game::Scene::Impl {
	namespace {
		template<typename _ParticleSystem, typename _Lambda>
		auto PreparePointLights(
			_ParticleSystem const& particleSystem_,
			_Lambda lambda_
		) {
			auto const& particleList{ particleSystem_.InstanceList() };
			using ParticleListIterator =
				typename std::_Remove_const_ref_t<decltype(particleList)>::Iterator;
			ParticleListIterator it_Particle{ particleList };
			for (it_Particle.Begin(); !it_Particle.End(); it_Particle.Next()) {
				auto const& particle{ *it_Particle };
				lambda_(particle);
			}
		}
	}

	template<>
	auto InGame::Update_<"Lighting.PointLight.PreBatch">() -> void {
		List_PointLight_.Clear();
		List_LocalToWorld_LightSphere_.Clear();
	}

	template<>
	auto InGame::Update_<"Lighting.PointLight.Batch.BasedOnParticle">() -> void {
		auto makePointLightBasedOnParticle{
			[this] (
				Lumina::Particle const& particle_,
				Lumina::F32 intensity_
			) -> Lumina::PointLight& {
				auto& pointLight{ List_PointLight_.New() };

				pointLight.WorldPosition = {
					particle_.Translate.X,
					particle_.Translate.Y,
					particle_.Translate.Z,
					1.0f
				};
				pointLight.RGB = {
					particle_.RenderData.RGBA.X,
					particle_.RenderData.RGBA.Y,
					particle_.RenderData.RGBA.Z
				};
				pointLight.Intensity = particle_.Scale.X * intensity_;

				return pointLight;
			}
		};

		auto makeLightSphereTransform{
			[this] (
				Lumina::PointLight const& pointLight_,
				Lumina::F32 inv_Threshold_ = 1024.0f,
				Lumina::F32 factor_MaxComp_intensity_ = 0.5f,
				Lumina::F32 factor_Constant_ = 1.0f,
				Lumina::F32 factor_Linear_ = 1.0f,
				Lumina::F32 factor_Quadratic_ = 0.5f
			) -> void {
				auto& lightSphere{ List_LocalToWorld_LightSphere_.New() };

				Lumina::F32 const radius{
					Lumina::LightSphereRadius(
						inv_Threshold_,
						pointLight_.Intensity * factor_MaxComp_intensity_,
						factor_Constant_,
						factor_Linear_,
						factor_Quadratic_
					)
				};
				lightSphere = {
					radius, 0.0f, 0.0f, 0.0f,
					0.0f, radius, 0.0f, 0.0f,
					0.0f, 0.0f, radius, 0.0f,
					pointLight_.WorldPosition.X,
					pointLight_.WorldPosition.Y,
					pointLight_.WorldPosition.Z,
					1.0f,
				};
			}
		};

		/*PreparePointLights(
			*KnockEffects_,
			[&, this] (Lumina::Particle const& particle_) {
				if (!List_PointLight_.IsFull()) {
					auto& pointLight{
						makePointLightBasedOnParticle(
							particle_,
							particle_.Scale.X * 30.0f
						)
					};
					makeLightSphereTransform(pointLight);
				}
			}
		);*/

		PreparePointLights(
			*PlayerEffects_,
			[&, this] (Lumina::Particle const& particle_) {
				if (
					!List_PointLight_.IsFull() &&
					(particle_.RenderData.DiffuseAtlasID == 4U)
				) {
					auto& pointLight{
						makePointLightBasedOnParticle(
							particle_,
							particle_.RenderData.RGBA.W * 50.0f
						)
					};
					makeLightSphereTransform(
						pointLight, 512.0f, 0.5f, 1.0f, 1.0f, 0.5f
					);
				}
			}
		);
		
		/*PreparePointLights(
			*KnockEffects_,
			[&, this] (Lumina::Particle const& particle_) {
				if (
					!List_PointLight_.IsFull() &&
					(particle_.RenderData.DiffuseAtlasID == 4U)
				) {
					auto& pointLight{
						makePointLightBasedOnParticle(
							particle_,
							particle_.RenderData.RGBA.W * 100.0f
						)
					};
					makeLightSphereTransform(pointLight);
				}
			}
		);*/

		/*

		ParticleListIterator it_Sparkle{ AmbientSparkles_->InstanceList() };
		for (it_Sparkle.Begin(); !it_Sparkle.End(); it_Sparkle.Next()) {
			auto const& sparkle{ *it_Sparkle };

			if (!List_PointLight_.IsFull()) {
				auto& pointLight{
					makePointLightBasedOnParticle(
						sparkle,
						sparkle.RenderData.RGBA.W * 400.0f
					)
				};
				makeLightSphereTransform(pointLight, 1024.0f, 1.0f, 1.0f, 1.0f, 0.5f);
			}
		}
		
		*/
	}

	template<>
	auto InGame::Update_<"Lighting.PointLight.PostBatch">() -> void {
		Arr_Index_ActivePointLight_.clear();
		Lumina::List<Lumina::PointLight>::Iterator it_Light{ List_PointLight_ };
		for (it_Light.Begin(); !it_Light.End(); it_Light.Next()) {
			Arr_Index_ActivePointLight_.emplace_back(it_Light.Index());
		}

		DeferredLighting_->Update(
			List_PointLight_,
			List_LocalToWorld_LightSphere_,
			Arr_Index_ActivePointLight_
		);
	}

	template<>
	auto InGame::Update_<"Lighting">() -> void {
		InGame::Update_<"Lighting.PointLight.PreBatch">();
		InGame::Update_<"Lighting.PointLight.Batch.BasedOnParticle">();
		InGame::Update_<"Lighting.PointLight.PostBatch">();
	}
}