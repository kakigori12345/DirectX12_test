//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.pch"
#include "Dx12Wrapper.h"

// Direct3D
#pragma comment( lib, "d3d12.lib")
#pragma comment( lib, "dxgi.lib")

// シェーダーのコンパイル
#include <d3dcompiler.h>
#pragma comment( lib, "d3dcompiler.lib")

// DirectXTexライブラリ
#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")

// その他
#include "Utility.h"
#include <assert.h>

#ifdef _DEBUG 
#include < iostream >
#endif


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------
namespace {
	// @brief コンソール 画面 に フォーマット 付き 文字列 を 表示 
	// @param format フォーマット（% d とか% f とか の） 
	// @param 可変 長 引数 
	// @remarks この 関数 は デバッグ 用 です。 デバッグ 時 にしか 動作 し ませ ん
	void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
		va_list valist;
		va_start(valist, format);
		vprintf_s(format, valist);
		va_end(valist);
#endif
	}

	// @brief デバッグレイヤーの有効化
	void EnableDebugLayer() {
		ID3D12Debug* debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

// @brief コンストラクタ
Dx12Wrapper::Dx12Wrapper()
	: m_device(nullptr)
	, m_dxgiFactory(nullptr)
	, m_swapchain(nullptr)
	, _cmdAllocator(nullptr)
	, _cmdList(nullptr)
	, _cmdQueue(nullptr)
	, m_windowHeight(0)
	, m_windowWidth(0)
	, m_isInitialized(false)
{}

// シングルトン関連
std::unique_ptr<Dx12Wrapper> Dx12Wrapper::s_instance = nullptr;
Dx12Wrapper* Dx12Wrapper::Instance() {
	assert(s_instance);
	return s_instance.get();
}
void Dx12Wrapper::Create() {
	s_instance = unique_ptr<Dx12Wrapper>(new Dx12Wrapper());
}
void Dx12Wrapper::Destroy() {
	s_instance.reset();
}



bool Dx12Wrapper::Init(HWND hwnd) {
	assert(!m_isInitialized);
	HRESULT result = S_OK;

	{// ウィンドウサイズを取得
		RECT rect;
		GetWindowRect(hwnd, &rect);
		m_windowHeight = rect.bottom - rect.top;
		m_windowWidth = rect.right - rect.left;
	}

#ifdef _DEBUG
	EnableDebugLayer();
#endif

	{// ファクトリー
#ifdef _DEBUG
		result = CreateDXGIFactory2(
			DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()));
#else
		auto result = CreateDXGIFactory1(
			IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
#endif
	}

	{
		// アダプター
		std::vector <IDXGIAdapter*> adapters; //ここにアダプターを列挙する
		IDXGIAdapter* tmpAdapter = nullptr;
		for (int i = 0; m_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
			adapters.push_back(tmpAdapter);
		}
		// アダプターを識別するための情報を取得（DXGI_ADAPTER＿DESC構造体）
		for (auto adpt : adapters) {
			DXGI_ADAPTER_DESC adesc = {};
			adpt->GetDesc(&adesc); // アダプターの説明オブジェクト取得
			std::wstring strDesc = adesc.Description;

			// 探したいアダプターの名前を確認
			if (strDesc.find(L"NVIDIA") != std::string::npos) {
				tmpAdapter = adpt;
				break;
			}
		}

		// デバイスオブジェクト
		D3D12CreateDevice(
			tmpAdapter,
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()));
	}


	{// コマンド
		result = m_device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandAllocator.");
			return 0;
		}

		result = m_device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			_cmdAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandList.");
			return 0;
		}
	}

	{// キューの作成
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //タイムアウトなし
		cmdQueueDesc.NodeMask = 0; //アダプター一つなので０でいい（らしい）
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		// 生成
		result = m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandQueue.");
			return 0;
		}
	}

	{// スワップチェーンの作成
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.Width = m_windowWidth;
		swapchainDesc.Height = m_windowHeight;
		swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.Stereo = false;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.SampleDesc.Quality = 0;
		swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
		swapchainDesc.BufferCount = 2;
		swapchainDesc.Scaling = DXGI_SCALING_STRETCH; // バックバッファーは伸び縮み可能
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップ後は速やかに破棄
		swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // ウィンドウ⇔フルスクリーン切り替え可能

		result = m_dxgiFactory->CreateSwapChainForHwnd(
			_cmdQueue.Get(),
			hwnd,
			&swapchainDesc,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)m_swapchain.ReleaseAndGetAddressOf());
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating SwapChain.");
			return 0;
		}
	}


	m_isInitialized = true;
	return true;
}


// @brief デバイスを取得
	// @return デバイス
ID3D12Device* Dx12Wrapper::GetDevice() {
	return m_device.Get();
}

// @brief ファクトリを取得
IDXGIFactory6* Dx12Wrapper::GetFactory(){
	return m_dxgiFactory.Get();
}

// @brief スワップチェーンを取得
IDXGISwapChain4* Dx12Wrapper::GetSwapchain() {
	return m_swapchain.Get();
}

// @brief コマンドリストを取得
ID3D12GraphicsCommandList* Dx12Wrapper::GetCommandList() {
	return _cmdList.Get();
}


//! @brief コマンドリスト実行
//! @note 処理が完了するまで内部で待機する
bool Dx12Wrapper::ExecuteCommandList() {
	// コマンドリスト実行
	ID3D12CommandList* cmdLists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdLists);
	// フェンスを作成しておく
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;
	HRESULT result = m_device->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));
	// GPUの処理が完了するまで待つ
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
	if (_fence->GetCompletedValue() != _fenceVal) {
		// イベントハンドルを取得
		auto event = CreateEvent(nullptr, false, false, nullptr);
		if (event == nullptr) {
			DebugOutputFormatString("Missed at Creating Event.");
			return false;
		}

		_fence->SetEventOnCompletion(_fenceVal, event);

		// イベントが発生するまで待機
		WaitForSingleObject(event, INFINITE);

		// イベントハンドルを閉じる
		CloseHandle(event);
	}
	// 待機
	while (_fence->GetCompletedValue() != _fenceVal) { ; }

	return true;
}


//! @brief コマンドリストをリセット
bool Dx12Wrapper::ResetCommandList() {
	HRESULT result = _cmdAllocator->Reset();
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Reset Allocator.");
		return false;
	}
	result = _cmdList->Reset(_cmdAllocator.Get(), nullptr);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Reset CommandList.");
		return false;
	}

	return true;
}


//! @brief スワップチェーンのフリップ処理
bool Dx12Wrapper::SwapchainPresent() {
	HRESULT result = m_swapchain->Present(1, 0);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Present Swapchain.");
		return false;
	}

	return true;
}


