module Collider;

namespace {
    using Vector3 = Lumina::Math::F32x3;
    using Matrix4x4 = Lumina::Math::F32x4x4<>;
}

//////////////////////
///
///  Convex
///
///////////////////////
void ConvexCollider::UpdateAABB() {
    // 頂点が1つもセットされていない場合は計算しない
    if (vertices_.empty()) {
        return;
    }

    // 最初は、0番目の頂点を最小値・最大値の基準にする
    Vector3 minLocal = vertices_[0];
    Vector3 maxLocal = vertices_[0];

    // 1番目以降の頂点と比較して、最小・最大を更新していく
    for (size_t i = 1; i < vertices_.size(); ++i) {
        minLocal.X = std::min(minLocal.X, vertices_[i].X);
        minLocal.Y = std::min(minLocal.Y, vertices_[i].Y);
        minLocal.Z = std::min(minLocal.Z, vertices_[i].Z);

        maxLocal.X = std::max(maxLocal.X, vertices_[i].X);
        maxLocal.Y = std::max(maxLocal.Y, vertices_[i].Y);
        maxLocal.Z = std::max(maxLocal.Z, vertices_[i].Z);
    }

    // 計算したローカルの最小値・最大値に、ワールド座標を足して aabb_ にセットする
    // ※ AABB構造体が min, max というメンバを持っている想定です
    aabb_.Min.X = minLocal.X + worldPosition_.X;
    aabb_.Min.Y = minLocal.Y + worldPosition_.Y;
    aabb_.Min.Z = minLocal.Z + worldPosition_.Z;

    aabb_.Max.X = maxLocal.X + worldPosition_.X;
    aabb_.Max.Y = maxLocal.Y + worldPosition_.Y;
    aabb_.Max.Z = maxLocal.Z + worldPosition_.Z;
}
