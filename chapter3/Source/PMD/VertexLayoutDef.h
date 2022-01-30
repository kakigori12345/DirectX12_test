#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include <d3d12.h>



// 頂点レイアウト
const D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[] = {
	{	// 座標
		"POSITION",		// セマンティクス名
		0,				// 同じセマンティクス名の時に使うインデックス
		DXGI_FORMAT_R32G32B32_FLOAT,	// フォーマット（要素数とビット数で型を表す）
		0,								// 入力スロットインデックス
		D3D12_APPEND_ALIGNED_ELEMENT,	// データのオフセット位置
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	// 
		0				// 一度に描画するインスタンスの数
	},
	{
		// 法線
		"NORMAL",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
	{	// uv
		"TEXCOORD",
		0,
		DXGI_FORMAT_R32G32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
	{
		// ボーン番号
		"BONE_NO",
		0,
		DXGI_FORMAT_R16G16_UINT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
	{
		// ウェイト
		"WEIGHT",
		0,
		DXGI_FORMAT_R8_UINT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
	{
		// 輪郭線フラグ
		"EDGE_FLG",
		0,
		DXGI_FORMAT_R8_UINT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
};