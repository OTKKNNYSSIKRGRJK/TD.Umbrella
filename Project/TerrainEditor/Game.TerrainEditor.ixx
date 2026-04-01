export module Game.TerrainEditor;

import <memory>;
import <vector>;

import nlohmann.json;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Game.Terrain;
import Lumina.Utils.Camera;
import Lumina.Utils.Misc;

namespace Game {

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
		auto OutputData(T& output_) const -> void;

	public:
		auto Update() -> void;

	public:
		auto SetShapes(TerrainShapeCollection& shapes_) -> void {
			Shapes_ = &shapes_;
		}
		auto SetCamera(Lumina::Utils::Camera const& camera_) -> void {
			*Camera_ = camera_;
		}
		auto SetViewport(Lumina::Utils::Viewport const& viewport_) -> void {
			Viewport_ = &viewport_;
		}

	public:
		auto Initialize() -> void;

	private:
		Polygon::Collection Polygons_;
		Ground Ground_;

		TerrainShapeCollection* Shapes_;
		std::unique_ptr<Lumina::Utils::Camera> Camera_;
		Lumina::Utils::Viewport const* Viewport_;

		Lumina::I32 CurrentPolygonID_;
		Lumina::I32 CurrentPolygonID_LastestUnused_;

		Lumina::I32 PreviousGroundPointID_;

		Polygon::Vertex* SelectedPoint_;
		Ground::Vertex* SelectedGroundPoint_;
		Lumina::I32 IsEditingGround_;

		Lumina::Math::F32x2 CanvasScreenPos_;
		Lumina::Math::F32x2 CanvasSize_;
		Lumina::Math::F32x2 CanvasSize_MIN_;
		Lumina::Math::F32x2 MouseScreenPos_;
		Lumina::Math::F32x2 MouseLocalPos_;

		Lumina::F32 Zoom_;
		
		Lumina::Math::F32x2 GroundOffset_;
	};
}