#include "PreCompileHeader.h"

#include "Application.h"
#include "Dx12Wrapper.h"
#include "PMD/PMDRenderer.h"

#ifdef _DEBUG
int main() {
#else 
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	// シングルトンの初期化 TODO:シングルトンの初期化する処理は別関数にまとめたい
	Application::Create();
	Dx12Wrapper::Create();

	Application* app = Application::Instance();
	if (!app->Init()) {
		return -1;
	}

	PMDRenderer::Create();

	app->Run();
	app->Terminate();

	// シングルトン終了
	PMDRenderer::Destroy();
	Dx12Wrapper::Destroy();
	Application::Destroy();

	return 0;
}
