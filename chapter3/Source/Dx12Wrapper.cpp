//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Dx12Wrapper.h"

#include <d3dcompiler.h>

#include "PMD/PMDActor.h"

// その他
#include "Util/Utility.h"
#include <assert.h>


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
	, basicDescHeap(nullptr)
	, constBuff(nullptr)
	, mapMatrix(nullptr)
	, depthBuffer(nullptr)
	, dsvHeap(nullptr)
	, viewport()
	, scissorrect(){
}

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

	{// ビューポートとシザー矩形
		viewport = CD3DX12_VIEWPORT{ _backBuffers[0] };
		scissorrect.top = 0;
		scissorrect.left = 0;
		scissorrect.right = scissorrect.left + wInfo.width;
		scissorrect.bottom = scissorrect.top + wInfo.height;
	}


	{// シーンデータを設定
		// ワールド行列
		angleY = 0; // XM_PIDIV4
		worldMat = XMMatrixRotationY(angleY);
		// ビュー行列
		eye = XMFLOAT3(0, 10, -15);
		target = XMFLOAT3(0, 10, 0);
		up = XMFLOAT3(0, 1, 0);
		viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		// プロジェクション行列
		projMat = XMMatrixPerspectiveFovLH(
			XM_PIDIV2,	//画角は90度
			static_cast<float>(wInfo.width) / static_cast<float>(wInfo.height),	// アスペクト比
			1.0f,	// ニアクリップ
			100.0f	// ファークリップ
		);
	}


	m_isInitialized = true;
	return true;
}


//! @brief シーンデータをセット
void Dx12Wrapper::SetSceneData() {
	// とりあえず回転させておく
	angleY += 0.01f;
	worldMat = XMMatrixRotationY(angleY);

	// セット
	mapMatrix->world = worldMat;
	mapMatrix->view = viewMat;
	mapMatrix->proj = projMat;
	mapMatrix->eye = eye;
}

//! @brief 描画前処理
void Dx12Wrapper::BeginDraw() {
	// レンダーターゲットをバックバッファにセット
		// 現在のバックバッファを取得
	const SIZE_T bbIdx = m_swapchain->GetCurrentBackBufferIndex(); // バッファは２つなので、0か1のはず

	// リソースバリアでバッファの使い道を GPU に通知する
	D3D12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_cmdList->ResourceBarrier(1, &BarrierDesc); //バリア指定実行

	// レンダーターゲットとして指定する
	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// 深度バッファビューを関連付け
	auto dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvHandle);
	// 深度バッファのクリア
	m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// レンダーターゲットを指定色でクリア
	float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; //白色
	m_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	m_cmdList->RSSetViewports(1, &viewport);
	m_cmdList->RSSetScissorRects(1, &scissorrect);
}

//! @brief 描画
	//! @param[in] actor 描画対象
void Dx12Wrapper::Draw(const DrawActorInfo& drawInfo) {
	m_cmdList->IASetVertexBuffers(0, 1, drawInfo.vbView);
	m_cmdList->IASetIndexBuffer(drawInfo.ibView);

	{// 描画時の設定
		ID3D12DescriptorHeap* bdh[] = { basicDescHeap.Get() };
		m_cmdList->SetDescriptorHeaps(1, bdh);
		m_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		// マテリアル
		ID3D12DescriptorHeap* mdh[] = { drawInfo.materialDescHeap };
		m_cmdList->SetDescriptorHeaps(1, mdh);

		auto materialHandle = drawInfo.materialDescHeap->GetGPUDescriptorHandleForHeapStart();
		unsigned int idxOffset = 0;
		auto cbvsrvIncSize = m_device->GetDescriptorHandleIncrementSize(drawInfo.descHeapType);
		cbvsrvIncSize *= drawInfo.incCount;	//CBV, SRV, SRV, SRV, SRV の５つ分

		for (auto& m : *(drawInfo.materials)) {
			m_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
			m_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

			// ヒープポインタとインデックスを次に進める
			materialHandle.ptr += cbvsrvIncSize;
			idxOffset += m.indicesNum;
		}
	}
}

//! @brief 描画終了時処理
void Dx12Wrapper::EndDraw() {
	const SIZE_T bbIdx = m_swapchain->GetCurrentBackBufferIndex();

	// リソースバリアでバッファの使い道を GPU に通知する
	D3D12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	);
	m_cmdList->ResourceBarrier(1, &BarrierDesc); //バリア指定実行

	m_cmdList->Close();
	ExecuteCommandList();
	ResetCommandList();

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	SwapchainPresent();
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


