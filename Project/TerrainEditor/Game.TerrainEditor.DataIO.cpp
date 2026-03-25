module Game.TerrainEditor;

import <string>;

import nlohmann.json;

namespace Game {
	namespace {
		auto operator>>(
			nlohmann::json const& in_,
			[[maybe_unused]] Lumina::List<Polygon>& polygons_
		) -> nlohmann::json const& {
			auto const& arr_Polygons{ in_.at("Polygons") };
			for (auto const& dict_PolygonAttrs : arr_Polygons) {
				auto& polygon{ polygons_.New() };
				
				auto const& arr_Vertices{ dict_PolygonAttrs.at("Vertices") };
				for (auto const& dict_VerticeAttrs : arr_Vertices) {
					auto& vert{ polygon.Points.emplace_back() };

					auto const& arr_Pos{ dict_VerticeAttrs.at("Pos") };
					vert.Pos.X = arr_Pos.at(0).get<Lumina::F32>();
					vert.Pos.Y = arr_Pos.at(1).get<Lumina::F32>();
				}
			}

			return in_;
		}
	}

	template<>
	auto TerrainEditor::InputData(nlohmann::json const& input_) -> void {
		/*Lumina::List<Polygon> Polygons_;
		Lumina::List<GroundPoint> GroundPolygon_;

		Lumina::I32 CurrentPolygonID_;
		Lumina::I32 CurrentPolygonID_LastestUnused_;

		Lumina::I32 PreviousGroundPointID_;*/

		Reset();

		auto const& dict_MapInfo{ input_.at("MapInfo") };
		auto const& arr_MapSize{ dict_MapInfo.at("Size") };
		CanvasSize_.X = arr_MapSize.at(0).get<Lumina::F32>();
		CanvasSize_.Y = arr_MapSize.at(1).get<Lumina::F32>();

		SelectedPoint_ = nullptr;
		SelectedGroundPoint_ = nullptr;

		input_ >> Polygons_;
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
					auto& dict_PolygonAttrs{ arr_Polygons.emplace_back(nlohmann::ordered_json::object()) };
					dict_PolygonAttrs["Vertices"] = nlohmann::ordered_json::array();
					auto& arr_Vertices{ dict_PolygonAttrs["Vertices"] };
					for (auto const& p : polygon.Points) {
						auto& dict_VerticeAttrs{ arr_Vertices.emplace_back() };
						dict_VerticeAttrs["Pos"].emplace_back(p.Pos.X);
						dict_VerticeAttrs["Pos"].emplace_back(p.Pos.Y);
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