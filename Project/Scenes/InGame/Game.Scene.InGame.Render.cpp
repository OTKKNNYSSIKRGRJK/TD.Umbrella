module Game.Scene.InGame;

import <vector>;

import nlohmann.json;

import Lumina;

import Lumina.Utils.Data;
import Lumina.Utils.ImGui;

import : Impl;

#if defined(_DEBUG)
using namespace ImGui;
#endif

namespace Game::Scene::Impl {
	namespace {
#if defined(_DEBUG)
		// デバッグ用に表示するサンプル値（例: 現在フレームカウンタ）
		inline int g_DebugFrameCount = 0;
#endif
	}

	void InGame::Render() {
#if defined(_DEBUG)
		// 数値確認用の簡単なウィンドウのみ描画する
		Begin("InGame Debug");
		Text("Frame: %d", g_DebugFrameCount);
		End();

		++g_DebugFrameCount;
#endif

		// TODO: 通常の描画処理をここに追加
	}
}

namespace Game::Scene {
	void InGame::Render() {
		// ImGui の BeginFrame/EndFrame はエンジン側の共通レンダーループで
		// 1 フレームに 1 回だけ呼ぶ前提にし、ここでは触らない。
		Impl_->Render();
	}
}