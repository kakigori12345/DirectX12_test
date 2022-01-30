#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
// 標準ライブラリ
#include <vector>
#include<memory>

// Direct3D
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>


//ComPtr
#include <wrl.h>

#include "Util/SingletonDef.h"


class Dx12Wrapper {
	SINGLETON_HEADER(Dx12Wrapper)

	//-----------------------------------------------------------------
	// Type Definition
	//-----------------------------------------------------------------
	// シェーダー側に渡すための基本的な行列データ
	struct SceneData {
		// TODO: 16バイトアライメントを施す
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMFLOAT3 eye;
	};

	//----------------------------------------------------
	// メソッド
	//----------------------------------------------------
public:
	// @brief クラスの初期化
	// @note このクラスを使用する前に一度必ず呼び出すこと
	bool Init(HWND hwnd);


	//------------------------------------------------------------------
	// Dx12関連のオブジェクト取得系
	// TODO: リファクタできたら必要ない関数ばかりになるので整理する
	//------------------------------------------------------------------
	// @brief デバイスを取得
	ID3D12Device* GetDevice();
	// @brief ファクトリを取得
	IDXGIFactory6* GetFactory();
	// @brief スワップチェーンを取得
	IDXGISwapChain4* GetSwapchain();
	// @brief コマンドリストを取得
	ID3D12GraphicsCommandList* GetCommandList();


	//! @brief コマンドリスト実行
	//! @note 処理が完了するまで内部で待機する
	bool ExecuteCommandList();

	//! @brief コマンドリストをリセット
	bool ResetCommandList();

	//! @brief スワップチェーンのフリップ処理
	bool SwapchainPresent();

	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:
	bool m_isInitialized;

private:
	Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
	Microsoft::WRL::ComPtr<IDXGIFactory6>				m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>				m_swapchain;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_cmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_cmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_cmdQueue;

public: //一時的にpublicにしておく。TODO:リファクタ後はprivateにしておく
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		rtvHeaps;
	std::vector<ID3D12Resource*>						_backBuffers;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		basicDescHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource>				constBuff;

	SceneData*											mapMatrix;

	Microsoft::WRL::ComPtr<ID3D12Resource>				depthBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		dsvHeap;

	CD3DX12_VIEWPORT									viewport;
	D3D12_RECT											scissorrect;
};