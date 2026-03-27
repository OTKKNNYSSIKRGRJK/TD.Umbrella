export module CollisionManager : GJK;

import <vector>;

import Lumina.Core.Math;

namespace {
	using Vector2 = Lumina::Math::F32x2;
}

namespace GJKUtils {
	export struct SupportPoint {
		Vector2 v;// 差分ベクトル(ShapeB - ShapeA)
		Vector2 supA;// 各形状のサポートポイント(後で衝突点計算に使う場合)
		Vector2 supB;
	};

	export struct Contact {
		Vector2 direction;
		float depth;
	};

	export Vector2 support(std::vector<Vector2>vertices, const Vector2& direction);

	export SupportPoint getSupport(std::vector<Vector2>shapeA, std::vector<Vector2>shapeB, const Vector2& direction);

	export Vector2 getPerpendicularToOrigin(Vector2 edge, Vector2 toOrigin);

	export Vector2 GetOriginProjection(const Vector2& v1, const Vector2& v2);

	export float GetDistanceToOrigin(const Vector2& point);

	export Vector2 Normalize(const Vector2& v);

	export bool UpdateSimplex(std::vector<SupportPoint>& simplex, Vector2& direction);

	export Contact EPA(std::vector<Vector2>shapeA, std::vector<Vector2>shapeB, std::vector<SupportPoint>& simplex);

	export bool collision(std::vector<Vector2> shapeA, std::vector<Vector2>shapeB, std::vector<SupportPoint>& simplex);
}