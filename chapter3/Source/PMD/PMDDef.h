#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include <DirectXMath.h>


//-----------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------

// PMD ヘッダ構造体
struct PMDHeader {
	float version;
	char modelName[20];
	char comment[256];
};

// PMD 頂点構造体
struct PMDVertex {
	DirectX::XMFLOAT3 pos;		// 頂点情報(12)
	DirectX::XMFLOAT3 normal;	// 法線ベクトル(12)
	DirectX::XMFLOAT2 uv;		// uv 座標(8)
	unsigned short boneNo[2];	// ボーン番号(4)
	unsigned char boneWeigth;	// ボーン影響度(1)
	unsigned char edgeFlag;		// 輪郭線フラグ(1)
};
constexpr size_t pmdVertexSize = 38;	// 頂点一つ当たりのサイズ

#pragma pack(1)//ここから1バイトパッキング…アライメントは発生しない
	// PMD マテリアル構造体
struct PMDMaterial {
	DirectX::XMFLOAT3 diffuse;	// ディヒューズ色
	float alpha;		// ディヒューズα
	float specularity;	// スペキュラの強さ（乗算値）
	DirectX::XMFLOAT3 specular;	// スペキュラ色
	DirectX::XMFLOAT3 ambient;	// アンビエント色
	unsigned char toonIdx;	// トゥーン番号
	unsigned char edgeFlag;	// マテリアルごとの輪郭線フラグ

	// 2バイトのパディングが発生

	unsigned int indicesNum;	// このマテリアルが割り当てられるインデックス数
	char texFilePath[20];		// テクスチャファイルパス + α
}; // 70バイトのはずだが、パディングにより72バイトになる
#pragma pack()//1バイトパッキング解除
static_assert(sizeof(PMDMaterial) == 70, "assertion error.");
