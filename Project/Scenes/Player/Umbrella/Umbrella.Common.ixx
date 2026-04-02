export module Game.Umbrella : Common;

//////////////////////
/// 
///  傘の形状・機能
/// 
//////////////////////
export enum class UmbrellaForm {
	Closed,     // 閉じている（攻撃特化）
	Opened,     // 開いている（防御・マナ回収特化）
	Reverse,    // 逆さ（防御・壊れやすい）
	Flying,	   // かさが飛んでいる(ない)状態（壊れている状態など）
	AirStop,  // 空中で止まっている状態（攻撃特化、空中での移動も可能）
};

//////////////////////
/// 
///  傘
/// 
//////////////////////
namespace Umbrella {
	///  傘の[持ち手]
	export class Handle;
	///  傘の[かさ]
	export class Top;
	///  傘の[全部]
	export class Main;
}