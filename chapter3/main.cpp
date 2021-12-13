#include <Windows.h>
#ifdef _DEBUG 
#include < iostream >

#endif using namespace std;

// @brief コンソール 画面 に フォーマット 付き 文字列 を 表示 
// @param format フォーマット（% d とか% f とか の） 
// @param 可変 長 引数 
// @remarks この 関数 は デバッグ 用 です。 デバッグ 時 にしか 動作 し ませ ん
void DebugOutputFormatString( const char* format, ...) { 
#ifdef _DEBUG
	va_list valist;
	va_start( valist, format);
	vprintf_s( format, valist);
	va_end(valist);
#endif
}

// 面倒 だ けど 書か なけれ ば いけ ない 関数
LRESULT WindowProcedure( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	// ウィンドウ が 破棄 さ れ たら 呼ば れる
	if (msg == WM_DESTROY) { PostQuitMessage( 0);
	// OS に対して「 もうこ の アプリ は 終わる」 と 伝える
	return 0;
	}
	
	return DefWindowProc(hwnd, msg, wparam, lparam); // 既定 の 処理 を 行う
}



#ifdef _DEBUG
int main() {
#else 
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	// ウィンドウ クラス の 生成＆ 登録
	WNDCLASSEX w = {};

	int window_width = 800;
	int window_height = 640;
	w.cbSize = sizeof( WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // コール バック 関数 の 指定
	w.lpszClassName = ("DX12Sample"); // アプリケーション クラス 名（ 適当 で よい）
	w.hInstance = GetModuleHandle(nullptr); // ハンドル の 取得
	RegisterClassEx(&w); // アプリケーション クラス（ ウィンドウ クラス の 指定 を OS に 伝える）
	RECT wrc = { 0, 0, window_width, window_height};// ウィンドウサイズ を 決める
													 
	// 関数 を 使っ て ウィンドウ の サイズ を 補正 する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウ オブジェクト の 生成
	HWND hwnd = CreateWindow(
		w. lpszClassName,// クラス 名 指定
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
	ShowWindow( hwnd, SW_SHOW);

	MSG msg = {};
	
	while (true) {
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
