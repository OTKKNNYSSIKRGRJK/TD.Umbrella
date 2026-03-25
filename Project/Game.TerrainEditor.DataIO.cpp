module Game.TerrainEditor;

import <string>;

import nlohmann.json;

namespace Game {
	template<>
	auto TerrainEditor::InputData(nlohmann::json const& input_) -> void {
		/*Lumina::List<Polygon> Polygons_;
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
		Lumina::Math::F32x2 MouseLocalPos_;*/

		auto const& dict_MapInfo{ input_.at("MapInfo") };
		auto const& arr_MapSize{ dict_MapInfo.at("Size") };
		CanvasSize_.X = arr_MapSize.at(0).get<Lumina::F32>();
		CanvasSize_.Y = arr_MapSize.at(1).get<Lumina::F32>();
	}
}

namespace Game {
	namespace {
		auto operator<<(
			nlohmann::ordered_json& out_,
			Lumina::List<Polygon> const& polygons_
		) -> nlohmann::ordered_json& {
			out_["Polygons"] = nlohmann::ordered_json::array();
			auto& arr_Polygons{ out_["Polygons"] };
			Lumina::List<Polygon>::Iterator it{ polygons_ };
			for (it.Begin(); !it.End(); it.Next()) {
				auto const& polygon{ *it };
				if (!polygon.Points.empty()) {
					auto& arr_Vertices{ arr_Polygons.emplace_back() };
					for (auto const& p : polygon.Points) {
						auto& dict_VerticeProp{ arr_Vertices.emplace_back() };
						dict_VerticeProp["Pos"].emplace_back(p.Pos.X);
						dict_VerticeProp["Pos"].emplace_back(p.Pos.Y);
					}
				}
			}
			return out_;
		}

		auto operator<<(
			nlohmann::ordered_json& out_,
			Lumina::List<GroundPoint> const& groundPoints_
		) -> nlohmann::ordered_json& {
			out_["GroundPoints"] = nlohmann::ordered_json::array();
			auto& arr_GroundPoints{ out_["GroundPoints"] };
			Lumina::List<GroundPoint>::Iterator it{ groundPoints_ };
			for (it.Begin(); !it.End(); it.Next()) {
				auto& dict_VerticeProp{ arr_GroundPoints.emplace_back() };
				auto const& p{ *it };
				dict_VerticeProp.emplace("ID", it.Index());
				dict_VerticeProp.emplace("PrevID", p.Prev);
				dict_VerticeProp.emplace("NextID", p.Next);
				dict_VerticeProp["Pos"] = nlohmann::ordered_json::array();
				dict_VerticeProp["Pos"].emplace_back(p.Pos.X);
				dict_VerticeProp["Pos"].emplace_back(p.Pos.Y);
			}
			return out_;
		}
	}

	template<>
	auto TerrainEditor::OutputData() const -> nlohmann::ordered_json {
		nlohmann::ordered_json ret{};
		ret["MapInfo"] = nlohmann::ordered_json::object();
		ret["MapInfo"]["Size"].emplace_back(CanvasSize_.X);
		ret["MapInfo"]["Size"].emplace_back(CanvasSize_.Y);
		ret << Polygons_ << GroundPolygon_;
		return ret;
	}
}