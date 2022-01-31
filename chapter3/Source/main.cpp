#include "PreCompileHeader.h"

#include "Application.h"
#include "Dx12Wrapper.h"
#include "PMD/PMDRenderer.h"

#include "Util/Utility.h"


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------
namespace {
	bool InitializeSingleton();
	void FinalizeSingleton();
}


#ifdef _DEBUG
int main() {
#else 
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	if (!InitializeSingleton()) {
		return -1;
	}

	// アプリ実行
	Application* app = Application::Instance();
	app->Run();
	app->Terminate();

	FinalizeSingleton();

	return 0;
}


namespace {
	bool InitializeSingleton() {
		// シングルトンの初期化 TODO:シングルトンの初期化する処理は別関数にまとめたい
		Application::Create();
		Dx12Wrapper::Create();
		PMDRenderer::Create();

		Application* app = Application::Instance();
		Dx12Wrapper* dx12 = Dx12Wrapper::Instance();
		if (!app->Init()) {
			DebugOutputFormatString("Application の初期化に失敗.");
			return false;
		}
		if (!dx12->Init(app->GetWindowHandle())) {
			DebugOutputFormatString("Dx12Wrapper の初期化に失敗.");
			return false;
		}
		if (!PMDRenderer::Instance()->Init(dx12->GetDevice())) {
			DebugOutputFormatString("PMDRenderer の初期化に失敗.");
			return false;
		}

		return true;
	}

	void FinalizeSingleton() {
		// シングルトン終了
		PMDRenderer::Destroy();
		Dx12Wrapper::Destroy();
		Application::Destroy();
	}
}
