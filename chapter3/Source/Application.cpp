//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Application.h"

// Windows
#include <Windows.h>

// その他
#include "Util/Utility.h"

// リファクタ
#include "Dx12Wrapper.h"
#include "PMD/PMDRenderer.h"
#include "PMD/PMDActor.h" //TODO:Rendererができたらいらない


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
}


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

//! @brief コンストラクタ
Application::Application()
	: window{}
	, m_hwnd(nullptr){

}

//! @brief デストラクタ
Application::~Application() {

}

// シングルトン
SINGLETON_CPP(Application)

HWND Application::GetWindowHandle() {
	return m_hwnd;
}

bool Application::Init() {
	HRESULT result = S_OK;

	// ウィンドウ クラス の 生成＆ 登録
	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure; // コール バック 関数 の 指定
	window.lpszClassName = L"DX12Sample"; // アプリケーション クラス 名
	window.hInstance = GetModuleHandle(nullptr); // ハンドル の 取得
	RegisterClassEx(&window); // アプリケーション クラス（ ウィンドウ クラス の 指定 を OS に 伝える）
	RECT wrc = { 0, 0, window_width, window_height };// ウィンドウサイズ を 決める

	// 関数 を 使っ て ウィンドウ の サイズ を 補正 する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウ オブジェクト の 生成
	m_hwnd = CreateWindow(
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
	WindowInfo wInfo = GetWindowInfo(m_hwnd);

	// ウィンドウ 表示
	ShowWindow(m_hwnd, SW_SHOW);

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
	PMDRenderer* renderer = PMDRenderer::Instance();

	ID3D12Device* _dev = dxWrapper->GetDevice();
	IDXGISwapChain4* _swapchain = dxWrapper->GetSwapchain();
	ID3D12GraphicsCommandList* _cmdList = dxWrapper->GetCommandList();

	// 仮でモデルを作成
	PMDActor actor("data/Model/初音ミクmetal.pmd");
	if (!actor.Init(_dev)) {
		DebugOutputFormatString("Failed Creating Model.");
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
		_cmdList->SetPipelineState(renderer->_pipelinestate.Get());
		_cmdList->SetGraphicsRootSignature(renderer->rootSignature.Get());
		_cmdList->RSSetViewports(1, &(dxWrapper->viewport));
		_cmdList->RSSetScissorRects(1, &(dxWrapper->scissorrect));
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
			TranslateMessage(&msg);
			DispatchMessage(&msg);
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