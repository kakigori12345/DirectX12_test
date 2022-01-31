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

	// �A�v�����s
	Application* app = Application::Instance();
	app->Run();
	app->Terminate();

	FinalizeSingleton();

	return 0;
}


namespace {
	bool InitializeSingleton() {
		// �V���O���g���̏����� TODO:�V���O���g���̏��������鏈���͕ʊ֐��ɂ܂Ƃ߂���
		Application::Create();
		Dx12Wrapper::Create();
		PMDRenderer::Create();

		Application* app = Application::Instance();
		Dx12Wrapper* dx12 = Dx12Wrapper::Instance();
		if (!app->Init()) {
			DebugOutputFormatString("Application �̏������Ɏ��s.");
			return false;
		}
		if (!dx12->Init(app->GetWindowHandle())) {
			DebugOutputFormatString("Dx12Wrapper �̏������Ɏ��s.");
			return false;
		}
		if (!PMDRenderer::Instance()->Init(dx12->GetDevice())) {
			DebugOutputFormatString("PMDRenderer �̏������Ɏ��s.");
			return false;
		}

		return true;
	}

	void FinalizeSingleton() {
		// �V���O���g���I��
		PMDRenderer::Destroy();
		Dx12Wrapper::Destroy();
		Application::Destroy();
	}
}
