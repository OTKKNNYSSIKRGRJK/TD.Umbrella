export module Game.CharacterTest;

import <memory>;
import <vector>;

import Lumina;
import Lumina.Sprite;

namespace Game {
	struct CharacterPartMaterial {
		// 色
		Lumina::F32x4 RGBA{ 1.0f, 1.0f, 1.0f, 1.0f };
		// 普通のテスクチャID
		Lumina::U32 ID_DiffuseMap;
		// 下の二つは無視していい
		Lumina::U32 ID_SpecularMap;
		Lumina::U32 ID_NormalMap;
	};


	struct CharacterPartBase {
		// ローカル座標変換
		Lumina::Math::F32x4x4<> Transform;
		int IsBox;
	};
	// ノードみたいなやつ？
	// 中身にパーツもしくはノード
	struct CharacterPartBox : public CharacterPartBase {
		std::vector<std::unique_ptr<CharacterPartBase>> Contents;
		CharacterPartBox() { IsBox = 1; }
	};
	// パーツ
	struct CharacterPart : public CharacterPartBase {
		CharacterPartMaterial Material;
		CharacterPart() { IsBox = 0; }
	};

	export class CharacterTest {
	public:
		constexpr Lumina::Math::F32x3 const& Position() const noexcept { return Position_; }
		constexpr Lumina::Math::F32x3 const& Velocity() const noexcept { return Velocity_; }

	public:
		void Update();
		void Render();

	public:
		void Initialize();

	private:
		Lumina::Math::F32x4x4<> View_;
		Lumina::Math::F32x4x4<> Projection_;
		Lumina::Math::F32x3 Position_;
		Lumina::Math::F32x3 Velocity_;

		Lumina::Math::F32x3 Scale_;
		Lumina::Math::F32x3 Rotate_;
		Lumina::Math::F32x3 Translate_;

		std::vector<Lumina::MeshShaderAsset> MeshShaderAssets_;

		Lumina::D3D12::GraphicsPSO GraphicsPSO_Mesh_;
		Lumina::D3D12::Shader VS_MeshDeferredGeometry_;
		Lumina::D3D12::Shader PS_MeshDeferredGeometry_;
		Lumina::D3D12::DescriptorTable GlobalTable_SRV_ImageTexture_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Scene_;

		CharacterPartBox Parts_;
		std::vector<std::unique_ptr<Lumina::D3D12::UploadBuffer>> UB_Materials_;
		Lumina::D3D12::DescriptorHeap LocalHeap_Materials_;
		Lumina::D3D12::UploadBuffer UB_WorldToNDC_;

		// パーツID
		enum PartName {
			MainBody = 0,
			LeftHand = 1,
			RightHand = 2,
			LeftLeg = 3,
			RightLeg = 4,
		};
	};
}