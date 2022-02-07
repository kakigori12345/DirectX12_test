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
#include "PMD/PMDActor.h"


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
	: m_window{}
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
	m_window.cbSize = sizeof(WNDCLASSEX);
	m_window.lpfnWndProc = (WNDPROC)WindowProcedure; // コール バック 関数 の 指定
	m_window.lpszClassName = L"DX12Sample"; // アプリケーション クラス 名
	m_window.hInstance = GetModuleHandle(nullptr); // ハンドル の 取得
	RegisterClassEx(&m_window); // アプリケーション クラス（ ウィンドウ クラス の 指定 を OS に 伝える）
	RECT wrc = { 0, 0, window_width, window_height };// ウィンドウサイズ を 決める

	// 関数 を 使っ て ウィンドウ の サイズ を 補正 する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウ オブジェクト の 生成
	m_hwnd = CreateWindow(
		m_window.lpszClassName,// クラス 名 指定
		L"DX12テスト", // タイトル バー の 文字
		WS_OVERLAPPEDWINDOW, // タイトル バー と 境界線 が ある ウィンドウ
		CW_USEDEFAULT, // 表示 x 座標 は OS に お 任せ
		CW_USEDEFAULT, // 表示 y 座標 は OS に お 任せ
		wrc.right - wrc.left, // ウィンドウ 幅
		wrc.bottom - wrc.top, // ウィンドウ 高
		nullptr, // 親 ウィンドウ ハンドル
		nullptr, // メニュー ハンドル
		m_window.hInstance, // 呼び出し アプリケーション ハンドル
		nullptr); // 追加 パラメーター

	// 作成したウィンドウの情報を取得
	WindowInfo wInfo = GetWindowInfo(m_hwnd);

	// ウィンドウ 表示
	ShowWindow(m_hwnd, SW_SHOW);

	return true;
}


void Application::Run() {
	Dx12Wrapper* dxWrapper = Dx12Wrapper::Instance();
	PMDRenderer* renderer = PMDRenderer::Instance();

	ID3D12Device* _dev = dxWrapper->GetDevice();
	ID3D12GraphicsCommandList* _cmdList = dxWrapper->GetCommandList();

	// 仮でモデルを作成
	PMDActor actor("data/Model/初音ミクmetal.pmd");
	if (!actor.Init(_dev)) {
		DebugOutputFormatString("Failed Creating Model.");
		return;
	}

	MSG msg = {};

	while (true) {
		// 更新処理
		dxWrapper->SetSceneData();
		actor.Update();


		// 描画前処理
		dxWrapper->BeginDraw();
		renderer->BeginDraw(_cmdList);
		// 描画
		DrawActorInfo drawInfo;
		actor.GetDrawInfo(drawInfo);
		dxWrapper->Draw(drawInfo);
		// 描画後処理
		dxWrapper->EndDraw();


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
	UnregisterClass(m_window.lpszClassName, m_window.hInstance);

	DebugOutputFormatString(" Show window test.");
}