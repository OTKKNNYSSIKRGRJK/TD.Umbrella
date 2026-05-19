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
    if (vertices_.empty() || !worldMatrix_) {
        return;
    }

    auto transformVertex = [&](const Vector3& v) -> Vector3 {
        Lumina::Math::F32x4 v4 = { v.X, v.Y, v.Z, 1.0f };
        v4 = v4 * (*worldMatrix_);
        return { v4.X(), v4.Y(), v4.Z() };
    };

    Vector3 minWorld = transformVertex(vertices_[0]);
    Vector3 maxWorld = minWorld;

    for (size_t i = 1; i < vertices_.size(); ++i) {
        Vector3 wv = transformVertex(vertices_[i]);
        
        minWorld.X = std::min(minWorld.X, wv.X);
        minWorld.Y = std::min(minWorld.Y, wv.Y);
        minWorld.Z = std::min(minWorld.Z, wv.Z);

        maxWorld.X = std::max(maxWorld.X, wv.X);
        maxWorld.Y = std::max(maxWorld.Y, wv.Y);
        maxWorld.Z = std::max(maxWorld.Z, wv.Z);
    }

    aabb_.Min = minWorld;
    aabb_.Max = maxWorld;
}
