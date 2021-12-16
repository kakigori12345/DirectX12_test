#include <Windows.h>
#include <vector>

#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment( lib, "d3d12.lib")
#pragma comment( lib, "dxgi.lib")


#ifdef _DEBUG 
#include < iostream >

#endif using namespace std;

namespace {

	// 定数
	int window_width = 800;
	int window_height = 640;



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

	// 面倒 だ けど 書か なけれ ば いけ ない 関数
	LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		// ウィンドウ が 破棄 さ れ たら 呼ば れる
		if (msg == WM_DESTROY) {
			PostQuitMessage(0);
			// OS に対して「 もうこ の アプリ は 終わる」 と 伝える
			return 0;
		}

		return DefWindowProc(hwnd, msg, wparam, lparam); // 既定 の 処理 を 行う
	}

}


#ifdef _DEBUG
int main() {
#else 
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	// ウィンドウ クラス の 生成＆ 登録
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // コール バック 関数 の 指定
	w.lpszClassName = ("DX12Sample"); // アプリケーション クラス 名（ 適当 で よい）
	w.hInstance = GetModuleHandle(nullptr); // ハンドル の 取得
	RegisterClassEx(&w); // アプリケーション クラス（ ウィンドウ クラス の 指定 を OS に 伝える）
	RECT wrc = { 0, 0, window_width, window_height };// ウィンドウサイズ を 決める

	// 関数 を 使っ て ウィンドウ の サイズ を 補正 する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウ オブジェクト の 生成
	HWND hwnd = CreateWindow(
		w.lpszClassName,// クラス 名 指定
		("DX12テスト"), // タイトル バー の 文字
		WS_OVERLAPPEDWINDOW, // タイトル バー と 境界線 が ある ウィンドウ
		CW_USEDEFAULT, // 表示 x 座標 は OS に お 任せ
		CW_USEDEFAULT, // 表示 y 座標 は OS に お 任せ
		wrc.right - wrc.left, // ウィンドウ 幅
		wrc.bottom - wrc.top, // ウィンドウ 高
		nullptr, // 親 ウィンドウ ハンドル
		nullptr, // メニュー ハンドル
		w.hInstance, // 呼び出し アプリケーション ハンドル
		nullptr); // 追加 パラメーター

	// ウィンドウ 表示
	ShowWindow(hwnd, SW_SHOW);



	// 3Dオブジェクトの生成
	ID3D12Device* _dev = nullptr;
	IDXGIFactory6* _dxgiFactory = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;


	// ファクトリー
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));

	// アダプター
	std::vector <IDXGIAdapter*> adapters; //ここにアダプターを列挙する
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
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
	D3D12CreateDevice(tmpAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_dev));


	// コマンドリストの作成とコマンドアロケータ
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating CommandAllocator.");
		return 0;
	}
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating CommandList.");
		return 0;
	}

	// キューの作成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //タイムアウトなし
	cmdQueueDesc.NodeMask = 0; //アダプター一つなので０でいい（らしい）
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// 生成
	ID3D12CommandQueue* _cmdQueue = nullptr;
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating CommandQueue.");
		return 0;
	}

	// スワップチェーンの作成
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
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

	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating SwapChain.");
		return 0;
	}

	// ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビュー
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; //表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps)); //この段階ではまだ RTV ではない
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating DescriptorHeap.");
		return 0;
	}

	// スワップチェーンとビューの関連付け
	std::vector<ID3D12Resource*> _backBuffers(swapchainDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (UINT idx = 0; idx < swapchainDesc.BufferCount; ++idx) {
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Getting BackBuffer.");
			return 0;
		}
		// 先ほど作成したディスクリプタヒープを RTV として設定する
		_dev->CreateRenderTargetView(
			_backBuffers[idx], 
			nullptr, 
			handle);
		// ハンドルを一つずらす
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}


	MSG msg = {};
	
	while (true) {
		// メインループの処理
		{
			// 1.コマンドアロケータとコマンドリストをクリア
			result = _cmdAllocator->Release();
			if (result != S_OK) {
				DebugOutputFormatString("Missed at Reset Allocator.");
				return 0;
			}

			// 2.レンダーターゲットをバックバッファにセット
			// 現在のバックバッファを取得
			UINT bbIdx = _swapchain->GetCurrentBackBufferIndex(); // バッファは２つなので、0か1のはず
			auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

			// 3.レンダーターゲットを指定色でクリア

			// 4.レンダーターゲットをクローズ

			// 5.たまったコマンドをコマンドリストに投げる

			// 6.スワップチェーンのフリップ処理

		}

		// メッセージ処理
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg); DispatchMessage(&msg);
		}

		//アプリケーション が 終わる とき に message が WM_ QUIT に なる
		if (msg. message == WM_QUIT) {
			break;
		}
	}
	
	//もう クラス は 使わ ない ので 登録 解除 する
	UnregisterClass( w. lpszClassName, w. hInstance);




	DebugOutputFormatString(" Show window test.");
	//getchar();
	return 0;
}
