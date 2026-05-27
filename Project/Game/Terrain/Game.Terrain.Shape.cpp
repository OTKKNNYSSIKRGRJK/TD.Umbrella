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
			std::vector<Polygon>& polygons_
		) -> JSON const& {
			auto const& arr_Polygons{ in_.at("Polygons") };
			polygons_.clear();
			for (auto const& dict_PolygonAttrs : arr_Polygons) {
				auto& polygon{ polygons_.emplace_back() };

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
			ground_.Vertices.clear();
			for (auto const& dict_VertexAttrs : arr_GroundPoints) {
				auto& vert{ ground_.Vertices.emplace_back() };

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
	namespace {
		struct ChainVertex {
			int ID;
			int PrevID;
			int NextID;
		};

		/// ### Notes
		/// A closed and simple polygonal curve as input is assumed.
		/// ### References
		/// 1. https://www.cs.umd.edu/class/spring2020/cmsc754/Lects/lect05-triangulate.pdf
		/// 2. https://dev.to/nail_sharipov_5d810d8cf71/monotone-triangulation-practical-advice-1k4j
		auto DivideSimplePolygon(
			[[maybe_unused]] std::vector<Lumina::Math::F32x3>& out_,
			[[maybe_unused]] std::vector<Polygon::Vertex> const& in_
		) -> void {
			/*out_.clear();
			if (in_.size() < 3) { return; }
			using Vec3 = Lumina::Math::F32x3;
			Vec3 const& pos0{ in_[0].Pos };
			Vec3 const& pos1{ in_[1].Pos };
			Vec3 edge01{ pos1 - pos0 };

			Lumina::F32 sum_CrossProd{ 0.0f };

			for (size_t idx{ 2 }; idx < in_.size(); ++idx) {
				Vec3 const& pos2{ in_[idx].Pos };
				Vec3 const edge02{ pos2 - pos0 };
				Vec3 const crossProd_E01_E02{ Vec3::Cross(edge01, edge02) };
				sum_CrossProd += crossProd_E01_E02.Z;

				edge01 = edge02;
			}*/

			//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//
			//	::	Step 1. Monotone Subdivision							::	//
			//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//

			

			//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//
			//	::	Step 2. Triangulation of Monotones						::	//
			//	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	::	//
		}
	}

	template<>
	void TerrainShapeCollection::Initialize(JSON const& serialized_) {
		(Polygons_.size() == 0) ||
		Lumina::Debug::ThrowIfFalse{ "Polygons should be uninitialized!" };

		(Ground_.Vertices.size() == 0) ||
		Lumina::Debug::ThrowIfFalse{ "Ground should be uninitialized!" };

		serialized_ >> Polygons_ >> Ground_;
	}
	template<>
	void TerrainShapeCollection::Initialize(nlohmann::ordered_json const& serialized_) {
		(Polygons_.size() == 0) ||
			Lumina::Debug::ThrowIfFalse{ "Polygons should be uninitialized!" };

		(Ground_.Vertices.size() == 0) ||
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

		//Lumina::F32 const inv_ViewportWidth{ 1.0f / viewport_.Width };
		Lumina::F32 const inv_ViewportWidth{ 1.0f / (1280.0f * 0.25f) };
		Lumina::F32 const inv_ViewportHeight{ 1.0f / (720.0f * 0.25f) };
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

		out_.Polygons_.clear();
		out_.Ground_.Vertices.clear();
		out_.Ground_.Colliders.clear();

		for (auto const& polygon : Polygons_) {
			auto& retPolygon{ out_.Polygons_.emplace_back() };

			for (auto const& vert : polygon.Vertices) {
				auto&& ndcPos{ screenToNDC(Lumina::Math::F32x3{ vert.Pos.X, vert.Pos.Y, tmp.Z() }) };
				auto&& worldPos{ ndcPos * ndcToWorld };
				worldPos /= worldPos.W();
				auto& retVert{ retPolygon.Vertices.emplace_back() };
				retVert.Pos = Lumina::Math::F32x3{ worldPos.X(), worldPos.Y(), worldPos.Z() };
			}
		}

		for (auto const& groundVert : Ground_.Vertices) {
			auto& retGroundVert{ out_.Ground_.Vertices.emplace_back() };

			auto&& ndcPos{ screenToNDC(Lumina::Math::F32x3{ groundVert.Pos.X, groundVert.Pos.Y, tmp.Z() }) };
			auto&& worldPos{ ndcPos * ndcToWorld };
			worldPos /= worldPos.W();
			retGroundVert.Pos = Lumina::Math::F32x3{ worldPos.X(), worldPos.Y(), 0.0f };
		}

		// ジェネラルポリゴンコライダー生成
		for (auto& outPolygon : out_.Polygons_) {
			outPolygon.Col = std::make_unique<ConvexCollider>();
			std::vector<Lumina::Math::F32x3> outVertPoses{};
			for (auto const& outVert : outPolygon.Vertices) {
				outVertPoses.emplace_back(outVert.Pos.X, outVert.Pos.Y, 0.0f);
			}
			outPolygon.Col->SetVertices(outVertPoses);

			outPolygon.Col->SetMyType(COL_Ground);
			outPolygon.Col->SetYourType(COL_Player | COL_Enemy | COL_Umbrella_Ground);
			outPolygon.Col->SetWorldPosition({ 0.0f, 0.0f, 0.0f });

			outPolygon.Col->UpdateAABB();
		}

		// 地面コライダー生成
		auto const* retGroundVert0{ &(out_.Ground_.Vertices[0]) };
		for (size_t i = 1; i < out_.Ground_.Vertices.size(); ++i) {
			auto const* retGroundVert1{ &(out_.Ground_.Vertices[i]) };

			auto& collider{ out_.Ground_.Colliders.emplace_back() };
			collider = std::make_unique<ConvexCollider>();
			collider->SetVertices(
				{
					retGroundVert0->Pos,
					retGroundVert1->Pos,
					//	便宜上ｙ座標を一旦適当なマイナスナンバーにする
					{ retGroundVert1->Pos.X, -100.0f, retGroundVert1->Pos.Z },
					{ retGroundVert0->Pos.X, -100.0f, retGroundVert0->Pos.Z }
				}
			);
			collider->SetMyType(COL_Ground);
			collider->SetYourType(COL_Player | COL_Enemy | COL_Umbrella_Ground);
			collider->SetWorldPosition({ 0.0f, 0.0f, 0.0f });

			collider->UpdateAABB();

			retGroundVert0 = retGroundVert1;
		}
	}

	TerrainShapeCollection::TerrainShapeCollection() {}
	TerrainShapeCollection::~TerrainShapeCollection() {}
}