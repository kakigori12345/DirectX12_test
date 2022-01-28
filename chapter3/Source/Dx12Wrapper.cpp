//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
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



bool Dx12Wrapper::Init() {
	assert(!m_isInitialized);

#ifdef _DEBUG
	EnableDebugLayer();
#endif

	// ファクトリー
#ifdef _DEBUG
	auto result = CreateDXGIFactory2(
		DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()));
#else
	auto result = CreateDXGIFactory1(
		IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
#endif

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