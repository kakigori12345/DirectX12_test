#include "PreCompileHeader.pch"

#include "Application.h"
#include "Dx12Wrapper.h"

#ifdef _DEBUG
int main() {
#else 
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	// �V���O���g���̏����� TODO:�V���O���g���̏��������鏈���͕ʊ֐��ɂ܂Ƃ߂���
	Application::Create();
	Dx12Wrapper::Create();

	Application* app = Application::Instance();
	if (!app->Init()) {
		return -1;
	}
	app->Run();
	app->Terminate();

	// �V���O���g���I��
	Dx12Wrapper::Destroy();
	Application::Destroy();

	return 0;
}
