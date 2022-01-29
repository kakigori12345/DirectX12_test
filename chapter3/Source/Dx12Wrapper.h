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

//ComPtr
#include <wrl.h>

#include "Util/SingletonDef.h"


class Dx12Wrapper {
	SINGLETON_HEADER(Dx12Wrapper)
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
	Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
	Microsoft::WRL::ComPtr<IDXGIFactory6>				m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>				m_swapchain;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		_cmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	_cmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			_cmdQueue;

private:
	UINT m_windowHeight;
	UINT m_windowWidth;

private:
	bool m_isInitialized;
};