export module Game.UIMenu;

import <vector>;
import <cmath>;

import Lumina.Core.Common;
import Lumina.Core.Math;
import Lumina.Primitive;
import Lumina.Main;
import Lumina.OS.Windows.RawInput;

namespace Game {
	/// 汎用メニューUI（PrimitiveManagerベース・テキストなし）
	/// パッド十字キー / キーボード(W/S, ↑/↓)で項目選択、A/Enterで決定
	export class UIMenu {
	public:
		struct Color4 {
			float R, G, B, A;
		};

		struct MenuItem {
			Color4 Color;         // 通常時の色
			Color4 SelectedColor; // 選択時の色
		};

		/// メニューを構成する
		void Setup(
			std::vector<MenuItem> items_,
			Color4 titleColor_ = { 0.9f, 0.2f, 0.2f, 0.9f }
		) {
			Items_ = std::move(items_);
			TitleColor_ = titleColor_;
			SelectedIndex_ = 0;
			DecidedIndex_ = -1;
			IsVisible_ = false;
			PrevUpHeld_ = false;
			PrevDownHeld_ = false;
			PrevDecideHeld_ = false;
			PulseTimer_ = 0.0f;
		}

		/// メニューを表示開始
		void Show() {
			IsVisible_ = true;
			SelectedIndex_ = 0;
			DecidedIndex_ = -1;
			PulseTimer_ = 0.0f;
		}

		/// メニューを非表示
		void Hide() {
			IsVisible_ = false;
			DecidedIndex_ = -1;
		}

		/// 毎フレーム呼び出し。決定されたインデックスを返す（未決定なら -1）
		int Update(float deltaTime_) {
			if (!IsVisible_ || Items_.empty()) return -1;

			DecidedIndex_ = -1;
			PulseTimer_ += deltaTime_;

			auto const& inputMngr{ Lumina::Context::Instance().RawInputContext() };
			auto const& keyboard{ inputMngr.Keyboard() };
			auto const& pad{ inputMngr.Pad() };
			using Lumina::OS::Windows::KEY;

			// --- 上下移動 ---
			bool upHeld = keyboard.IsPressed(KEY::W) || keyboard.IsPressed(KEY::ARROW_UP) || pad.IsHold(0x0001);
			bool downHeld = keyboard.IsPressed(KEY::S) || keyboard.IsPressed(KEY::ARROW_DOWN) || pad.IsHold(0x0002);

			bool upJust = upHeld && !PrevUpHeld_;
			bool downJust = downHeld && !PrevDownHeld_;
			PrevUpHeld_ = upHeld;
			PrevDownHeld_ = downHeld;

			if (upJust) {
				SelectedIndex_ = (SelectedIndex_ - 1 + static_cast<int>(Items_.size())) % static_cast<int>(Items_.size());
			}
			if (downJust) {
				SelectedIndex_ = (SelectedIndex_ + 1) % static_cast<int>(Items_.size());
			}

			// --- 決定 ---
			bool decideHeld = keyboard.IsPressed(KEY::ENTER) || keyboard.IsPressed(KEY::SPACE) || pad.IsHold(0x1000);
			bool decideJust = decideHeld && !PrevDecideHeld_;
			PrevDecideHeld_ = decideHeld;

			if (decideJust) {
				DecidedIndex_ = SelectedIndex_;
			}

			return DecidedIndex_;
		}

		/// PrimitiveManagerに矩形をバッチする
		void Render(Lumina::PrimitiveManager& primMngr_) const {
			if (!IsVisible_ || Items_.empty()) return;

			int const itemCount = static_cast<int>(Items_.size());

			// --- レイアウト定数 (NDC空間: -1 ~ +1) ---
			float const menuWidth = 0.5f;
			float const itemHeight = 0.06f;
			float const itemGap = 0.03f;
			float const titleHeight = 0.08f;
			float const titleGap = 0.05f;

			float const totalHeight = titleHeight + titleGap
				+ itemCount * itemHeight + (itemCount - 1) * itemGap;
			float const menuTop = totalHeight * 0.5f;

			// --- 半透明オーバーレイ ---
			BatchQuad(primMngr_, -1.0f, 1.0f, 1.0f, -1.0f, { 0.0f, 0.0f, 0.0f, 0.5f });

			// --- タイトル矩形 ---
			BatchQuad(primMngr_, -menuWidth, menuTop, menuWidth, menuTop - titleHeight, ToF32x4(TitleColor_));

			// --- メニュー項目矩形 ---
			float y_cursor = menuTop - titleHeight - titleGap;
			float pulse = std::sin(PulseTimer_ * 4.0f) * 0.5f + 0.5f;

			for (int i = 0; i < itemCount; ++i) {
				float y_top = y_cursor;
				float y_bottom = y_cursor - itemHeight;

				Lumina::F32x4 color;
				if (i == SelectedIndex_) {
					auto const& sc = Items_[i].SelectedColor;
					float br = 0.8f + pulse * 0.2f;
					color = { sc.R * br, sc.G * br, sc.B * br, sc.A };
				}
				else {
					color = ToF32x4(Items_[i].Color);
				}

				BatchQuad(primMngr_, -menuWidth * 0.9f, y_top, menuWidth * 0.9f, y_bottom, color);
				y_cursor -= itemHeight + itemGap;
			}

			// --- 選択カーソル（左右の三角マーカー） ---
			{
				float y_sel_top = menuTop - titleHeight - titleGap - SelectedIndex_ * (itemHeight + itemGap);
				float y_mid = y_sel_top - itemHeight * 0.5f;
				float ms = 0.02f;
				float mo = menuWidth * 0.9f + 0.03f;
				Lumina::F32x4 mc{ 1.0f, 1.0f, 1.0f, 0.9f };

				// 左マーカー
				primMngr_.BatchTriangle(
					{ { -mo, y_mid + ms, 0.0f, 1.0f }, mc, {0.0f, 0.0f}, 0U },
					{ { -mo + ms, y_mid, 0.0f, 1.0f }, mc, {0.0f, 0.0f}, 0U },
					{ { -mo, y_mid - ms, 0.0f, 1.0f }, mc, {0.0f, 0.0f}, 0U }
				);
				// 右マーカー
				primMngr_.BatchTriangle(
					{ { mo, y_mid + ms, 0.0f, 1.0f }, mc, {0.0f, 0.0f}, 0U },
					{ { mo - ms, y_mid, 0.0f, 1.0f }, mc, {0.0f, 0.0f}, 0U },
					{ { mo, y_mid - ms, 0.0f, 1.0f }, mc, {0.0f, 0.0f}, 0U }
				);
			}
		}

		bool IsVisible() const noexcept { return IsVisible_; }
		int SelectedIndex() const noexcept { return SelectedIndex_; }

	private:
		static Lumina::F32x4 ToF32x4(Color4 const& c) {
			return { c.R, c.G, c.B, c.A };
		}

		static void BatchQuad(
			Lumina::PrimitiveManager& pm,
			float l, float t, float r, float b,
			Lumina::F32x4 const& c
		) {
			pm.BatchTriangle(
				{ { l, t, 0.0f, 1.0f }, c, {0.0f, 0.0f}, 0U },
				{ { r, t, 0.0f, 1.0f }, c, {0.0f, 0.0f}, 0U },
				{ { l, b, 0.0f, 1.0f }, c, {0.0f, 0.0f}, 0U }
			);
			pm.BatchTriangle(
				{ { r, t, 0.0f, 1.0f }, c, {0.0f, 0.0f}, 0U },
				{ { r, b, 0.0f, 1.0f }, c, {0.0f, 0.0f}, 0U },
				{ { l, b, 0.0f, 1.0f }, c, {0.0f, 0.0f}, 0U }
			);
		}

		std::vector<MenuItem> Items_;
		Color4 TitleColor_{ 0.9f, 0.2f, 0.2f, 0.9f };
		int SelectedIndex_{ 0 };
		int DecidedIndex_{ -1 };
		bool IsVisible_{ false };
		float PulseTimer_{ 0.0f };

		bool PrevUpHeld_{ false };
		bool PrevDownHeld_{ false };
		bool PrevDecideHeld_{ false };
	};
}
