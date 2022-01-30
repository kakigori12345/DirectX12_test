#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include <d3d12.h>
#include <DirectXMath.h>

#include <string>
#include <vector>

#include <wrl.h>

//-----------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------
// シェーダー側に投げられるマテリアルデータ
struct MaterialForHlsl {
	DirectX::XMFLOAT3 diffuse;	// ディヒューズ色
	float alpha;				// ディヒューズα
	DirectX::XMFLOAT3 specular;	// スペキュラ色
	float specularity;			// スペキュラの強さ（乗算値）
	DirectX::XMFLOAT3 ambient;	// アンビエント色
};

// それ以外のマテリアルデータ
struct AdditionalMaterial {
	std::string texPath;	// テクスチャファイルパス
	int toonIdx;			// トゥーン番号
	bool edgeFlag;			// マテリアルごとの輪郭線フラグ
};

// 全体をまとめる
struct Material {
	unsigned int indicesNum;	// インデックス数
	MaterialForHlsl material;
	AdditionalMaterial additional;
};


class PMDActor {
	//----------------------------------------------------
	// コンストラクタ関連
	//----------------------------------------------------
public:
	//! @brief コンストラクタ
	explicit PMDActor(std::string modelPath);
	//! @brief デストラクタ
	~PMDActor();

	//----------------------------------------------------
	// メソッド
	//----------------------------------------------------
public:
	//! @brief 初期化
	bool Init(ID3D12Device* device);


	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:
	std::string m_modelPath;

private:
public: //TODO:一旦publicにしているだけ。リファクタ後に戻す
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_vertBuff;
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_idxBuff;
	D3D12_VERTEX_BUFFER_VIEW							m_vbView;
	D3D12_INDEX_BUFFER_VIEW								m_ibView;

	std::vector<Material>								m_materials;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_materialDescHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_materialBuff;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_whiteTex;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_blackTex;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_gradTex;
};