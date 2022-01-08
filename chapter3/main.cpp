#include <Windows.h>
#include <vector>

// Direct3D
#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment( lib, "d3d12.lib")
#pragma comment( lib, "dxgi.lib")

// シェーダーのコンパイル
#include <d3dcompiler.h>
#pragma comment( lib, "d3dcompiler.lib")

// 数学関数
#include <DirectXMath.h>

// DirectXTexライブラリ
#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")


#ifdef _DEBUG 
#include < iostream >

#endif
using namespace std;
using namespace DirectX;

namespace {

	// 定数
	int window_width = 800;
	int window_height = 480;

	// 頂点情報
	struct Vertex {
		XMFLOAT3 pos;	// xyz 座標
		XMFLOAT2 uv;	// uv 座標
	};

	// テクスチャデータ
	struct TexRGBA {
		unsigned char R, G, B, A;
	};



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

	// デバッグレイヤーの有効化
	void EnableDebugLayer() {
		ID3D12Debug* debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
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

#ifdef _DEBUG
	EnableDebugLayer();
#endif


	// 3Dオブジェクトの生成
	ID3D12Device* _dev = nullptr;
	IDXGIFactory6* _dxgiFactory = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;


	// ファクトリー
#ifdef _DEBUG
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif

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

	// sRGB 用のレンダーターゲットビュー設定を作成しておく
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//ガンマ補正あり
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
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
			&rtvDesc,
			handle);
		// ハンドルを一つずらす
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}


	// ポリゴンの表示
	// 頂点情報作成
	Vertex vertices[] = {
		{{-0.4f, -0.7f, 0.0f}, {0.0f, 1.0f}},	// 左下
		{{-0.4f,  0.7f, 0.0f}, {0.0f, 0.0f}},	// 左上
		{{ 0.4f, -0.7f, 0.0f}, {1.0f, 1.0f}},	// 右下
		{{ 0.4f,  0.7f, 0.0f}, {1.0f, 0.0f}},	// 右上
	};
	// 頂点バッファの作成
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices); //頂点情報が入るだけのサイズ
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating CommittedResource.");
		return 0;
	}
	// 頂点情報のコピー
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Mapping Vertex.");
		return 0;
	}
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr); // verMap の情報を渡したので、マップを解除する
	// 頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // バッファの仮想アドレス
	vbView.SizeInBytes = sizeof(vertices); //全バイト数
	vbView.StrideInBytes = sizeof(vertices[0]); //１頂点当たりのバイト数


	// インデックス情報作成
	unsigned short indices[] = {
		0, 1, 2,
		2, 1, 3
	};
	// インデックスバッファの作成
	ID3D12Resource* idxBuff = nullptr;
	resdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff) );
	// バッファにコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);
	// インデックスバッファビューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);


	// テクスチャデータ作成（256*256）
	//std::vector<TexRGBA> texturedata(256 * 256);
	//for (auto& rgba : texturedata) {
	//	rgba.R = rand() % 256;
	//	rgba.G = rand() % 256;
	//	rgba.B = rand() % 256;
	//	rgba.A = 255;	// αは1.0
	//}
	// テクスチャに画像データを用意する
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(
		L"data/img/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Loading Image Data.");
		return 0;
	}
	const Image* img = scratchImg.GetImage(0, 0, 0);	//生データ抽出

	// テクスチャバッファの作成
	D3D12_HEAP_PROPERTIES heappropTex = {};
	heappropTex.Type = D3D12_HEAP_TYPE_CUSTOM;	//特殊な設定なので DEFAULT でも UPLOAD でもない
	heappropTex.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;	//ライトバック
	heappropTex.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;	//転送はL0,つまりCPU側から直接行う
	heappropTex.CreationNodeMask = 0;	//単一アダプターなので 0
	heappropTex.VisibleNodeMask = 0;
	// リソース設定
	D3D12_RESOURCE_DESC resDescTex = {};
	resDescTex.Format	= metadata.format;	// RGBAフォーマット
	resDescTex.Width	= metadata.width;
	resDescTex.Height	= metadata.height;
	resDescTex.DepthOrArraySize		= metadata.arraySize;
	resDescTex.SampleDesc.Count		= 1;	//通常テクスチャなのでアンチエイリアシングしない
	resDescTex.SampleDesc.Quality	= 0;	//クオリティは最低
	resDescTex.MipLevels	= metadata.mipLevels;
	resDescTex.Dimension	= static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDescTex.Layout		= D3D12_TEXTURE_LAYOUT_UNKNOWN;			//レイアウトは決定しない
	resDescTex.Flags		= D3D12_RESOURCE_FLAG_NONE;				//特にフラグなし
	// リソースの生成
	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heappropTex,
		D3D12_HEAP_FLAG_NONE,
		&resDescTex,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, //テクスチャ用指定
		nullptr,
		IID_PPV_ARGS(&texbuff));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Texture Resource.");
		return 0;
	}
	// データ転送
	result = texbuff->WriteToSubresource(
		0,			// サブリソースインデックス
		nullptr,	// 書き込み領域の指定（今回は先頭から全領域）
		img->pixels,	// 書き込みたいデータのアドレス
		img->rowPitch,	// １行あたりのデータサイズ
		img->slicePitch	// スライスあたりのデータサイズ（今回は全サイズ）
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Writing to Subresource.");
		return 0;
	}
	// シェーダーリソースビュー
	ID3D12DescriptorHeap* texDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//シェーダーから見えるように
	descHeapDesc.NodeMask = 0;		// アダプタは一つなので0をセット
	descHeapDesc.NumDescriptors = 1;// ディスクリプタヒープの数は一つ
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	//シェーダーリソースビュー用
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Descriptor Heap For ShaderReosurceView.");
		return 0;
	}
	// シェーダーリソースビューを作る
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	// 2Dテクスチャ
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;	// ミニマップは使用しないので1
	_dev->CreateShaderResourceView(
		texbuff,	// ビューと関連付けるバッファ
		&srvDesc,	// テクスチャ設定情報
		texDescHeap->GetCPUDescriptorHandleForHeapStart()	// ヒープのどこに割り当てるか
		 // もしテクスチャビューが複数あるなら、ここは取得したハンドルからのオフセットを指定する必要がある
	);


	// シェーダーの読み込みと生成
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",
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
		return 0;
	}

	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",
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
		return 0;
	}

	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{	// 座標
			"POSITION",		// セマンティクス名
			0,				// 同じセマンティクス名の時に使うインデックス
			DXGI_FORMAT_R32G32B32_FLOAT,	// フォーマット（要素数とビット数で型を表す）
			0,								// 入力スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,	// データのオフセット位置
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	// 
			0				// 一度に描画するインスタンスの数
		},
		{	// uv
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};



	// グラフィクスパイプラインを作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	// 頂点シェーダー、ピクセルシェーダーを設定
	gpipeline.pRootSignature = nullptr; //後々設定
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();
	// サンプルマスクとラスタライザーの設定
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; //デフォルトのサンプルマスク（0xffffffff）
	gpipeline.RasterizerState.MultisampleEnable = false; //アンチエイリアスは（今は）使わない
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; //カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; //中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true; //深度方向のクリッピングは有効に

	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// 入力レイアウト設定
	gpipeline.InputLayout.pInputElementDescs = inputLayout;		//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);	//レイアウト配列の要素数

	// その他
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//カットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//三角形
	gpipeline.NumRenderTargets = 1;	//今は一つ（マルチレンダーではない）
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//0～1に正規化されたRGBA
	gpipeline.SampleDesc.Count = 1;		//サンプリングは１ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;	//クオリティは最低


	// ディスクリプタテーブルレンジの作成
	D3D12_DESCRIPTOR_RANGE descTblRange = {};
	descTblRange.NumDescriptors = 1;	
	descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	//種別はテクスチャ
	descTblRange.BaseShaderRegister = 0;
	descTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータの作成
	D3D12_ROOT_PARAMETER rootparam = {};
	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
	rootparam.DescriptorTable.NumDescriptorRanges = 1;

	// サンプラーの作成
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//横方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//縦方向の繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//奥行の繰り返し
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	//ボーダーは黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;	//線形補間
	//samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;	//補間しない
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;	//ミップマップ最大値
	samplerDesc.MinLOD = 0.0f;				//ミップマップ最小値
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//ピクセルシェーダーから見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;	//リサンプリングしない


	// ルートシグネチャの作成
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = &rootparam;	//ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 1;		//ルートパラメータの数
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob,
		&errorBlob);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Serializing Root Signature.");
		return 0;
	}

	ID3D12RootSignature* rootSignature = nullptr;
	result = _dev->CreateRootSignature(
		0,	//nodemask
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Root Signature");
		return 0;
	}
	rootSigBlob->Release();
	// 作成したルートシグネチャをパイプラインに設定
	gpipeline.pRootSignature = rootSignature;


	// グラフィクスパイプラインステートオブジェクトの生成
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Graphics Pipeline State.");
		return 0;
	}


	// ビューポートとシザー矩形
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;
	viewport.Height = window_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	D3D12_RECT scissorrect = {}; //今回は特に一部を切り抜いたりしない
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + window_width;
	scissorrect.bottom = scissorrect.top + window_height;


	MSG msg = {};
	
	while (true) {
		// メインループの処理
		{
			// 描画時の設定
			// ルートシグネチャの指定
			_cmdList->SetGraphicsRootSignature(rootSignature);
			// ディスクリプタヒープの指定
			_cmdList->SetDescriptorHeaps(1, &texDescHeap);
			// ルートパラメータとディスクリプタヒープの関連付け
			_cmdList->SetGraphicsRootDescriptorTable(
				0,	//ルートパラメータインデックス
				texDescHeap->GetGPUDescriptorHandleForHeapStart());	//ヒープアドレス

			// 1.コマンドアロケータとコマンドリストをクリア
			result = _cmdAllocator->Reset();
			// ここで判定するとなぜか E_FAIL が帰ってくる
			/*if (result != S_OK) {
				DebugOutputFormatString("Missed at Reset Allocator.");
				return 0;
			}*/
			result = _cmdList->Reset(_cmdAllocator, nullptr);
			/*if (result != S_OK) {
				DebugOutputFormatString("Missed at Reset Command List.");
				return 0;
			}*/

			// 2.レンダーターゲットをバックバッファにセット
			// 現在のバックバッファを取得
			UINT bbIdx = _swapchain->GetCurrentBackBufferIndex(); // バッファは２つなので、0か1のはず
			auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			// リソースバリアでバッファの使い道を GPU に通知する
			D3D12_RESOURCE_BARRIER BarrierDesc = {};
			BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; //遷移
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
			BarrierDesc.Transition.Subresource = 0;
			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			_cmdList->ResourceBarrier(1, &BarrierDesc); //バリア指定実行
			// レンダーターゲットとして指定する
			_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

			// 3.レンダーターゲットを指定色でクリア
			float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f }; //黄色
			_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

			// 描画命令
			_cmdList->SetPipelineState(_pipelinestate);
			_cmdList->SetGraphicsRootSignature(rootSignature);
			_cmdList->RSSetViewports(1, &viewport);
			_cmdList->RSSetScissorRects(1, &scissorrect);
			_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			_cmdList->IASetVertexBuffers(0, 1, &vbView);
			_cmdList->IASetIndexBuffer(&ibView);
			_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			// 4.レンダーターゲットをクローズ
			_cmdList->Close();

			// 5.たまったコマンドをコマンドリストに投げる
			// コマンドリスト実行
			ID3D12CommandList* cmdLists[] = { _cmdList };
			_cmdQueue->ExecuteCommandLists(1, cmdLists);
			// フェンスを作成しておく
			ID3D12Fence* _fence = nullptr;
			UINT64 _fenceVal = 0;
			result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
			// GPUの処理が完了するまで待つ
			_cmdQueue->Signal(_fence, ++_fenceVal);
			if (_fence->GetCompletedValue() != _fenceVal) {
				// イベントハンドルを取得
				auto event = CreateEvent(nullptr, false, false, nullptr);

				_fence->SetEventOnCompletion(_fenceVal, event);

				// イベントが発生するまで待機
				WaitForSingleObject(event, INFINITE);

				// イベントハンドルを閉じる
				CloseHandle(event);
			}
			while (_fence->GetCompletedValue() != _fenceVal) { ; }
			// クリア
			result = _cmdAllocator->Reset();
			if (result != S_OK) {
				DebugOutputFormatString("Missed at Reset Allocator.");
				return 0;
			}
			result = _cmdList->Reset(_cmdAllocator, nullptr);
			if (result != S_OK) {
				DebugOutputFormatString("Missed at Reset CommandList.");
				return 0;
			}

			// 6.スワップチェーンのフリップ処理
			// 状態遷移
			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			result = _swapchain->Present(1, 0);
			if (result != S_OK) {
				DebugOutputFormatString("Missed at Present Swapchain.");
				return 0;
			}


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
