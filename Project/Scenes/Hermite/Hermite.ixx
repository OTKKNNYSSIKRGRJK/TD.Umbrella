export module Hermite;

import <vector>;

import nlohmann.json;

using json = nlohmann::json;

export namespace MathUtils {
    export namespace Spline {
        // Hermite補間
        template<typename T>
        T HermiteInterpolation(T p0, T m0, T p1, T m1, float t) {
            float t2 = t * t;
            float t3 = t2 * t;
            return (2.0f * t3 - 3.0f * t2 + 1.0f) * p0 + (t3 - 2.0f * t2 + t) * (m0 - p0) + (-2.0f * t3 + 3.0f * t2) * p1 + (t3 - t2) * (m1 - p1);
        }

        // points: 制御点の配列, tangents: 接線の配列, t: 全体の進行度(0.0 ~ 1.0) <- 進行度を渡すときにEasing関数を使うと、より面白くできそう
        template <typename T>
        T GetPointOnSpline(const std::vector<T>& points, const std::vector<T>& tangents, float t) {
            int pointCount = points.size();
            if (pointCount < 2) return points[0]; // 点が足りない場合のエラー回避

            // 区間の数 (点が4つなら区間は3つ)
            int segmentCount = pointCount - 1;

            // t(0.0~1.0) に区間数を掛けることで、「何番目の区間にいるか」を計算
            float scaledT = t * segmentCount;

            // 整数部分が「現在の区間のインデックス(i)」
            int i = (int)scaledT;

            // 小数部分が「その区間内での進行度(ローカルt)」
            float localT = scaledT - i;

            // 終点(t=1.0)に到達した時の特別処理 (配列外参照を防ぐ)
            if (i >= segmentCount) {
                i = segmentCount - 1;
                localT = 1.0f;
            }

            // その区間の開始点と終了点を取り出して、エルミート関数に投げる！
            return HermiteInterpolation(points[i], tangents[i], points[i + 1], tangents[i + 1], localT);
        }

        template<typename T>
        struct Node {
            T position;  // 通過する点の座標
            T TangentIn; // 入ってくるハンドルの座標
            T TangentOut;// 出ていくハンドルの座標

            bool isBroken = false;// 連動させるかどうか
            // コンストラクタ
            Node(T pos) : position(pos), TangentIn(pos), TangentOut(pos) {}
        };

        template<typename T>
        T GetPointSpline(const std::vector<Node<T>>& nodes, float t) {
            int pointCount = (int)nodes.size();
            // ノードがない場合のエラー回避
            if (pointCount == 0) return T();
            // ノードが1つの場合は、ノードの位置を返す
            if (pointCount == 1)return nodes[0].position;
            // 区間の数 (例）ノードが2つなら１つ
            int segmentCount = pointCount - 1;
            float scaledT = t * segmentCount;
            int i = (int)scaledT;
            float localT = scaledT - i;
            if (i >= segmentCount) {
                i = segmentCount - 1;
                localT = 1.0f;
            }
            return HermiteInterpolation(
                nodes[i].position,     // 今の点の位置
                nodes[i].TangentOut,   // 出ていくハンドル
                nodes[i + 1].position, // 次の点の位置
                nodes[i + 1].TangentIn,// 次の点に入るハンドル
                localT
            );
        }

        template<typename T>
        json SerializeNodes(const std::vector<Node<T>>& nodes) {
            json j = json::array();
            for (const auto& node : nodes) {
                j.push_back({
                    {"posX", node.position.X}, {"posY", node.position.Y}, {"posZ", node.position.Z},
                    {"inX", node.TangentIn.X}, {"inY", node.TangentIn.Y}, {"inZ", node.TangentIn.Z},
                    {"outX", node.TangentOut.X}, {"outY", node.TangentOut.Y}, {"outZ", node.TangentOut.Z}
                    });
            }
            return j;
        }

        template<typename T>
        std::vector<Node<T>> DeserializeNodes(const json& j) {
            std::vector<Node<T>> nodes;
            for (const auto& item : j) {
                Node<T> node(T{ item["posX"], item["posY"], item["posZ"] });
                node.TangentIn = T{ item["inX"], item["inY"], item["inZ"] };
                node.TangentOut = T{ item["outX"], item["outY"], item["outZ"] };
                node.isBroken = false;
                nodes.push_back(node);
            }
            return nodes;
        }
    }
}