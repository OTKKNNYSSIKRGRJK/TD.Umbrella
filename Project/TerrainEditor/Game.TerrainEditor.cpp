module Game.TerrainEditor;

import <string>;

#if defined(_DEBUG)
import Lumina.Utils.ImGui;
#endif

namespace Game {
	#if defined(_DEBUG)
	namespace {
		auto operator<<(Lumina::Math::F32x2& dst_, ImVec2 const& src_) -> void {
			dst_.X = src_.x;
			dst_.Y = src_.y;
		}

		auto operator<<(ImVec2& dst_, Lumina::Math::F32x2 const& src_) -> void {
			dst_.x = src_.X;
			dst_.y = src_.Y;
		}

		auto IsInside(Lumina::Math::F32x3 const& p_, Polygon const& poly_) -> bool {
			if (poly_.Vertices.size() < 3) { return false; }

			bool ret{ false };

			auto isIntersected{
				[&] (
					Lumina::Math::F32x3 const& p0_,
					Lumina::Math::F32x3 const& p1_
				) constexpr -> bool {
					if (
						p_.Y > std::min<float>(p0_.Y, p1_.Y) &&
						p_.Y <= std::max<float>(p0_.Y, p1_.Y)
					) {
						if (p_.X <= std::max<float>(p0_.X, p1_.X)) {
							if (p0_.Y != p1_.Y) {
								float const inv_Slope{ (p1_.X - p0_.X) / (p1_.Y - p0_.Y) };
								//	X-coordinate of the point
								//	where the horizontal LINE and the segment should intersect;
								float const xRef{ (p_.Y - p0_.Y) * inv_Slope + p0_.X };
								//	The segment intersects with the RAY
								//	only when p.X <= xRef
								//	or the segment is vertical
								//	since we already have p.X <= std::max<float>(p0.X, p1.X).
								if ((p0_.X == p1_.X) || (p_.X <= xRef)) { return true; }
							}
						}
					}
					return false;
				}
			};

			for (int i{ 1 }; i < static_cast<int>(poly_.Vertices.size()); ++i) {
				auto const& p0{ poly_.Vertices.at(i - 1).Pos };
				auto const& p1{ poly_.Vertices.at(i).Pos };

				if (isIntersected(p0, p1)) { ret = !ret; }
			}

			auto const& p0{ poly_.Vertices.at(poly_.Vertices.size() - 1).Pos };
			auto const& p1{ poly_.Vertices.at(0).Pos };
			if (isIntersected(p0, p1)) { ret = !ret; }

			return ret;
		}
	}

	auto TerrainEditor::AddGroundVertices() -> void {
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			MouseScreenPos_ << ImGui::GetMousePos();
			MouseLocalPos_ = (MouseScreenPos_ - CanvasScreenPos_) / Zoom_;

			if (PreviousGroundPointID_ != -1) {
				if (Ground_.Vertices.at(PreviousGroundPointID_).Pos.X < MouseLocalPos_.X) {
					auto& newVertex{ Ground_.Vertices.emplace_back() };
					newVertex.Pos = MouseLocalPos_;
					newVertex.PrevID = PreviousGroundPointID_;
					newVertex.NextID = -1;
					Lumina::I32 const newGroundPointID{ static_cast<Lumina::I32>(&newVertex - Ground_.Vertices.data()) };
					newVertex.ID = newGroundPointID;
					Ground_.Vertices.at(PreviousGroundPointID_).NextID = newGroundPointID;
					PreviousGroundPointID_ = newGroundPointID;
				}
			}
			else {
				auto& newVertex{ Ground_.Vertices.emplace_back() };
				newVertex.Pos = MouseLocalPos_;
				PreviousGroundPointID_ = static_cast<Lumina::I32>(&newVertex - Ground_.Vertices.data());
				newVertex.ID = PreviousGroundPointID_;
				newVertex.PrevID = -1;
				newVertex.NextID = -1;
			}

			//Point const p1{ Lumina::Math::F32x2{ MouseLocalPos_.X, CanvasSize_.Y } };
		}
	}

	auto TerrainEditor::AddVertices() -> void {
		auto& currentPolygon{ Polygons_.at(CurrentPolygonID_) };

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			MouseScreenPos_ << ImGui::GetMousePos();
			MouseLocalPos_ = (MouseScreenPos_ - CanvasScreenPos_) / Zoom_;

			CurrentPolygonID_LastestUnused_ = -1;

			currentPolygon.Vertices.emplace_back(MouseLocalPos_);
			/*for (auto const& p : currentPolygon.Vertices) {
				Lumina::Debug::Logger::ConsolePrint("({}, {}) ", p.Pos.X, p.Pos.Y);
			}
			Lumina::Debug::Logger::ConsolePrint("\n");*/
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
			if (currentPolygon.Vertices.size() > 2) {
				if (CurrentPolygonID_LastestUnused_ == -1) {
					[[maybe_unused]] auto& newPolygon{ Polygons_.emplace_back() };
					CurrentPolygonID_ = static_cast<Lumina::I32>(&newPolygon - Polygons_.data());
					CurrentPolygonID_LastestUnused_ = CurrentPolygonID_;
				}
				else {
					CurrentPolygonID_ = CurrentPolygonID_LastestUnused_;
				}
			}
		}
	}

	auto TerrainEditor::EditGroundVertexMode() -> void {
		if (ImGui::IsItemHovered()) {
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				MouseScreenPos_ << ImGui::GetMousePos();
				MouseLocalPos_ = (MouseScreenPos_ - CanvasScreenPos_) / Zoom_;

				for (auto& p : Ground_.Vertices) {
					auto d{ MouseLocalPos_ - p.Pos };
					if (d.Dot(d) < 100.0f) {
						SelectedGroundPoint_ = &p;
						break;
					}
				}
			}

			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && SelectedGroundPoint_ != nullptr) {
				auto const dragDelta{ ImGui::GetMouseDragDelta(ImGuiMouseButton_Left) };
				SelectedGroundPoint_->Pos.X += dragDelta.x / Zoom_;
				auto const prevID{ SelectedGroundPoint_->PrevID };
				auto const nextID{ SelectedGroundPoint_->NextID };
				if (prevID != -1) {
					SelectedGroundPoint_->Pos.X = std::max<float>(
						Ground_.Vertices.at(prevID).Pos.X,
						SelectedGroundPoint_->Pos.X
					);
				}
				else {
					SelectedGroundPoint_->Pos.X = std::max<float>(
						0.0f,
						SelectedGroundPoint_->Pos.X
					);
				}
				if (nextID != -1) {
					SelectedGroundPoint_->Pos.X = std::min<float>(
						Ground_.Vertices.at(nextID).Pos.X,
						SelectedGroundPoint_->Pos.X
					);
				}
				else {
					SelectedGroundPoint_->Pos.X = std::min<float>(
						CanvasSize_.X,
						SelectedGroundPoint_->Pos.X
					);
				}
				SelectedGroundPoint_->Pos.Y += dragDelta.y / Zoom_;
				ImGui::ResetMouseDragDelta();
			}
		}
	}

	auto TerrainEditor::EditVertexMode() -> void {
		if (ImGui::IsItemHovered()) {
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				MouseScreenPos_ << ImGui::GetMousePos();
				MouseLocalPos_ = (MouseScreenPos_ - CanvasScreenPos_) / Zoom_;

				for (auto& polygon : Polygons_) {
					bool isSelected{ false };
					for (auto& p : polygon.Vertices) {
						auto d{ MouseLocalPos_ - p.Pos };
						if (d.Dot(d) < 100.0f) {
							SelectedPoint_ = &p;
							isSelected = true;
							break;
						}
					}

					if (isSelected) { break; }
				}
			}

			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && SelectedPoint_ != nullptr) {
				auto const dragDelta{ ImGui::GetMouseDragDelta(ImGuiMouseButton_Left) };
				SelectedPoint_->Pos.X += dragDelta.x / Zoom_;
				SelectedPoint_->Pos.Y += dragDelta.y / Zoom_;
				ImGui::ResetMouseDragDelta();
			}
		}
	}

	auto TerrainEditor::PolygonMode() -> void {
		if (ImGui::IsItemHovered()) {
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				MouseScreenPos_ << ImGui::GetMousePos();
				MouseLocalPos_ = (MouseScreenPos_ - CanvasScreenPos_) / Zoom_;

				for (auto& polygon : Polygons_) {
					if (IsInside(MouseLocalPos_, polygon)) {
						CurrentPolygonID_ = static_cast<Lumina::U32>(&polygon - Polygons_.data());
						break;
					}
				}
			}
			else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				CurrentPolygonID_ = CurrentPolygonID_LastestUnused_;
			}

			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && CurrentPolygonID_ != CurrentPolygonID_LastestUnused_) {
				auto const dragDelta{ ImGui::GetMouseDragDelta(ImGuiMouseButton_Left) };
				auto const dx{ dragDelta.x / Zoom_ };
				auto const dy{ dragDelta.y / Zoom_ };
				
				for (auto& vert : Polygons_[CurrentPolygonID_].Vertices) {
					vert.Pos.X += dx;
					vert.Pos.Y += dy;
				}
				ImGui::ResetMouseDragDelta();
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
			CurrentPolygonID_ = CurrentPolygonID_LastestUnused_;
		}
	}

	auto TerrainEditor::DrawCanvas() const -> void {
		ImDrawList* drawList{ ImGui::GetWindowDrawList() };
		drawList->AddRectFilled(
			ImVec2{ CanvasScreenPos_.X, CanvasScreenPos_.Y },
			ImVec2{ CanvasScreenPos_.X + CanvasSize_.X * Zoom_, CanvasScreenPos_.Y + CanvasSize_.Y * Zoom_ },
			0x0FFFFFFF
		);


		for (auto const& polygon : Polygons_) {

			bool const isPolygonSelected{ static_cast<Lumina::I32>(&polygon - Polygons_.data()) == CurrentPolygonID_ };
			Lumina::U32 const pointColor{ (isPolygonSelected) ? (0xFF1F3FFFU) : (0x7F1F7FFFU) };
			Lumina::U32 const segmentColor{ (isPolygonSelected) ? (0x3F1F3FFFU) : (0x1F1F7FFFU) };
			Lumina::U32 const fillColor{ (isPolygonSelected) ? (0x0F1F3FFFU) : (0x081F7FFFU) };

			std::vector<ImVec2> points{};
			for (auto const& p : polygon.Vertices) {
				points.emplace_back(
					CanvasScreenPos_.X + p.Pos.X * Zoom_,
					CanvasScreenPos_.Y + p.Pos.Y * Zoom_
				);
			}

			for (int i{ 0 }; i < static_cast<int>(points.size()); ++i) {
				auto const& p{ points.at(i) };
				drawList->AddCircleFilled(p, 3.0f, pointColor);
			}

			for (int i{ 1 }; i < static_cast<int>(points.size()); ++i) {
				auto const& p0{ points.at(i - 1) };
				auto const& p1{ points.at(i) };
				drawList->AddLine(p0, p1, segmentColor, 1.0f);
			}

			if (polygon.Vertices.size() > 2) {
				auto const& p0{ points.at(points.size() - 1) };
				auto const& p1{ points.at(0) };
				drawList->AddLine(p0, p1, segmentColor, 1.0f);

				drawList->AddConvexPolyFilled(points.data(), static_cast<int>(points.size()), fillColor);
			}
		}

		if (Ground_.Vertices.size() > 0) {
			auto const* p0{ &(Ground_.Vertices[0]) };
			for (size_t k = 1; k < Ground_.Vertices.size(); ++k) {
				auto const* p1{ &(Ground_.Vertices[k]) };

				std::vector<ImVec2> screenPoints{};
				screenPoints.emplace_back(
					CanvasScreenPos_.X + p0->Pos.X * Zoom_,
					CanvasScreenPos_.Y + p0->Pos.Y * Zoom_
				);
				screenPoints.emplace_back(
					CanvasScreenPos_.X + p1->Pos.X * Zoom_,
					CanvasScreenPos_.Y + p1->Pos.Y * Zoom_
				);
				screenPoints.emplace_back(
					CanvasScreenPos_.X + p1->Pos.X * Zoom_,
					CanvasScreenPos_.Y + CanvasSize_.Y * Zoom_
				);
				screenPoints.emplace_back(
					CanvasScreenPos_.X + p0->Pos.X * Zoom_,
					CanvasScreenPos_.Y + CanvasSize_.Y * Zoom_
				);

				for (int i{ 0 }; i < static_cast<int>(screenPoints.size()); ++i) {
					auto const& p{ screenPoints.at(i) };
					drawList->AddCircleFilled(p, 3.0f, 0xFF1FDFAFU);
				}

				for (int i{ 1 }; i < static_cast<int>(screenPoints.size()); ++i) {
					auto const& screenP0{ screenPoints.at(i - 1) };
					auto const& screenP1{ screenPoints.at(i) };
					drawList->AddLine(screenP0, screenP1, 0x7F1FDFAFU, 1.0f);
				}
				auto const& screenP0{ screenPoints.at(screenPoints.size() - 1) };
				auto const& screenP1{ screenPoints.at(0) };
				drawList->AddLine(screenP0, screenP1, 0x7F1FDFAFU, 1.0f);

				drawList->AddConvexPolyFilled(
					screenPoints.data(),
					static_cast<int>(screenPoints.size()),
					0x1F1F9F6FU
				);

				p0 = p1;
			}
		}

		if (SelectedPoint_ != nullptr) {
			ImVec2 const pos{
				CanvasScreenPos_.X + SelectedPoint_->Pos.X * Zoom_,
				CanvasScreenPos_.Y + SelectedPoint_->Pos.Y * Zoom_
			};
			drawList->AddCircleFilled(pos, 3.0f, 0xFF0008DFU);
		}
		if (SelectedGroundPoint_ != nullptr) {
			ImVec2 const pos{
				CanvasScreenPos_.X + SelectedGroundPoint_->Pos.X * Zoom_,
				CanvasScreenPos_.Y + SelectedGroundPoint_->Pos.Y * Zoom_
			};
			drawList->AddCircleFilled(pos, 3.0f, 0xFF0008DFU);
		}
	}

	auto TerrainEditor::Update() -> void {
		ImGui::Begin(
			"TerrainEditor",
			nullptr,
			ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoResize
		);
		ImGui::SetWindowSize(ImVec2{ 1280.0f * 0.9f, 720.0f * 0.9f });

		ImGui::BeginMenuBar();
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open")) {
				OpenFile();
			}
			if (ImGui::MenuItem("Save")) {
				SaveFile();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Build")) {
			if (ImGui::MenuItem("Build In-game Terrain")) {
				OutputData<TerrainShapeCollection>(*Shapes_);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();

		if (ImGui::BeginTabBar("EditMode")) {
			int const tabItemFlags_AddVertices{
				(CurrentPolygonID_LastestUnused_ == -1 && !IsEditingGround_) ?
				(ImGuiTabItemFlags_SetSelected) :
				(ImGuiTabItemFlags_None)
			};
			if (ImGui::BeginTabItem("Add Ground Vertices")) {
				CurrentEditMethod_ = &TerrainEditor::AddGroundVertices;
				IsEditingGround_ = 1;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Edit Ground Vertices")) {
				CurrentEditMethod_ = &TerrainEditor::EditGroundVertexMode;
				IsEditingGround_ = 1;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Add Vertices", nullptr, tabItemFlags_AddVertices)) {
				CurrentEditMethod_ = &TerrainEditor::AddVertices;
				IsEditingGround_ = 0;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Edit Vertices")) {
				CurrentEditMethod_ = &TerrainEditor::EditVertexMode;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Polygon Mode")) {
				CurrentEditMethod_ = &TerrainEditor::PolygonMode;
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::BeginChild(
			"Editor.Canvas",
			ImVec2{ ImGui::GetContentRegionAvail().x, 720.0f * 0.5f },
			ImGuiChildFlags_Borders,
			ImGuiWindowFlags_HorizontalScrollbar
		);

		CanvasSize_MIN_ << ImGui::GetContentRegionAvail();

		CanvasScreenPos_ << ImGui::GetCursorScreenPos();
		[[maybe_unused]] ImVec2 const canvasScroll{ ImGui::GetScrollX(), ImGui::GetScrollY() };

		//	Space for mouse input
		ImGui::InvisibleButton("Canvas", { CanvasSize_.X * Zoom_, CanvasSize_.Y * Zoom_ });

		(this->*CurrentEditMethod_)();

		DrawCanvas();

		ImGui::EndChild();

		static float canvasSizeY = CanvasSize_.Y;
		if (ImGui::DragFloat2("Map size (in screen coordinate)", &CanvasSize_.X, 1.0f, 0.0f)) {
			CanvasSize_.X = std::max<float>(CanvasSize_MIN_.X, CanvasSize_.X);
			CanvasSize_.Y = std::max<float>(CanvasSize_MIN_.Y, CanvasSize_.Y);
			if (canvasSizeY != CanvasSize_.Y) {
				auto const diffY{ canvasSizeY - CanvasSize_.Y };
				for (auto& polygon : Polygons_) {
					for (auto& vert : polygon.Vertices) {
						vert.Pos.Y -= diffY;
					}
				}
				for (auto& groundVert : Ground_.Vertices) {
					groundVert.Pos.Y -= diffY;
				}
			}
		}
		canvasSizeY = CanvasSize_.Y;
		ImGui::DragFloat("Zoom", &Zoom_, 0.01f, 0.1f);
		Zoom_ = std::clamp<float>(Zoom_, 0.1f, 5.0f);

		ImGui::Text("CurrentPolygonID = %d", CurrentPolygonID_);
		ImGui::Text("CurrentPolygonID_LastestUnused = %d", CurrentPolygonID_LastestUnused_);

		ImGui::DragFloat2("Ground offset (in screen coordinate)", &GroundOffset_.X, 1.0f, 0.0f);
		
		ImGui::End();
	}
	#endif

	auto TerrainEditor::Reset() -> void {
		for (auto& polygon : Polygons_) {
			polygon.Vertices.clear();
		}
		Polygons_.clear();

		Ground_.Vertices.clear();
	}

	auto TerrainEditor::Initialize() -> void {
		[[maybe_unused]] auto& polygon{ Polygons_.emplace_back() };
		CurrentPolygonID_ = 0;
		CurrentPolygonID_LastestUnused_ = 0;

		PreviousGroundPointID_ = -1;

		IsEditingGround_ = 1;

		#if defined(_DEBUG)
		CurrentEditMethod_ = &TerrainEditor::AddGroundVertices;
		#endif

		CanvasSize_ = { 1.0f, 1.0f };

		SelectedPoint_ = nullptr;
		SelectedGroundPoint_ = nullptr;

		Zoom_ = 1.0f;

		GroundOffset_ = { 0.0f, 0.0f };

		Camera_ = std::make_unique<Lumina::Utils::Camera>();
	}
}