//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Dx12Wrapper.h"

#include <d3dcompiler.h>

// その他
#include "Util/Utility.h"
#include <assert.h>


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;
//using namespace DirectX;
using namespace Microsoft::WRL;


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------
namespace {
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
	: m_isInitialized(false)
	, m_device(nullptr)
	, m_dxgiFactory(nullptr)
	, m_swapchain(nullptr)
	, m_cmdAllocator(nullptr)
	, m_cmdList(nullptr)
	, m_cmdQueue(nullptr)
	, rtvHeaps(nullptr)
	, _backBuffers()
	, _vsBlob(nullptr)
	, _psBlob(nullptr)
	, errorBlob(nullptr)
	, basicDescHeap(nullptr)
	, constBuff(nullptr)
	, mapMatrix(nullptr)
	, depthBuffer(nullptr)
	, dsvHeap(nullptr)
{}

// @brief デストラクタ
Dx12Wrapper::~Dx12Wrapper() {

}

// シングルトン
SINGLETON_CPP(Dx12Wrapper)



bool Dx12Wrapper::Init(HWND hwnd) {
	assert(!m_isInitialized);
	HRESULT result = S_OK;
	// ウィンドウサイズ取得
	WindowInfo wInfo = GetWindowInfo(hwnd);

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
			D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmdAllocator.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandAllocator.");
			return false;
		}

		result = m_device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_cmdAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(m_cmdList.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandList.");
			return false;
		}
	}

	{// キューの作成
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //タイムアウトなし
		cmdQueueDesc.NodeMask = 0; //アダプター一つなので０でいい（らしい）
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		// 生成
		result = m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(m_cmdQueue.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandQueue.");
			return false;
		}
	}

	{// スワップチェーンの作成
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.Width = wInfo.width;
		swapchainDesc.Height = wInfo.height;
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
			m_cmdQueue.Get(),
			hwnd,
			&swapchainDesc,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)m_swapchain.ReleaseAndGetAddressOf());
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating SwapChain.");
			return false;
		}
	}

	{// ディスクリプタヒープの作成
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビュー
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = 2; //表裏の２つ
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		result = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf())); //この段階ではまだ RTV ではない
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating DescriptorHeap.");
			return false;
		}

		// sRGB 用のレンダーターゲットビュー設定を作成しておく
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//ガンマ補正あり
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		// スワップチェーンとビューの関連付け
		_backBuffers.resize(COMMAND_BUFFER_COUNT);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		for (UINT idx = 0; idx < COMMAND_BUFFER_COUNT; ++idx) {
			result = m_swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
			if (result != S_OK) {
				DebugOutputFormatString("Missed at Getting BackBuffer.");
				return false;
			}
			// 先ほど作成したディスクリプタヒープを RTV として設定する
			rtvDesc.Format = _backBuffers[idx]->GetDesc().Format;
			m_device->CreateRenderTargetView(
				_backBuffers[idx],
				&rtvDesc,
				handle);
			// ハンドルを一つずらす
			handle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}
	}


	{// シェーダーリソースビュー
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//シェーダーから見えるように
		descHeapDesc.NodeMask = 0;		// アダプタは一つなので0をセット
		descHeapDesc.NumDescriptors = 1;// CBV
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	//シェーダーリソースビュー用

		result = m_device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Descriptor Heap For ShaderReosurceView.");
			return false;
		}
	}


	{// 定数バッファーの作成
		D3D12_HEAP_PROPERTIES constBufferHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC constBufferDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
		m_device->CreateCommittedResource(
			&constBufferHeap,
			D3D12_HEAP_FLAG_NONE,
			&constBufferDesc,	// 0xffアライメント
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf())
		);
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Const Buffer.");
			return false;
		}
		// マップで定数コピー
		result = constBuff->Map(0, nullptr, (void**)&mapMatrix);

		// 定数バッファービューを作成する
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constBuff->GetDesc().Width;
		// ディスクリプタヒープ上でのメモリ位置（ハンドル）を取得
		auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart(); //この状態だとシェーダリソースビューの位置を示す
		// 実際に定数バッファービューを作成
		m_device->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
	}


	{// シェーダーの読み込みと生成
		result = D3DCompileFromFile(
			L"Resource/BasicVertexShader.hlsl",
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicVS",
			"vs_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //デバッグ用 および 最適化なし
			0,
			&_vsBlob,
			&errorBlob);
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Compiling Vertex Shader.");
			return false;
		}

		result = D3DCompileFromFile(
			L"Resource/BasicPixelShader.hlsl",
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicPS",
			"ps_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //デバッグ用 および 最適化なし
			0,
			&_psBlob,
			&errorBlob);
		if (result != S_OK) {
			// 詳細なエラー表示
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			OutputDebugStringA(errstr.c_str());

			DebugOutputFormatString("Missed at Compiling Pixel Shader.");
			return false;
		}
	}

	{// 深度バッファの作成
		D3D12_RESOURCE_DESC depthResDesc = {};
		depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthResDesc.Width = wInfo.width;
		depthResDesc.Height = wInfo.height;
		depthResDesc.DepthOrArraySize = 1;	//配列でも3Dテクスチャでもない
		depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthResDesc.SampleDesc.Count = 1;	//サンプルは1ピクセルあたり一つ
		depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		// 深度値用ヒーププロパティ
		D3D12_HEAP_PROPERTIES depthHeapProp = {};
		depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_CLEAR_VALUE depthClearValue = {};
		depthClearValue.DepthStencil.Depth = 1.0f;	// 深さ1.0fでクリア
		depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	//32ビット float 値としてクリア

		result = m_device->CreateCommittedResource(
			&depthHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&depthResDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,	//深度地書き込み用に使う
			&depthClearValue,
			IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf())
		);
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating depth stensil buffer.");
			return false;
		}

		// 深度バッファービューの作成
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

		result = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Depth Heap.");
			return false;
		}

		// 深度ビューの作成
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		m_device->CreateDepthStencilView(
			depthBuffer.Get(),
			&dsvDesc,
			dsvHeap->GetCPUDescriptorHandleForHeapStart()
		);
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
	return m_cmdList.Get();
}


//! @brief コマンドリスト実行
//! @note 処理が完了するまで内部で待機する
bool Dx12Wrapper::ExecuteCommandList() {
	// コマンドリスト実行
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdLists);
	// フェンスを作成しておく
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;
	HRESULT result = m_device->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));
	// GPUの処理が完了するまで待つ
	m_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
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
	HRESULT result = m_cmdAllocator->Reset();
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Reset Allocator.");
		return false;
	}
	result = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
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


