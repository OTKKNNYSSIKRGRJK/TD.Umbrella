export module AttackData;

import <string>;
import <vector>;
import <unordered_map>;
import <optional>;

export namespace AttackData {

    // 派生ルートの条件など
    struct AttackCondition {
        float minMana = 0.0f;
    };

    // 派生のルール
    struct AttackBranch {
        std::string type;         // "Input", "Auto", "OnGround" など
        std::string input;        // "Attack", "Evasion" など（Inputの時のみ使用）
        float timeMin = 0.0f;
        float timeMax = 999.0f;   // デフォルトは適当な大きい値

        AttackCondition condition;
        float consumeMana = 0.0f; // 派生時に消費するマナ

        std::string nextAttack;   // 次の攻撃ID
    };

    // 物理・移動データ
    struct AttackPhysics {
        float velocityX = 0.0f;
        float velocityY = 0.0f;
        float gravityScale = 1.0f;
    };

    // 演出データ
    struct AttackFeel {
        float hitStop = 0.0f;
        float cameraShake = 0.0f;
    };

    // 攻撃1つ分のメインデータ
    struct AttackData {
        std::string name;
        std::string motion;
        std::string animationName;
        float duration = 0.0f;
        float damage = 0.0f;
        float manaCost = 0.0f; // 攻撃そのものを出すのに必要なマナ

        AttackPhysics physics;
        AttackFeel feel;

        std::vector<AttackBranch> branches;// 色々な攻撃に派生できる
    };

    // 攻撃データベース全体
    using Database = std::unordered_map<std::string, AttackData>;

}