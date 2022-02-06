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

//! @brief 描画時の情報を集めた構造体
struct DrawActorInfo {
	D3D_PRIMITIVE_TOPOLOGY topology; //CA:こいつにトポロジーを含ませていいものか悩む
	const D3D12_VERTEX_BUFFER_VIEW* vbView;
	const D3D12_INDEX_BUFFER_VIEW* ibView;
	D3D12_DESCRIPTOR_HEAP_TYPE descHeapType;
	unsigned int incCount;
	ID3D12DescriptorHeap* transformDescHeap;
	ID3D12DescriptorHeap* materialDescHeap;
	const std::vector<Material>* materials;
};

struct Transform {
	//内部に持ってるXMMATRIXメンバが16バイトアライメントであるため
	//Transformをnewする際には16バイト境界に確保する
	void* operator new(size_t size);
	DirectX::XMMATRIX world;
};

#pragma pack(1)
struct PMDBone {
	char boneName[20];		// ボーン名
	unsigned short parentNo;// 親ボーン番号
	unsigned short nextNo;	// 先端のボーン番号。方向を示すための情報
	unsigned char type;		// ボーン種別。今回は「回転」のみを扱う
	unsigned short ikBoneNo;// IKボーン番号
	DirectX::XMFLOAT3 pos;			// ボーンの基準点座標
};
#pragma pack()


class PMDActor {
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

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

	//! @brief 描画情報取得
	void GetDrawInfo(DrawActorInfo& output) const;

	//! @brief 描画情報更新
	void Update();

private:
	//座標変換用ビューの生成
	bool _CreateTransformView(ID3D12Device* device);


	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:
	std::string m_modelPath;
	float		m_angleY;

private:
	Transform						m_transform;
	Transform*						m_mappedTransform;
	ComPtr<ID3D12Resource>			m_transformBuff;

	ComPtr<ID3D12Resource>			m_transformMat;//座標変換行列(今はワールドのみ)
	ComPtr<ID3D12DescriptorHeap>	m_transformHeap;//座標変換ヒープ

private:
	ComPtr<ID3D12Resource>			m_vertBuff;
	ComPtr<ID3D12Resource>			m_idxBuff;
	D3D12_VERTEX_BUFFER_VIEW		m_vbView;
	D3D12_INDEX_BUFFER_VIEW			m_ibView;

	std::vector<Material>			m_materials;
	ComPtr<ID3D12DescriptorHeap>	m_materialDescHeap;
	ComPtr<ID3D12Resource>			m_materialBuff;
};