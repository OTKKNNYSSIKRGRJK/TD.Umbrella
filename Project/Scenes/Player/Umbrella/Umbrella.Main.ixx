export module Game.Umbrella : Main;

import : Common;
import : State;
import Collider;

import Game.Attachment;
import StatusComponent;
import ManaComponent;

import Lumina.Core.Math;
import Lumina.MeshManager;
import Lumina.D3D12;

namespace{
	using Vector3 = Lumina::Math::F32x3;
}

//////////////////////
/// 
///  傘
/// 
//////////////////////

namespace Umbrella {

    //////////////////////
    /// 
    ///  傘の[持ち手]
    /// 
    //////////////////////
    export class Handle {
        //////////////////////
        /// 
        ///  コンストラクタ・デストラクタ
        /// 
        //////////////////////
    public:
        // コンストラクタ
        Handle() = default;
        // デストラクタ
        ~Handle() = default;

        //////////////////////
        /// 
        ///  基本的な関数
        /// 
        //////////////////////
    public:
        // 初期化処理
        void Initialize();
        // 更新処理
        void Update(float deltaTime);
        // 描画処理
        void Draw();

        //////////////////////
        /// 
        ///  Joint関係
        /// 
        //////////////////////
    public:
        Attachment* GetTipJoint() { return &tipJoint_; }
        Attachment* GetBaseJoint() { return &baseJoint_; }
        void SetMesh(Lumina::MeshShaderAsset const& mesh_) noexcept { Mesh_ = &mesh_; }
        void SetMeshMaterialCBV(D3D12_CPU_DESCRIPTOR_HANDLE cbv_) noexcept { MeshMaterialCBV_ = cbv_; }
    private:
        // プレイヤーに持たれる用のJoint
        Attachment baseJoint_;

        // 「かさ」をくっつけるための先端のJoint(他にもおｋでいいかも)
        Attachment tipJoint_;

        Lumina::MeshShaderAsset const* Mesh_;
        D3D12_CPU_DESCRIPTOR_HANDLE MeshMaterialCBV_;
    };

    //////////////////////
    /// 
    ///  傘の[かさ]
    /// 
    //////////////////////
    export class Top {
        //////////////////////
        /// 
        ///  コンストラクタ・デストラクタ
        /// 
        //////////////////////
    public:
        // コンストラクタ
        Top() = default;
        // デストラクタ
        ~Top() = default;

        //////////////////////
        /// 
        ///  基本的な関数
        /// 
        //////////////////////
    public:
        // 初期化処理
        void Initialize();
        // 更新処理
        void Update(float deltaTime);
        // 描画処理
        void Draw();

        //////////////////////
        /// 
        ///  Joint関係
        /// 
        //////////////////////
    public:
        Attachment* GetRootJoint() { return &rootJoint_; }
        void SetAttachment(Attachment* attachment){
            if(attachment){
                rootJoint_.AttachTo(attachment);
            }
		}
    private:
        // 持ち手、壁、敵にくっつくためのJoint
        Attachment rootJoint_;

        //////////////////////
        /// 
        ///  当たり判定
        /// 
        //////////////////////
    public:
        void UpdateColliderShape();
        ConvexCollider* GetCollider() const { return collider_.get(); }
        // 攻撃判定のON/OFF（属性の切り替え）
        void EnableAttackCollision() { collider_->SetMyType(COL_Player_Attack); }
        void DisableAttackCollision() { collider_->SetMyType(COL_None); }
    private:
        std::unique_ptr<ConvexCollider>collider_;

        //////////////////////
        /// 
        ///  パラメータ
        /// 
        //////////////////////
    public:
        // 壊れているかどうかの判定（HPが0以下なら壊れている）
        bool IsBroken() const { return status_->IsDead(); }
        void Repair() {
            // MaxHP分の回復値を渡すことで全回復させる(だんだんはHealを呼び出す)
            status_->Heal(status_->GetMaxHp());
        }
    private:
        std::unique_ptr<StatusComponent>status_;// 耐久度・攻撃力
        std::unique_ptr<ManaComponent>mana_;// マナ回収用

    public:
        StatusComponent& GetStatusComponent() { return *status_; }
        ManaComponent& GetManaComponent() { return *mana_; }

        //////////////////////
        /// 
        ///  State
        /// 
        //////////////////////
    public:
        void ChangeForm(UmbrellaForm newForm) { form_ = newForm; }
        void ChangeState(UmbrellaStates::Base* newState);
    private:
        // 状態のState
        UmbrellaForm form_ = UmbrellaForm::Closed;
        bool isBroken_ = false;

        // 現在のステート
        UmbrellaStates::Base* currentState_;

    public:// Get・Set関係の関数
        // 傘の「かさ」の状態を返す関数
        UmbrellaForm GetUmbrellaForm()const { return form_; }

        //////////////////////
        /// 
        ///  Playerに関して
        /// 
        //////////////////////
    public:
		void SetPlayerPos(const Vector3& pos) { playerPos_ = pos; }
		Vector3 GetPlayerPos() const { return playerPos_; }
		void StartRecall() { isRecalling_ = true; }
		bool IsRecalling() const { return isRecalling_; }
    private:
        Vector3 playerPos_;
		bool isRecalling_ = false;// プレイヤーが呼び戻し中かどうか

        //////////////////////
        /// 
        ///  基本的な情報
        /// 
        //////////////////////
    public:
        void SetMesh(Lumina::MeshShaderAsset const& mesh_) noexcept { Mesh_ = &mesh_; }
        void SetMeshOpen(Lumina::MeshShaderAsset const& mesh_) noexcept { MeshOpen_ = &mesh_; }
        void SetMeshMaterialCBV(D3D12_CPU_DESCRIPTOR_HANDLE cbv_) noexcept { MeshMaterialCBV_ = cbv_; }
    private:
        Lumina::MeshShaderAsset const* Mesh_;
        Lumina::MeshShaderAsset const* MeshOpen_;
        D3D12_CPU_DESCRIPTOR_HANDLE MeshMaterialCBV_;

    };

    //////////////////////
    /// 
    ///  傘の[全部]
    /// 
    //////////////////////
    export class Main {
        //////////////////////
        /// 
        ///  コンストラクタ・デストラクタ
        /// 
        //////////////////////
    public:
        // コンストラクタ
        Main() = default;
        // デストラクタ
        ~Main() = default;

        //////////////////////
        ///
        ///  基本的な関数
        ///
        //////////////////////
    public:
        // 初期化処理
        void Initialize();
        // 更新処理
        void Update(float deltaTime);
        // 描画処理
        void Draw();

    public:

        //////////////////////
        ///
        ///  基本的な情報
        ///
        //////////////////////
    public:

    public:
        std::unique_ptr<Top>top_;
        std::unique_ptr<Handle>handle_;
    };
}