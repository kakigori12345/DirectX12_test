#include "PreCompileHeader.pch"

#include "Application.h"
#include "Dx12Wrapper.h"

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
	app->Run();
	app->Terminate();

	// シングルトン終了
	Dx12Wrapper::Destroy();
	Application::Destroy();

	return 0;
}
