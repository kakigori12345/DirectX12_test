//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Application.h"

// Windows
#include <Windows.h>

// ���̑�
#include "Util/Utility.h"

// ���t�@�N�^
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
	// �萔
	int window_width = 1280;
	int window_height = 760;
}

// �֐���`
namespace {
	// �ʓ| �� ���� ���� �Ȃ��� �� ���� �Ȃ� �֐�
	LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		// �E�B���h�E �� �j�� �� �� ���� �Ă� ���
		if (msg == WM_DESTROY) {
			PostQuitMessage(0);
			// OS �ɑ΂��āu ������ �� �A�v�� �� �I���v �� �`����
			return 0;
		}

		return DefWindowProc(hwnd, msg, wparam, lparam); // ���� �� ���� �� �s��
	}
}


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

//! @brief �R���X�g���N�^
Application::Application()
	: m_window{}
	, m_hwnd(nullptr){

}

//! @brief �f�X�g���N�^
Application::~Application() {

}

// �V���O���g��
SINGLETON_CPP(Application)

HWND Application::GetWindowHandle() {
	return m_hwnd;
}

bool Application::Init() {
	HRESULT result = S_OK;

	// �E�B���h�E �N���X �� ������ �o�^
	m_window.cbSize = sizeof(WNDCLASSEX);
	m_window.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[�� �o�b�N �֐� �� �w��
	m_window.lpszClassName = L"DX12Sample"; // �A�v���P�[�V���� �N���X ��
	m_window.hInstance = GetModuleHandle(nullptr); // �n���h�� �� �擾
	RegisterClassEx(&m_window); // �A�v���P�[�V���� �N���X�i �E�B���h�E �N���X �� �w�� �� OS �� �`����j
	RECT wrc = { 0, 0, window_width, window_height };// �E�B���h�E�T�C�Y �� ���߂�

	// �֐� �� �g�� �� �E�B���h�E �� �T�C�Y �� �␳ ����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E �I�u�W�F�N�g �� ����
	m_hwnd = CreateWindow(
		m_window.lpszClassName,// �N���X �� �w��
		L"DX12�e�X�g", // �^�C�g�� �o�[ �� ����
		WS_OVERLAPPEDWINDOW, // �^�C�g�� �o�[ �� ���E�� �� ���� �E�B���h�E
		CW_USEDEFAULT, // �\�� x ���W �� OS �� �� �C��
		CW_USEDEFAULT, // �\�� y ���W �� OS �� �� �C��
		wrc.right - wrc.left, // �E�B���h�E ��
		wrc.bottom - wrc.top, // �E�B���h�E ��
		nullptr, // �e �E�B���h�E �n���h��
		nullptr, // ���j���[ �n���h��
		m_window.hInstance, // �Ăяo�� �A�v���P�[�V���� �n���h��
		nullptr); // �ǉ� �p�����[�^�[

	// �쐬�����E�B���h�E�̏����擾
	WindowInfo wInfo = GetWindowInfo(m_hwnd);

	// �E�B���h�E �\��
	ShowWindow(m_hwnd, SW_SHOW);

	return true;
}


void Application::Run() {
	Dx12Wrapper* dxWrapper = Dx12Wrapper::Instance();
	PMDRenderer* renderer = PMDRenderer::Instance();

	ID3D12Device* _dev = dxWrapper->GetDevice();
	ID3D12GraphicsCommandList* _cmdList = dxWrapper->GetCommandList();

	// ���Ń��f�����쐬
	PMDActor actor("data/Model/�����~�Nmetal.pmd");
	if (!actor.Init(_dev)) {
		DebugOutputFormatString("Failed Creating Model.");
		return;
	}

	MSG msg = {};

	while (true) {
		// �X�V����
		dxWrapper->SetSceneData();
		actor.Update();


		// �`��O����
		dxWrapper->BeginDraw();
		renderer->BeginDraw(_cmdList);
		// �`��
		DrawActorInfo drawInfo;
		actor.GetDrawInfo(drawInfo);
		dxWrapper->Draw(drawInfo);
		// �`��㏈��
		dxWrapper->EndDraw();


		// ���b�Z�[�W����
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//�A�v���P�[�V���� �� �I��� �Ƃ� �� message �� WM_QUIT �� �Ȃ�
		if (msg.message == WM_QUIT) {
			break;
		}
	}
}

void Application::Terminate() {
	//���� �N���X �� �g�� �Ȃ� �̂� �o�^ ���� ����
	UnregisterClass(m_window.lpszClassName, m_window.hInstance);

	DebugOutputFormatString(" Show window test.");
}