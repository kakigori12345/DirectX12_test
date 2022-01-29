#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------

// 標準ライブラリ
#include <vector>
#include<memory>

// Direct3D
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <DirectXMath.h>

//ComPtr
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

// シェーダー側に渡すための基本的な行列データ
struct SceneData {
	// TODO: 16バイトアライメントを施す
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
	DirectX::XMFLOAT3 eye;
};


//! アプリケーションクラス（シングルトン）
class Application {
	//----------------------------------------------------
	// コンストラクタ関連
	//----------------------------------------------------
private:
	// シングルトンなので非公開
	Application() {};
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
public:
	~Application() {};
public:
	// シングルトン用初期化関数
	static void Create();
	static void Destroy();

	//----------------------------------------------------
	// メソッド
	//----------------------------------------------------
public:
	static Application* Instance();

	bool Init();

	void Run();

	void Terminate();

	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:
	WNDCLASSEX window;
	/*Microsoft::WRL::ComPtr<IDXGISwapChain4>				_swapchain		= nullptr;*/

	/*Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		_cmdAllocator	= nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	_cmdList		= nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			_cmdQueue		= nullptr;*/

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		rtvHeaps		= nullptr;
	std::vector<ID3D12Resource*>						_backBuffers;

	Microsoft::WRL::ComPtr<ID3D12Resource>				vertBuff		= nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>				idxBuff			= nullptr;
	D3D12_VERTEX_BUFFER_VIEW							vbView;
	D3D12_INDEX_BUFFER_VIEW								ibView;

	Microsoft::WRL::ComPtr<ID3DBlob>					_vsBlob			= nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob>					_psBlob			= nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob>					errorBlob		= nullptr;

	std::vector<Material>								materials;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		materialDescHeap= nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>				 materialBuff	= nullptr;
	
	Microsoft::WRL::ComPtr<ID3D12Resource> whiteTex;
	Microsoft::WRL::ComPtr<ID3D12Resource> blackTex;
	Microsoft::WRL::ComPtr<ID3D12Resource> gradTex;


	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		basicDescHeap	= nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>				constBuff		= nullptr;

	SceneData*											mapMatrix		= nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource>				depthBuffer		= nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		dsvHeap			= nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature>			rootSignature	= nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob>					rootSigBlob		= nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState>			_pipelinestate	= nullptr;

	CD3DX12_VIEWPORT									viewport;
	D3D12_RECT											scissorrect;

	float												angleY;
	DirectX::XMFLOAT3									eye;
	DirectX::XMFLOAT3									target;
	DirectX::XMFLOAT3									up;
	DirectX::XMMATRIX									worldMat;
	DirectX::XMMATRIX									viewMat;
	DirectX::XMMATRIX									projMat;

private:
	static std::unique_ptr<Application> s_instance;
};