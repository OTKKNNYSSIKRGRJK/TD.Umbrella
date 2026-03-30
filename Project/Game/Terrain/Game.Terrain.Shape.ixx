export module Game.Terrain : Shape;

import <memory>;
import <vector>;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Utils.Camera;
import Lumina.Utils.Misc;

import Collider;

namespace Game {
	export struct Polygon {
		struct Vertex {
			Lumina::Math::F32x3 Pos;
		};

		std::vector<Vertex> Vertices;

		using Collection = Lumina::List<Polygon>;
	};

	export struct Ground {
		struct Vertex {
			Lumina::Math::F32x3 Pos;
			Lumina::I32 ID;
			Lumina::I32 PrevID;
			Lumina::I32 NextID;
		};

		Lumina::List<ConvexCollider> Colliders;

		Lumina::List<Vertex> Vertices;
	};

	export class TerrainShapeCollection {
	public:
		auto PolygonsData() noexcept -> Lumina::List<Polygon>& { return Polygons_; }
		auto PolygonsData() const noexcept -> Lumina::List<Polygon> const& { return Polygons_; }
		auto GroundData() noexcept -> Ground& { return Ground_; }
		auto GroundData() const noexcept -> Ground const& { return Ground_; }

	public:
		template<typename _Serialized>
		void Initialize(_Serialized const& serialized_);

		auto ConvertToWorldCoordinate(
			TerrainShapeCollection& out_,
			Lumina::Utils::Camera const& camera_,
			Lumina::Utils::Viewport const& viewport_
		) const -> void;

	public:
		TerrainShapeCollection();
		~TerrainShapeCollection();

	private:
		Polygon::Collection Polygons_;
		Ground Ground_;
	};
}