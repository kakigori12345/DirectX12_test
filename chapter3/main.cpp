#include <Windows.h>
#ifdef _DEBUG 
#include < iostream >

#endif using namespace std;

// @brief �R���\�[�� ��� �� �t�H�[�}�b�g �t�� ������ �� �\�� 
// @param format �t�H�[�}�b�g�i% d �Ƃ�% f �Ƃ� �́j 
// @param �� �� ���� 
// @remarks ���� �֐� �� �f�o�b�O �p �ł��B �f�o�b�O �� �ɂ��� ���� �� �܂� ��
void DebugOutputFormatString( const char* format, ...) { 
#ifdef _DEBUG
	va_list valist;
	va_start( valist, format);
	vprintf_s( format, valist);
	va_end(valist);
#endif
}

// �ʓ| �� ���� ���� �Ȃ��� �� ���� �Ȃ� �֐�
LRESULT WindowProcedure( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	// �E�B���h�E �� �j�� �� �� ���� �Ă� ���
	if (msg == WM_DESTROY) { PostQuitMessage( 0);
	// OS �ɑ΂��āu ������ �� �A�v�� �� �I���v �� �`����
	return 0;
	}
	
	return DefWindowProc(hwnd, msg, wparam, lparam); // ���� �� ���� �� �s��
}



#ifdef _DEBUG
int main() {
#else 
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	// �E�B���h�E �N���X �� ������ �o�^
	WNDCLASSEX w = {};

	int window_width = 800;
	int window_height = 640;
	w.cbSize = sizeof( WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[�� �o�b�N �֐� �� �w��
	w.lpszClassName = ("DX12Sample"); // �A�v���P�[�V���� �N���X ���i �K�� �� �悢�j
	w.hInstance = GetModuleHandle(nullptr); // �n���h�� �� �擾
	RegisterClassEx(&w); // �A�v���P�[�V���� �N���X�i �E�B���h�E �N���X �� �w�� �� OS �� �`����j
	RECT wrc = { 0, 0, window_width, window_height};// �E�B���h�E�T�C�Y �� ���߂�
													 
	// �֐� �� �g�� �� �E�B���h�E �� �T�C�Y �� �␳ ����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E �I�u�W�F�N�g �� ����
	HWND hwnd = CreateWindow(
		w. lpszClassName,// �N���X �� �w��
		("DX12�e�X�g"), // �^�C�g�� �o�[ �� ����
		WS_OVERLAPPEDWINDOW, // �^�C�g�� �o�[ �� ���E�� �� ���� �E�B���h�E
		CW_USEDEFAULT, // �\�� x ���W �� OS �� �� �C��
		CW_USEDEFAULT, // �\�� y ���W �� OS �� �� �C��
		wrc.right - wrc.left, // �E�B���h�E ��
		wrc.bottom - wrc.top, // �E�B���h�E ��
		nullptr, // �e �E�B���h�E �n���h��
		nullptr, // ���j���[ �n���h��
		w.hInstance, // �Ăяo�� �A�v���P�[�V���� �n���h��
		nullptr); // �ǉ� �p�����[�^�[
	
	// �E�B���h�E �\��
	ShowWindow( hwnd, SW_SHOW);

	MSG msg = {};
	
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg); DispatchMessage(&msg);
		}

		//�A�v���P�[�V���� �� �I��� �Ƃ� �� message �� WM_ QUIT �� �Ȃ�
		if (msg. message == WM_QUIT) {
			break;
		}
	}
	
	//���� �N���X �� �g�� �Ȃ� �̂� �o�^ ���� ����
	UnregisterClass( w. lpszClassName, w. hInstance);




	DebugOutputFormatString(" Show window test.");
	//getchar();
	return 0;
}
