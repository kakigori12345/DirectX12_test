//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Application.h"
#include "PMD/PMDActor.h" //TODO:Rendererができたらいらない

// Windows
#include <Windows.h>
#include <map>

// シェーダーのコンパイル
#include <d3dcompiler.h>

// DirectXTexライブラリ
#include <DirectXTex.h>

// その他
#include "Util/Utility.h"

// リファクタ
#include "Dx12Wrapper.h"


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;


//-----------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------
namespace {
	// 定数
	int window_width = 1280;
	int window_height = 760;
}

// 関数定義
namespace {
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


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

//! @brief コンストラクタ
Application::Application() {

}

//! @brief デストラクタ
Application::~Application() {

}

// シングルトン
SINGLETON_CPP(Application)


bool Application::Init() {
	HRESULT result = S_OK;

	// ウィンドウ クラス の 生成＆ 登録
	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure; // コール バック 関数 の 指定
	window.lpszClassName = L"DX12Sample"; // アプリケーション クラス 名（ 適当 で よい）
	window.hInstance = GetModuleHandle(nullptr); // ハンドル の 取得
	RegisterClassEx(&window); // アプリケーション クラス（ ウィンドウ クラス の 指定 を OS に 伝える）
	RECT wrc = { 0, 0, window_width, window_height };// ウィンドウサイズ を 決める

	// 関数 を 使っ て ウィンドウ の サイズ を 補正 する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウ オブジェクト の 生成
	HWND hwnd = CreateWindow(
		window.lpszClassName,// クラス 名 指定
		L"DX12テスト", // タイトル バー の 文字
		WS_OVERLAPPEDWINDOW, // タイトル バー と 境界線 が ある ウィンドウ
		CW_USEDEFAULT, // 表示 x 座標 は OS に お 任せ
		CW_USEDEFAULT, // 表示 y 座標 は OS に お 任せ
		wrc.right - wrc.left, // ウィンドウ 幅
		wrc.bottom - wrc.top, // ウィンドウ 高
		nullptr, // 親 ウィンドウ ハンドル
		nullptr, // メニュー ハンドル
		window.hInstance, // 呼び出し アプリケーション ハンドル
		nullptr); // 追加 パラメーター

	// 作成したウィンドウの情報を取得
	WindowInfo wInfo = GetWindowInfo(hwnd);

	// ウィンドウ 表示
	ShowWindow(hwnd, SW_SHOW);


	// 各クラスを初期化
	Dx12Wrapper* dxWrapper = Dx12Wrapper::Instance();
	if (!dxWrapper->Init(hwnd)) {
		DebugOutputFormatString("Dx12Wrapper の初期化に失敗.");
		return 0;
	}

	// 必要なデバイスを一時的に取得
	// TODO: ラッパーが完成したらこれらは必要なくなるはずなので消す
	ID3D12Device* _dev = dxWrapper->GetDevice();

	//------------------------------------------------------
	// ここから↓がレンダラーの仕事な気がする
	//------------------------------------------------------

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
		{
			// 法線
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
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
		{
			// ボーン番号
			"BONE_NO",
			0,
			DXGI_FORMAT_R16G16_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			// ウェイト
			"WEIGHT",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			// 輪郭線フラグ
			"EDGE_FLG",
			0,
			DXGI_FORMAT_R8_UINT,
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
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(dxWrapper->_vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(dxWrapper->_psBlob.Get());
	// サンプルマスクとラスタライザーの設定
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; //デフォルトのサンプルマスク（0xffffffff）
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; //カリングしない

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// 入力レイアウト設定
	gpipeline.InputLayout.pInputElementDescs = inputLayout;		//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);	//レイアウト配列の要素数

	// 深度値の設定
	gpipeline.DepthStencilState.DepthEnable = true;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	//ピクセル描画時に、深度バッファに深度値を書き込む
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			//小さいほうを採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// その他
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//カットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//三角形
	gpipeline.NumRenderTargets = 1;	//今は一つ（マルチレンダーではない）
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//0～1に正規化されたRGBA
	gpipeline.SampleDesc.Count = 1;		//サンプリングは１ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;	//クオリティは最低


	// ディスクリプタテーブルレンジの作成
	//メモ：ここでの分け方は、同じ種類かつ同じレジスタならまとめているのだと思う。
	// 	    テクスチャは種類が全部同じで、かつ扱うレジスタも同じなので、複数のディスクリプタを使っている。	
	CD3DX12_DESCRIPTOR_RANGE descTblRange[3] = {};	//テクスチャと定数で２つ
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // 座標変換[b0]
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // マテリアル[b1]
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // テクスチャ４つ

	// ルートパラメータの作成
	CD3DX12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);	// 座標変換
	rootparam[1].InitAsDescriptorTable(2, &descTblRange[1]);	// マテリアル周り

	// サンプラーの作成
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	// ルートシグネチャの作成
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootparam;	//ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 2;		//ルートパラメータの数
	rootSignatureDesc.pStaticSamplers = samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 2;

	result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob,
		&(dxWrapper->errorBlob));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Serializing Root Signature.");
		return 0;
	}

	result = _dev->CreateRootSignature(
		0,	//nodemask
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(rootSignature.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Root Signature");
		return 0;
	}
	// 作成したルートシグネチャをパイプラインに設定
	gpipeline.pRootSignature = rootSignature.Get();


	// グラフィクスパイプラインステートオブジェクトの生成
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Graphics Pipeline State.");
		return 0;
	}


	// ビューポートとシザー矩形
	viewport = CD3DX12_VIEWPORT { dxWrapper->_backBuffers[0] };
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + wInfo.width;
	scissorrect.bottom = scissorrect.top + wInfo.height;


	// ワールド行列
	angleY = 0;// XM_PIDIV4;
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

	return true;
}


void Application::Run() {
	Dx12Wrapper* dxWrapper = Dx12Wrapper::Instance();
	ID3D12Device* _dev = dxWrapper->GetDevice();
	IDXGISwapChain4* _swapchain = dxWrapper->GetSwapchain();
	ID3D12GraphicsCommandList* _cmdList = dxWrapper->GetCommandList();

	// 仮でモデルを作成
	PMDActor actor("data/Model/初音ミクmetal.pmd");
	if (!actor.Init(_dev)) {
		DebugOutputFormatString("Failed Creating Model");
		return;
	}

	MSG msg = {};

	while (true) {
		{ // 行列計算
			angleY += 0.01f;
			worldMat = XMMatrixRotationY(angleY);
			dxWrapper->mapMatrix->world = worldMat;
			dxWrapper->mapMatrix->view = viewMat;
			dxWrapper->mapMatrix->proj = projMat;
			dxWrapper->mapMatrix->eye = eye;
		}

		// 2.レンダーターゲットをバックバッファにセット
		// 現在のバックバッファを取得
		SIZE_T bbIdx = _swapchain->GetCurrentBackBufferIndex(); // バッファは２つなので、0か1のはず

		// リソースバリアでバッファの使い道を GPU に通知する
		D3D12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			dxWrapper->_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		_cmdList->ResourceBarrier(1, &BarrierDesc); //バリア指定実行

		// レンダーターゲットとして指定する
		auto rtvH = dxWrapper->rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		// 深度バッファビューを関連付け
		auto dsvHandle = dxWrapper->dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvHandle);
		// 深度バッファのクリア
		_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// 3.レンダーターゲットを指定色でクリア
		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; //白色
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		// 描画命令
		_cmdList->SetPipelineState(_pipelinestate.Get());
		_cmdList->SetGraphicsRootSignature(rootSignature.Get());
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &(actor.m_vbView));
		_cmdList->IASetIndexBuffer(&(actor.m_ibView));
		//_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		{// 描画時の設定
			// 行列変換
			ID3D12DescriptorHeap* bdh[] = { dxWrapper->basicDescHeap.Get() };
			_cmdList->SetDescriptorHeaps(1, bdh);
			_cmdList->SetGraphicsRootDescriptorTable(0, dxWrapper->basicDescHeap->GetGPUDescriptorHandleForHeapStart());

			// マテリアル
			ID3D12DescriptorHeap* mdh[] = { actor.m_materialDescHeap.Get() };
			_cmdList->SetDescriptorHeaps(1, mdh);

			auto materialHandle = actor.m_materialDescHeap->GetGPUDescriptorHandleForHeapStart();
			unsigned int idxOffset = 0;
			auto cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cbvsrvIncSize *= 5;	//CBV, SRV, SRV, SRV, SRV の５つ分

			for (auto& m : actor.m_materials) {
				_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
				_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

				// ヒープポインタとインデックスを次に進める
				materialHandle.ptr += cbvsrvIncSize;
				idxOffset += m.indicesNum;
			}
		}

		// リソースバリアでバッファの使い道を GPU に通知する
		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			dxWrapper->_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
		);
		_cmdList->ResourceBarrier(1, &BarrierDesc); //バリア指定実行

		// 4.レンダーターゲットをクローズ
		_cmdList->Close();


		dxWrapper->ExecuteCommandList();
		dxWrapper->ResetCommandList();

		// 6.スワップチェーンのフリップ処理
		// 状態遷移
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

		dxWrapper->SwapchainPresent();


		// メッセージ処理
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg); DispatchMessage(&msg);
		}

		//アプリケーション が 終わる とき に message が WM_QUIT に なる
		if (msg.message == WM_QUIT) {
			break;
		}
	}
}

void Application::Terminate() {
	//もう クラス は 使わ ない ので 登録 解除 する
	UnregisterClass(window.lpszClassName, window.hInstance);

	DebugOutputFormatString(" Show window test.");
}