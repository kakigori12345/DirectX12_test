#include <Windows.h>
#include <vector>

#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment( lib, "d3d12.lib")
#pragma comment( lib, "dxgi.lib")


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
	// 3D�I�u�W�F�N�g�̐���
	ID3D12Device* _dev = nullptr;
	IDXGIFactory6* _dxgiFactory = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;


	// �t�@�N�g���[
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	
	// �A�_�v�^�[
	std::vector <IDXGIAdapter*> adapters; //�����ɃA�_�v�^�[��񋓂���
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}
	// �A�_�v�^�[�����ʂ��邽�߂̏����擾�iDXGI_ADAPTER�QDESC�\���́j
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc); // �A�_�v�^�[�̐����I�u�W�F�N�g�擾
		std::wstring strDesc = adesc.Description;

		// �T�������A�_�v�^�[�̖��O���m�F
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}

	// �f�o�C�X�I�u�W�F�N�g
	D3D12CreateDevice(tmpAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_dev));


	// �R�}���h���X�g�̍쐬�ƃR�}���h�A���P�[�^
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating CommandAllocator.");
		return 0;
	}
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating CommandList.");
		return 0;
	}

	// �L���[�̍쐬
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //�^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0; //�A�_�v�^�[��Ȃ̂łO�ł����i�炵���j
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// ����
	ID3D12CommandQueue* _cmdQueue = nullptr;
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating CommandQueue.");
		return 0;
	}


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
