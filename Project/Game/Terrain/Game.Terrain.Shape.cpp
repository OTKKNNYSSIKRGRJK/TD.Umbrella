module Game.Terrain;

import <string>;

import Lumina.Core.Math;
import Lumina.Core.Debug;

import : Shape;

import nlohmann.json;

namespace {
	using JSON = nlohmann::json;
}

namespace Game{
	namespace {
		auto operator>>(
			JSON const& in_,
			Lumina::List<Polygon>& polygons_
		) -> JSON const& {
			auto const& arr_Polygons{ in_.at("Polygons") };
			polygons_.Initialize(static_cast<Lumina::U32>(arr_Polygons.size()));
			for (auto const& dict_PolygonAttrs : arr_Polygons) {
				auto& polygon{ polygons_.New() };

				auto const& arr_Vertices{ dict_PolygonAttrs.at("Vertices") };
				for (auto const& dict_VerticeAttrs : arr_Vertices) {
					auto& vert{ polygon.Vertices.emplace_back() };

					auto const& arr_Pos{ dict_VerticeAttrs.at("Pos") };
					vert.Pos.X = arr_Pos.at(0).get<Lumina::F32>();
					vert.Pos.Y = arr_Pos.at(1).get<Lumina::F32>();
					vert.Pos.Z = 0.0f;
				}
			}

			return in_;
		}

		auto operator>>(
			nlohmann::json const& in_,
			Ground& ground_
		) -> nlohmann::json const& {
			auto const& arr_GroundPoints{ in_.at("GroundPoints") };
			ground_.Vertices.Initialize(static_cast<Lumina::U32>(arr_GroundPoints.size()));
			for (auto const& dict_VertexAttrs : arr_GroundPoints) {
				auto& vert{ ground_.Vertices.New() };

				vert.ID = dict_VertexAttrs.at("ID").get<Lumina::I32>();
				vert.PrevID = dict_VertexAttrs.at("PrevID").get<Lumina::I32>();
				vert.NextID = dict_VertexAttrs.at("NextID").get<Lumina::I32>();
				auto const& arr_Pos{ dict_VertexAttrs.at("Pos") };
				vert.Pos.X = arr_Pos.at(0).get<Lumina::F32>();
				vert.Pos.Y = arr_Pos.at(1).get<Lumina::F32>();
				vert.Pos.Z = 0.0f;
			}

			return in_;
		}
	}
}

namespace Game {
	template<>
	void TerrainShapeCollection::Initialize(JSON const& serialized_) {
		(Polygons_.Size() == 0) ||
		Lumina::Debug::ThrowIfFalse{ "Polygons should be uninitialized!" };

		(Ground_.Vertices.Size() == 0) ||
		Lumina::Debug::ThrowIfFalse{ "Ground should be uninitialized!" };

		serialized_ >> Polygons_ >> Ground_;
	}
	template<>
	void TerrainShapeCollection::Initialize(nlohmann::ordered_json const& serialized_) {
		(Polygons_.Size() == 0) ||
			Lumina::Debug::ThrowIfFalse{ "Polygons should be uninitialized!" };

		(Ground_.Vertices.Size() == 0) ||
			Lumina::Debug::ThrowIfFalse{ "Ground should be uninitialized!" };

		serialized_ >> Polygons_ >> Ground_;
	}

	auto TerrainShapeCollection::ConvertToWorldCoordinate(
		TerrainShapeCollection& out_,
		Lumina::Utils::Camera const& camera_,
		Lumina::Utils::Viewport const& viewport_
	) const -> void {
		//	We want to know the depth in screen coordinate of the world origin (0, 0, 0).
		auto const worldToHomogeneous{ camera_.View() * camera_.Projection() };
		auto tmp{ Lumina::Math::F32x4{ 0.0f, 0.0f, 0.0f, 1.0f } * worldToHomogeneous };
		tmp /= tmp.W();
		tmp.Z(viewport_.MinDepth + tmp.Z() * (viewport_.MaxDepth - viewport_.MinDepth));

		Lumina::F32 const inv_ViewportWidth{ 1.0f / viewport_.Width };
		Lumina::F32 const inv_ViewportHeight{ 1.0f / viewport_.Height };
		Lumina::F32 const inv_ViewportDepthDiff{ 1.0f / (viewport_.MaxDepth - viewport_.MinDepth) };
		auto screenToNDC{
			[&] (Lumina::Math::F32x3 const& screenPos_) noexcept -> Lumina::Math::F32x4 {
				return {
					((screenPos_.X - viewport_.TopLeftX) * inv_ViewportWidth) * 2.0f - 1.0f,
					1.0f - ((screenPos_.Y - viewport_.TopLeftY) * inv_ViewportHeight) * 2.0f,
					(screenPos_.Z - viewport_.MinDepth) * inv_ViewportDepthDiff,
					1.0f
				};
			}
		};
		
		auto const& inv_View{ camera_.ViewInverse() };
		auto const inv_Proj{ camera_.Projection().Inverse() };
		auto const ndcToWorld{ inv_Proj * inv_View };

		out_.Polygons_.Initialize(Polygons_.Size());
		out_.Ground_.Vertices.Initialize(Ground_.Vertices.Size());
		out_.Ground_.Colliders.clear();

		Lumina::List<Polygon>::Iterator it{ Polygons_ };
		for (it.Begin(); !it.End(); it.Next()) {
			auto const& polygon{ *it };
			auto& retPolygon{ out_.Polygons_.New() };

			for (auto const& vert : polygon.Vertices) {
				auto&& ndcPos{ screenToNDC(Lumina::Math::F32x3{ vert.Pos.X, vert.Pos.Y, tmp.Z() }) };
				auto&& worldPos{ ndcPos * ndcToWorld };
				worldPos /= worldPos.W();
				auto& retVert{ retPolygon.Vertices.emplace_back() };
				retVert.Pos = Lumina::Math::F32x3{ worldPos.X(), worldPos.Y(), worldPos.Z() };
			}
		}

		Lumina::List<Ground::Vertex>::Iterator it_GroundVert{ Ground_.Vertices };
		for (it_GroundVert.Begin(); !it_GroundVert.End(); it_GroundVert.Next()) {
			auto const& groundVert{ *it_GroundVert };
			auto& retGroundVert{ out_.Ground_.Vertices.New() };

			auto&& ndcPos{ screenToNDC(Lumina::Math::F32x3{ groundVert.Pos.X, groundVert.Pos.Y, tmp.Z() }) };
			auto&& worldPos{ ndcPos * ndcToWorld };
			worldPos /= worldPos.W();
			retGroundVert.Pos = Lumina::Math::F32x3{ worldPos.X(), worldPos.Y(), worldPos.Z() };
		}

		Lumina::List<Ground::Vertex>::Iterator it_RetGroundVert{ out_.Ground_.Vertices };
		it_RetGroundVert.Begin();
		auto const* retGroundVert0{ &(*it_RetGroundVert) };
		for (it_RetGroundVert.Next(); !it_RetGroundVert.End(); it_RetGroundVert.Next()) {
			auto const* retGroundVert1{ &(*it_RetGroundVert) };

			auto& collider{ out_.Ground_.Colliders.emplace_back() };
			collider = std::make_unique<ConvexCollider>();
			collider->SetVertices(
				{
					retGroundVert0->Pos,
					retGroundVert1->Pos,
					//	便宜上ｙ座標を一旦適当なマイナスナンバーにする
					{ retGroundVert1->Pos.X, -10.0f, retGroundVert1->Pos.Z },
					{ retGroundVert0->Pos.X, -10.0f, retGroundVert0->Pos.Z }
				}
			);
			collider->UpdateAABB();

			retGroundVert0 = retGroundVert1;
		}
	}

	TerrainShapeCollection::TerrainShapeCollection() {}
	TerrainShapeCollection::~TerrainShapeCollection() {}
}