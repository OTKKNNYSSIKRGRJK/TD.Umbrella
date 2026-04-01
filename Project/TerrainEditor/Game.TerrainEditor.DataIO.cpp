module Game.TerrainEditor;

import <string>;

import nlohmann.json;
import Game.Terrain;

namespace Game {
	namespace {
		auto operator>>(
			nlohmann::json const& in_,
			[[maybe_unused]] std::vector<Polygon>& polygons_
		) -> nlohmann::json const& {
			if (!in_.contains("Polygons")) return in_;
			auto const& arr_Polygons{ in_.at("Polygons") };
			for (auto const& dict_PolygonAttrs : arr_Polygons) {
				auto& polygon{ polygons_.emplace_back() };
				
				auto const& arr_Vertices{ dict_PolygonAttrs.at("Vertices") };
				for (auto const& dict_VerticeAttrs : arr_Vertices) {
					auto& vert{ polygon.Vertices.emplace_back() };

					auto const& arr_Pos{ dict_VerticeAttrs.at("Pos") };
					vert.Pos.X = arr_Pos.at(0).get<Lumina::F32>();
					vert.Pos.Y = arr_Pos.at(1).get<Lumina::F32>();
				}
			}

			return in_;
		}

		auto operator>>(
			nlohmann::json const& in_,
			[[maybe_unused]] std::vector<Ground::Vertex>& groundPolygon_
		) -> nlohmann::json const& {
			if (!in_.contains("GroundPoints")) return in_;
			auto const& arr_GroundPoints{ in_.at("GroundPoints") };
			for (auto const& dict_VertexAttrs : arr_GroundPoints) {
				auto& vert{ groundPolygon_.emplace_back() };

				vert.ID = dict_VertexAttrs.at("ID").get<Lumina::I32>();
				vert.PrevID = dict_VertexAttrs.at("PrevID").get<Lumina::I32>();
				vert.NextID = dict_VertexAttrs.at("NextID").get<Lumina::I32>();
				auto const& arr_Pos{ dict_VertexAttrs.at("Pos") };
				vert.Pos.X = arr_Pos.at(0).get<Lumina::F32>();
				vert.Pos.Y = arr_Pos.at(1).get<Lumina::F32>();
			}

			return in_;
		}
	}

	template<>
	auto TerrainEditor::InputData(nlohmann::json const& input_) -> void {
		/*Lumina::List<Polygon> Polygons_;
		Lumina::List<Ground::Vertex> GroundPolygon_;

		Lumina::I32 CurrentPolygonID_;
		Lumina::I32 CurrentPolygonID_LastestUnused_;

		Lumina::I32 PreviousGroundPointID_;*/

		Reset();

		/*if (input_.is_object()) {
			OriginalData_ = input_;
		} else {
			OriginalData_ = nlohmann::json::object();
		}*/

		if (input_.contains("width") && input_.contains("height")) {
			CanvasSize_.X = input_.at("width").get<Lumina::F32>();
			CanvasSize_.Y = input_.at("height").get<Lumina::F32>();
		} else if (input_.contains("MapInfo")) {
			auto const& dict_MapInfo{ input_.at("MapInfo") };
			if (dict_MapInfo.contains("Size")) {
				auto const& arr_MapSize{ dict_MapInfo.at("Size") };
				CanvasSize_.X = arr_MapSize.at(0).get<Lumina::F32>();
				CanvasSize_.Y = arr_MapSize.at(1).get<Lumina::F32>();
			}
		} else {
			CanvasSize_.X = 1280.0f;
			CanvasSize_.Y = 720.0f;
		}

		SelectedPoint_ = nullptr;
		SelectedGroundPoint_ = nullptr;

		input_ >> Polygons_ >> Ground_.Vertices;

		[[maybe_unused]] auto& newPolygon{ Polygons_.emplace_back() };
		CurrentPolygonID_ = static_cast<Lumina::I32>(&newPolygon - Polygons_.data());
		CurrentPolygonID_LastestUnused_ = CurrentPolygonID_;
	}
}

namespace Game {
	namespace {
		auto operator<<(
			nlohmann::ordered_json& out_,
			std::vector<Polygon> const& polygons_
		) -> nlohmann::ordered_json& {
			out_["Polygons"] = nlohmann::ordered_json::array();
			auto& arr_Polygons{ out_["Polygons"] };
			for (auto const& polygon : polygons_) {
				if (!polygon.Vertices.empty()) {
					auto& dict_PolygonAttrs{ arr_Polygons.emplace_back(nlohmann::ordered_json::object()) };
					dict_PolygonAttrs["Vertices"] = nlohmann::ordered_json::array();
					auto& arr_Vertices{ dict_PolygonAttrs["Vertices"] };
					for (auto const& p : polygon.Vertices) {
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
			std::vector<Ground::Vertex> const& groundPoints_
		) -> nlohmann::ordered_json& {
			out_["GroundPoints"] = nlohmann::ordered_json::array();
			auto& arr_GroundPoints{ out_["GroundPoints"] };
			for (auto const& p : groundPoints_) {
				auto& dict_VerticeProp{ arr_GroundPoints.emplace_back() };
				dict_VerticeProp.emplace("ID", static_cast<Lumina::U32>(&p - groundPoints_.data()));
				dict_VerticeProp.emplace("PrevID", p.PrevID);
				dict_VerticeProp.emplace("NextID", p.NextID);
				dict_VerticeProp["Pos"] = nlohmann::ordered_json::array();
				dict_VerticeProp["Pos"].emplace_back(p.Pos.X);
				dict_VerticeProp["Pos"].emplace_back(p.Pos.Y);
			}
			return out_;
		}
	}

	template<>
	auto TerrainEditor::OutputData(nlohmann::ordered_json& output_) const -> void {
		output_["MapInfo"] = nlohmann::ordered_json::object();
		output_["MapInfo"]["Size"].emplace_back(CanvasSize_.X);
		output_["MapInfo"]["Size"].emplace_back(CanvasSize_.Y);
		output_ << Polygons_ << Ground_.Vertices;
	}

	template<>
	auto TerrainEditor::OutputData(TerrainShapeCollection& output_) const -> void {
		nlohmann::ordered_json intermediate0{};
		TerrainShapeCollection intermediate1{};
		OutputData(intermediate0);
		intermediate1.Initialize(intermediate0);
		intermediate1.ConvertToWorldCoordinate(output_, *Camera_, *Viewport_);
	}
}