#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------

// Direct3D
#include <DirectXMath.h>

//ComPtr
#include <wrl.h>

#include "Util/SingletonDef.h"


//! アプリケーションクラス（シングルトン）
class Application {
	SINGLETON_HEADER(Application)
	//----------------------------------------------------
	// メソッド
	//----------------------------------------------------
public:
	HWND GetWindowHandle();

	bool Init();

	void Run();

	void Terminate();


	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:
	WNDCLASSEX m_window;
	HWND m_hwnd;
};