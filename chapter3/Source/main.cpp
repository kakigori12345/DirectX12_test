#include "PreCompileHeader.h"

#include "Application.h"
#include "Dx12Wrapper.h"
#include "PMD/PMDRenderer.h"

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

	PMDRenderer::Create();

	app->Run();
	app->Terminate();

	// �V���O���g���I��
	PMDRenderer::Destroy();
	Dx12Wrapper::Destroy();
	Application::Destroy();

	return 0;
}
