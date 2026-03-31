export module Game.TerrainEditor;

import <vector>;

import Lumina.Core.Common;
import Lumina.Core.Math;

namespace Game {
	struct Point {
		Lumina::Math::F32x2 Pos;
	};

	struct GroundPoint {
		Lumina::Math::F32x2 Pos;
		Lumina::I32 ID;
		Lumina::I32 Prev;
		Lumina::I32 Next;
	};

	struct Polygon {
		std::vector<Point> Points;
	};

	export class TerrainEditor {
	private:
		using EditMethod = auto (TerrainEditor::*)() -> void;
		EditMethod CurrentEditMethod_;

		auto AddGroundVertices() -> void;
		auto AddVertices() -> void;
		auto EditGroundVertexMode() -> void;
		auto EditVertexMode() -> void;
		auto PolygonMode() -> void;

	private:
		auto DrawCanvas() const -> void;

		auto OpenFile() -> void;
		auto SaveFile() const -> void;

		auto Reset() -> void;

		template<typename T>
		auto InputData(T const& input_) -> void;
		template<typename T>
		auto OutputData() const -> T;

	public:
		auto Update() -> void;

	public:
		auto Initialize() -> void;

	private:
		Lumina::List<Polygon> Polygons_;
		Lumina::List<GroundPoint> GroundPolygon_;

		Lumina::I32 CurrentPolygonID_;
		Lumina::I32 CurrentPolygonID_LastestUnused_;

		Lumina::I32 PreviousGroundPointID_;

		Point* SelectedPoint_;
		GroundPoint* SelectedGroundPoint_;
		Lumina::I32 IsEditingGround_;

		Lumina::Math::F32x2 CanvasScreenPos_;
		Lumina::Math::F32x2 CanvasSize_;
		Lumina::Math::F32x2 CanvasSize_MIN_;
		Lumina::Math::F32x2 MouseScreenPos_;
		Lumina::Math::F32x2 MouseLocalPos_;

		Lumina::F32 Zoom_;
		
	};
}