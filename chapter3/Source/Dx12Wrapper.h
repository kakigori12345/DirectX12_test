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


class Dx12Wrapper {
	//----------------------------------------------------
	// コンストラクタ関連
	//----------------------------------------------------
private:
	// シングルトンなので非公開
	Dx12Wrapper();
	Dx12Wrapper(const Dx12Wrapper&) = delete;
	Dx12Wrapper& operator=(const Dx12Wrapper&) = delete;
public:
	~Dx12Wrapper() {};
public:
	// シングルトン用関数
	static void Create();
	static void Destroy();
	static Dx12Wrapper* Instance();


	//----------------------------------------------------
	// メソッド
	//----------------------------------------------------
public:
	// @brief クラスの初期化
	// @note このクラスを使用する前に一度必ず呼び出すこと
	bool Init();


	//------------------------------------------------------------------
	// Dx12関連のオブジェクト取得系
	// TODO: リファクタできたら必要ない関数ばかりになるので整理する
	//------------------------------------------------------------------
	// @brief デバイスを取得
	ID3D12Device* GetDevice();
	// @brief ファクトリを取得
	IDXGIFactory6* GetFactory();


	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:
	Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
	Microsoft::WRL::ComPtr<IDXGIFactory6>				m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>				m_swapchain;

private:
	static std::unique_ptr<Dx12Wrapper> s_instance;
	bool m_isInitialized;
};