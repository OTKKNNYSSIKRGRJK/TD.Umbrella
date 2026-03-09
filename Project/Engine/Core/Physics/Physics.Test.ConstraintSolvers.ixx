export module Lumina.Core.Physics : Test.ConstraintSolvers;

import <list>;

import Lumina.Core.Common;
import Lumina.Core.Math;

import : RigidBody;

namespace Lumina::Physics {
	export struct Constraint {
		Math::F32x3 ImpulsePosition;
		Math::F32x3 ImpulseNormal;
		Math::F32x3 JLV0;
		Math::F32x3 JLV1;
		Math::F32x3 JAV0;
		Math::F32x3 JAV1;
		F32 Bias;
		F32 BiasFactor_JV;
		F32 Lambda_Current;
		F32 Lambda_Min;
		F32 Lambda_Max;
		F32 Inv_Mass;
		RigidBody* RB0;
		RigidBody* RB1;
	};

	export Constraint NormalContact(
		RigidBody& rb0_,
		RigidBody& rb1_,
		Math::F32x3 const& contactPoint_,
		Math::UnitF32x3 const& contactNorm_,
		[[maybe_unused]] F32 cor_ = 1.0f
	) {
		Constraint ret{};
		{
			ret.ImpulsePosition = contactPoint_;
			ret.ImpulseNormal = contactNorm_;

			Math::F32x3 const r0{ ret.ImpulsePosition - rb0_.Position };
			Math::F32x3 const r1{ ret.ImpulsePosition - rb1_.Position };

			ret.JLV0 = ret.ImpulseNormal * (-1.0f);
			ret.JLV1 = ret.ImpulseNormal;
			ret.JAV0 = Math::F32x3::Cross(r0, ret.ImpulseNormal) * (-1.0f);
			ret.JAV1 = Math::F32x3::Cross(r1, ret.ImpulseNormal);

			ret.BiasFactor_JV = -cor_;

			ret.Lambda_Current = 0.0f;
			ret.Lambda_Min = 0.0f;
			ret.Lambda_Max = Numeric::Max<F32>;

			// J * M^(-1) * Tr(J) 
			F32 const m{
				rb0_.Inv_Mass() +
				rb1_.Inv_Mass() +
				Math::F32x3::Dot(rb0_.Inv_Inertia * ret.JAV0, ret.JAV0) +
				Math::F32x3::Dot(rb1_.Inv_Inertia * ret.JAV1, ret.JAV1)
			};
			ret.Inv_Mass = 1.0f / m;

			ret.RB0 = &rb0_;
			ret.RB1 = &rb1_;
		}
		return ret;
	}

	export class Solver {
	public:
		void AddConstraint(Constraint&&);

	public:
		void Update();

	private:
		std::list<Constraint> Constraints_{};
	};

	void Solver::AddConstraint(Constraint&& constraint_) {
		Constraints_.emplace_back(std::move(constraint_));
	}

	void Solver::Update() {
		int loopTime = 5;
		for (int i = 0; i < loopTime; ++i) {
			for (auto& constraint : Constraints_) {
				auto* rb0{ constraint.RB0 };
				auto* rb1{ constraint.RB1 };

				F32 jv{
					Math::F32x3::Dot(constraint.JLV0, rb0->LinearVelocity) +
					Math::F32x3::Dot(constraint.JAV0, rb0->AngularVelocity)
				};
				if (rb1 != nullptr) {
					jv +=
						Math::F32x3::Dot(constraint.JLV1, rb1->LinearVelocity) +
						Math::F32x3::Dot(constraint.JAV1, rb1->AngularVelocity);
				}
				F32 d_Lambda{ -(jv + constraint.Bias) * constraint.Inv_Mass };
				F32 const lambda_Prev{ constraint.Lambda_Current };
				constraint.Lambda_Current = std::clamp(
					lambda_Prev + d_Lambda,
					constraint.Lambda_Min,
					constraint.Lambda_Max
				);
				d_Lambda = constraint.Lambda_Current - lambda_Prev;

				Math::F32x3 imp{ d_Lambda * constraint.ImpulseNormal };
				Math::F32x3 rev_Imp{ imp * (-1.0f) };
				rb0->ApplyLinearImpulse(rev_Imp);
				rb0->ApplyAngularImpulse(rev_Imp, constraint.ImpulsePosition);
				if (rb1 != nullptr) {
					rb1->ApplyLinearImpulse(imp);
					rb1->ApplyAngularImpulse(imp, constraint.ImpulsePosition);
				}
			}
		}
	}
}

namespace Lumina::Physics {
	export auto CalculateImpulseMagnitudeOnCollision(
		F32 cor_,
		Math::F32x3 const& contactPoint_,
		RigidBody const& rb0_,
		RigidBody const& rb1_
	) -> F32 {
		Math::UnitF32x3 const contactNormal{ rb1_.Position - rb0_.Position };
		Math::F32x3 const r0{ contactPoint_ - rb0_.Position };
		Math::F32x3 const r1{ contactPoint_ - rb1_.Position };
		Math::F32x3 const v_P0{ rb0_.LinearVelocity + Math::F32x3::Cross(rb1_.AngularVelocity, r0) };
		Math::F32x3 const v_P1{ rb1_.LinearVelocity + Math::F32x3::Cross(rb1_.AngularVelocity, r1) };
		Math::F32x3 const v_Rel{ v_P1 - v_P0 };
		F32 const proj_VOnNormal{ Math::F32x3::Dot(v_Rel, contactNormal) };
		if (proj_VOnNormal < 0.0f) { return 0.0f; }
		F32 const k{
			rb0_.Inv_Mass() +
			rb1_.Inv_Mass() +
			Math::F32x3::Dot(
				contactNormal,
				Math::F32x3::Cross(rb0_.Inv_Inertia * Math::F32x3::Cross(r0, contactNormal), r0) +
				Math::F32x3::Cross(rb1_.Inv_Inertia * Math::F32x3::Cross(r1, contactNormal), r1)
			)
		};
		auto&& tmp0 = Math::F32x3::Cross(r0, contactNormal);
		auto&& tmp1 = Math::F32x3::Cross(r1, contactNormal);
		[[maybe_unused]] F32 const m{
				rb0_.Inv_Mass() +
				rb1_.Inv_Mass() +
				Math::F32x3::Dot(rb0_.Inv_Inertia * tmp0, tmp0) +
				Math::F32x3::Dot(rb1_.Inv_Inertia * tmp1, tmp1)
		};
		return (-(1.0f + cor_) * proj_VOnNormal / k);
	}
}