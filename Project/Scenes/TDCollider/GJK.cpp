module CollisionManager : GJK;

import <limits>;

namespace {
	using Vector2 = Lumina::Math::F32x2;
}

namespace GJKUtils {
	float GetDistanceToOrigin(const Vector2& point) {
		return sqrtf(point.X * point.X + point.Y * point.Y);
	}

	Vector2 Normalize(const Vector2& v) {
		float length = sqrtf(v.X * v.X + v.Y * v.Y);
		if (length > 0) {
			return { v.X / length, v.Y / length };
		}
		return { 0.0f, 0.0f }; // 長さがゼロの場合はゼロベクトルを返す
	}

	Vector2 support(std::vector<Vector2>vertices, const Vector2& direction) {
		// 内積が最大の点を探す
		// 保存する変数
		float maxDot = std::numeric_limits<float>::lowest();// 最小値からスタート
		Vector2 supportPoint = vertices[0];
		// 全頂点を調べる
		for (const Vector2& vertex : vertices) {
			float dot = vertex.X * direction.X + vertex.Y * direction.Y;
			if (dot > maxDot) {
				maxDot = dot;
				supportPoint = vertex;
			}
		}
		return supportPoint;
	}

	SupportPoint getSupport(std::vector<Vector2>shapeA, std::vector<Vector2>shapeB, const Vector2& direction) {
		SupportPoint p = {};
		p.supA = support(shapeA, direction);
		p.supB = support(shapeB, { -direction.X,-direction.Y });
		p.v = { p.supA.X - p.supB.X, p.supA.Y - p.supB.Y };
		return p;
	}

	Vector2 getPerpendicularToOrigin(Vector2 edge, Vector2 toOrigin) {
		// 垂直なベクトルを作成
		Vector2 perp = { -edge.Y, edge.X };

		// 原点方向を向いているかを内積で確認
		float dot = perp.X * toOrigin.X + perp.Y * toOrigin.Y;

		if (dot < 0) {
			// 反転
			perp.X = -perp.X;
			perp.Y = -perp.Y;
		}

		return perp;
	}

	Vector2 GetOriginProjection(const Vector2& v1, const Vector2& v2) {
		float a = v1.Y - v2.Y;
		float b = v2.X - v1.X;
		float c = v1.X * v2.Y - v2.X * v1.Y;

		// デノミネーター(分母)
		float denom = a * a + b * b;
		// 原点からの最短点を求める
		Vector2 projection = { -a * c / denom, -b * c / denom };
		return projection;
	}

	bool UpdateSimplex(std::vector<SupportPoint>& simplex, Vector2& direction) {
		if (simplex.size() == 2) {
			// 直線のケース
			SupportPoint p = simplex[0];
			SupportPoint r = simplex[1];
			//// 線分PRの作成
			//Vector2 pr = { r.v.X - p.v.X, r.v.Y - p.v.Y };
			//Vector2 rO = { -r.v.X, -r.v.Y };

			//// PRの垂線で原点方向を向くものを次の方向に設定
			//direction = getPerpendicularToOrigin(pr, rO);
			direction = GetOriginProjection(p.v, r.v);
			direction.X = -direction.X;
			direction.Y = -direction.Y;
			return false;
		}
		else {

			// 三角形の場合
			SupportPoint a = simplex[0];
			SupportPoint b = simplex[1];
			SupportPoint c = simplex[2];

			Vector2 ab = { b.v.X - a.v.X, b.v.Y - a.v.Y };
			Vector2 bc = { c.v.X - b.v.X, c.v.Y - b.v.Y };
			Vector2 ca = { a.v.X - c.v.X, a.v.Y - c.v.Y };

			Vector2 ao = { -a.v.X,-a.v.Y };
			Vector2 bo = { -b.v.X,-b.v.Y };
			Vector2 co = { -c.v.X,-c.v.Y };

			float crossABAO = Vector2::Cross(ab, ao);
			float crossBCBO = Vector2::Cross(bc, bo);
			if (crossABAO * crossBCBO < 0) {
				// 符号が違うのでFalse
				// 最初の頂点を削除
				simplex.erase(simplex.begin());
				direction = GetOriginProjection(b.v, c.v);
				direction.X = -direction.X;
				direction.Y = -direction.Y;
				return false;
			}
			float crossCACO = Vector2::Cross(ca, co);
			if (crossBCBO * crossCACO < 0) {
				// 符号が違うのでFalse
				// 最初の頂点を削除
				simplex.erase(simplex.begin());
				direction = GetOriginProjection(b.v, c.v);
				direction.X = -direction.X;
				direction.Y = -direction.Y;
				return false;
			}
			// ここまできたら原点は含まれているのでOK
			return true;
		}
	}

	Contact EPA(std::vector<Vector2>shapeA, std::vector<Vector2>shapeB, std::vector<SupportPoint>& simplex) {
		const int MAX_ITERATIONS = 30;// 無限ループ防止用
		const float TOLERANCE = 0.0001f;// 許容範囲

		Contact contact = {};

		for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
			float minDepth = std::numeric_limits<float>::max();
			Vector2 minDirection = { 0.0f,0.0f };
			int insertIndex = 0;

			int numPoints = (int)simplex.size();

			// 全ての辺に対して原点からの距離を求める
			for (int i = 0; i < numPoints; ++i) {
				int j = (i + 1) % numPoints;// 次の点のインデックス
				SupportPoint p1 = simplex[i];
				SupportPoint p2 = simplex[j];

				// [ 点を求める ]
				Vector2 closestPoint = GetOriginProjection(p1.v, p2.v);
				// [ 距離を求める ]
				float depth = GetDistanceToOrigin(closestPoint);

				// [ 一番近い距離を更新 ]
				if (depth < minDepth) {
					minDepth = depth;
					minDirection = closestPoint; // 原点から最も近い点へのベクトル
					insertIndex = j;             // 後で頂点を挿入する位置
				}
			}

			// 押し出し方向のベクトルを正規化（長さ1にする）
			// ※ ユーザー環境の正規化関数（Normalize等）を使用してください
			Vector2 normal = Normalize(minDirection);

			// 2. 求めたベクトルからサポートポイントを求める
			SupportPoint d = getSupport(shapeA, shapeB, normal);

			// 新しいサポートポイントが、原点から法線方向にどれだけ離れているか（内積）
			float newDepth = Vector2::Dot(d.v, normal);

			// 3. 終了判定（似たような値が出たか）
			if (newDepth - minDepth < TOLERANCE) {
				// 境界に到達したと判定して結果を返す
				contact.depth = newDepth; // または minDepth
				contact.direction = normal;
				return contact;
			}

			// 4. 境界に達していなければ、求めたサポートポイントを辺の間に挿入して多角形を拡張 (Expand) する
			simplex.insert(simplex.begin() + insertIndex, d);

		}
		// 計算の結果を構造体にいれ、返す
		return contact;
	}

	bool collision(std::vector<Vector2> shapeA, std::vector<Vector2>shapeB, std::vector<SupportPoint>& simplex) {
		// 初期方向
		Vector2 direction = { 1.0f,0.0f };
		// 最初のサポートポイント
		SupportPoint p = getSupport(shapeA, shapeB, direction);

		// 単体(Simplex)を管理
		simplex.push_back(p);// 登録
		// 次の方向の取得
		direction = { -p.v.X,-p.v.Y };
		int index = 0;// ループ制御変数
		while (index < 30) {
			SupportPoint r = getSupport(shapeA, shapeB, direction);

			// 新しい点が原点を超えていないなら衝突しない
			if ((r.v.X * direction.X + r.v.Y * direction.Y) < 0) {
				return false;
			}

			simplex.push_back(r);

			if (UpdateSimplex(simplex, direction)) {
				return true;
			}
			index++;
		}
		return false;
	}
}