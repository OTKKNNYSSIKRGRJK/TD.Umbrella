export module Game.Attachment;

import <memory>;

import Lumina.Core.Math;

namespace {
	using Vector3 = Lumina::Math::F32x3;
	using Matrix4x4 = Lumina::Math::F32x4x4<>;

	template<typename T>
	using UniPtr = std::unique_ptr<T>;
}

export enum AttachmentType : int {
	None = 0,
	PlayerBack = 1 << 0,
	PlayerHand = 1 << 1,
	UmbrellaHandle = 1 << 2,
	UmbrellaTip = 1 << 3,
	UmbrellaTopRoot = 1 << 4,
};

export class Attachment {
public:
	Attachment();
	~Attachment();
public:
	// 更新処理
	void Update();
	//////////////////////////
	///
	///   Jointの基本的な情報
	/// 
	//////////////////////////
public:
	// 情報のセット
	
	// 座標と回転のセット
	void SetInfo(const Vector3& pos, const Vector3& rot) { position_ = pos;rotation_ = rot; }
	void SetPos(const Vector3& pos) { position_ = pos; }
	void SetRot(const Vector3& rot) { rotation_ = rot; }
	Vector3 GetPos() { return position_; }
	// 取得
	Matrix4x4 const& GetMatrix() const noexcept { return *matWorld_; }
	Vector3 GetWorldPos() const noexcept {
		auto const& row3{ (*matWorld_)[3] };
		return {
			row3.Get(0),
			row3.Get(1),
			row3.Get(2),
		};
	}
private:
	Vector3 position_;// 座標
	Vector3 rotation_;// 回転
	//Vector3 scale_;※ Scaleは使わないのでいらない{1.0f,1.0f,1.0f}で固定
	UniPtr<Matrix4x4> matWorld_; // 最終的な座標・回転
	//////////////////////////
	///
	///   接続関係の情報
	/// 
	//////////////////////////
public:
	// くっつく
	void AttachTo(Attachment* parentJoint);
	// 離れる
	void Detach();
	// 接続情報の取得
	bool Attached() const { return attached_; }
	// タイプの登録
	void SetType(uint32_t type) { type_ = type; }
	// タイプの追加
	void AddType(uint32_t type) { type_ = type_ | type; }
	// タイプの登録
	void SetAcceptType(uint32_t type) { acceptType_ = type; }
	// タイプの追加
	void AddAcceptType(uint32_t type) { acceptType_ = type_ | type; }
	// タイプの取得
	uint32_t GetType() const { return type_; }
private:
	uint32_t type_ = AttachmentType::None;// Typeの情報
	uint32_t acceptType_;// 繋がっていい相手
	Attachment* parent_ = nullptr; // 繋がっている相手
	bool attached_ = false;// 接続しているかどうか
};