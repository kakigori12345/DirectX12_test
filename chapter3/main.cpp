#include <Windows.h>
#include <vector>

// Direct3D
#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment( lib, "d3d12.lib")
#pragma comment( lib, "dxgi.lib")

// �V�F�[�_�[�̃R���p�C��
#include <d3dcompiler.h>
#pragma comment( lib, "d3dcompiler.lib")

// ���w�֐�
#include <DirectXMath.h>


#ifdef _DEBUG 
#include < iostream >

#endif
using namespace std;
using namespace DirectX;

namespace {

	// �萔
	int window_width = 800;
	int window_height = 640;

	// ���_���
	struct Vertex {
		XMFLOAT3 pos;	// xyz ���W
		XMFLOAT2 uv;	// uv ���W
	};



	// @brief �R���\�[�� ��� �� �t�H�[�}�b�g �t�� ������ �� �\�� 
	// @param format �t�H�[�}�b�g�i% d �Ƃ�% f �Ƃ� �́j 
	// @param �� �� ���� 
	// @remarks ���� �֐� �� �f�o�b�O �p �ł��B �f�o�b�O �� �ɂ��� ���� �� �܂� ��
	void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
		va_list valist;
		va_start(valist, format);
		vprintf_s(format, valist);
		va_end(valist);
#endif
	}

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

	// �f�o�b�O���C���[�̗L����
	void EnableDebugLayer() {
		ID3D12Debug* debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}


#ifdef _DEBUG
int main() {
#else 
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	// �E�B���h�E �N���X �� ������ �o�^
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[�� �o�b�N �֐� �� �w��
	w.lpszClassName = ("DX12Sample"); // �A�v���P�[�V���� �N���X ���i �K�� �� �悢�j
	w.hInstance = GetModuleHandle(nullptr); // �n���h�� �� �擾
	RegisterClassEx(&w); // �A�v���P�[�V���� �N���X�i �E�B���h�E �N���X �� �w�� �� OS �� �`����j
	RECT wrc = { 0, 0, window_width, window_height };// �E�B���h�E�T�C�Y �� ���߂�

	// �֐� �� �g�� �� �E�B���h�E �� �T�C�Y �� �␳ ����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E �I�u�W�F�N�g �� ����
	HWND hwnd = CreateWindow(
		w.lpszClassName,// �N���X �� �w��
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
	ShowWindow(hwnd, SW_SHOW);

#ifdef _DEBUG
	EnableDebugLayer();
#endif


	// 3D�I�u�W�F�N�g�̐���
	ID3D12Device* _dev = nullptr;
	IDXGIFactory6* _dxgiFactory = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;


	// �t�@�N�g���[
#ifdef _DEBUG
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif

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

	// �X���b�v�`�F�[���̍쐬
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH; // �o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // �t���b�v��͑��₩�ɔj��
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // �E�B���h�E�̃t���X�N���[���؂�ւ��\

	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating SwapChain.");
		return 0;
	}

	// �f�B�X�N���v�^�q�[�v�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; //�\���̂Q��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps)); //���̒i�K�ł͂܂� RTV �ł͂Ȃ�
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating DescriptorHeap.");
		return 0;
	}

	// �X���b�v�`�F�[���ƃr���[�̊֘A�t��
	std::vector<ID3D12Resource*> _backBuffers(swapchainDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (UINT idx = 0; idx < swapchainDesc.BufferCount; ++idx) {
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Getting BackBuffer.");
			return 0;
		}
		// ��قǍ쐬�����f�B�X�N���v�^�q�[�v�� RTV �Ƃ��Đݒ肷��
		_dev->CreateRenderTargetView(
			_backBuffers[idx],
			nullptr,
			handle);
		// �n���h��������炷
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}


	// �|���S���̕\��
	// ���_���쐬
	Vertex vertices[] = {
		{{-0.4f, -0.7f, 0.0f}, {0.0f, 1.0f}},	// ����
		{{-0.4f,  0.7f, 0.0f}, {0.0f, 0.0f}},	// ����
		{{ 0.4f, -0.7f, 0.0f}, {1.0f, 1.0f}},	// �E��
		{{ 0.4f,  0.7f, 0.0f}, {1.0f, 0.0f}},	// �E��
	};
	// ���_�o�b�t�@�̍쐬
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices); //���_��񂪓��邾���̃T�C�Y
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating CommittedResource.");
		return 0;
	}
	// ���_���̃R�s�[
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Mapping Vertex.");
		return 0;
	}
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr); // verMap �̏���n�����̂ŁA�}�b�v����������
	// ���_�o�b�t�@�r���[�̍쐬
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // �o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = sizeof(vertices); //�S�o�C�g��
	vbView.StrideInBytes = sizeof(vertices[0]); //�P���_������̃o�C�g��


	// �C���f�b�N�X���쐬
	unsigned short indices[] = {
		0, 1, 2,
		2, 1, 3
	};
	// �C���f�b�N�X�o�b�t�@�̍쐬
	ID3D12Resource* idxBuff = nullptr;
	resdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff) );
	// �o�b�t�@�ɃR�s�[
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);
	// �C���f�b�N�X�o�b�t�@�r���[���쐬
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);



	// �V�F�[�_�[�̓ǂݍ��݂Ɛ���
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //�f�o�b�O�p ����� �œK���Ȃ�
		0,
		&_vsBlob,
		&errorBlob);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Compiling Vertex Shader.");
		return 0;
	}

	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //�f�o�b�O�p ����� �œK���Ȃ�
		0,
		&_psBlob,
		&errorBlob);
	if (result != S_OK) {
		// �ڍׂȃG���[�\��
		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());
		std::copy_n(
			(char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			errstr.begin());
		OutputDebugStringA(errstr.c_str());

		DebugOutputFormatString("Missed at Compiling Pixel Shader.");
		return 0;
	}

	// ���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{	// ���W
			"POSITION",		// �Z�}���e�B�N�X��
			0,				// �����Z�}���e�B�N�X���̎��Ɏg���C���f�b�N�X
			DXGI_FORMAT_R32G32B32_FLOAT,	// �t�H�[�}�b�g�i�v�f���ƃr�b�g���Ō^��\���j
			0,								// ���̓X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,	// �f�[�^�̃I�t�Z�b�g�ʒu
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	// 
			0				// ��x�ɕ`�悷��C���X�^���X�̐�
		},
		{	// uv
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};



	// �O���t�B�N�X�p�C�v���C�����쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	// ���_�V�F�[�_�[�A�s�N�Z���V�F�[�_�[��ݒ�
	gpipeline.pRootSignature = nullptr; //��X�ݒ�
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();
	// �T���v���}�X�N�ƃ��X�^���C�U�[�̐ݒ�
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; //�f�t�H���g�̃T���v���}�X�N�i0xffffffff�j
	gpipeline.RasterizerState.MultisampleEnable = false; //�A���`�G�C���A�X�́i���́j�g��Ȃ�
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; //�J�����O���Ȃ�
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; //���g��h��Ԃ�
	gpipeline.RasterizerState.DepthClipEnable = true; //�[�x�����̃N���b�s���O�͗L����

	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// ���̓��C�A�E�g�ݒ�
	gpipeline.InputLayout.pInputElementDescs = inputLayout;		//���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout);	//���C�A�E�g�z��̗v�f��

	// ���̑�
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//�J�b�g�Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//�O�p�`
	gpipeline.NumRenderTargets = 1;	//���͈�i�}���`�����_�[�ł͂Ȃ��j
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//0�`1�ɐ��K�����ꂽRGBA
	gpipeline.SampleDesc.Count = 1;		//�T���v�����O�͂P�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;	//�N�I���e�B�͍Œ�


	// ���[�g�V�O�l�`���̍쐬
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob,
		&errorBlob);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Serializing Root Signature.");
		return 0;
	}

	ID3D12RootSignature* rootSignature = nullptr;
	result = _dev->CreateRootSignature(
		0,	//nodemask
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Root Signature");
		return 0;
	}
	rootSigBlob->Release();
	// �쐬�������[�g�V�O�l�`�����p�C�v���C���ɐݒ�
	gpipeline.pRootSignature = rootSignature;


	// �O���t�B�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̐���
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Graphics Pipeline State.");
		return 0;
	}


	// �r���[�|�[�g�ƃV�U�[��`
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;
	viewport.Height = window_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	D3D12_RECT scissorrect = {}; //����͓��Ɉꕔ��؂蔲�����肵�Ȃ�
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + window_width;
	scissorrect.bottom = scissorrect.top + window_height;


	MSG msg = {};
	
	while (true) {
		// ���C�����[�v�̏���
		{
			// 1.�R�}���h�A���P�[�^�ƃR�}���h���X�g���N���A
			result = _cmdAllocator->Reset();
			// �����Ŕ��肷��ƂȂ��� E_FAIL ���A���Ă���
			/*if (result != S_OK) {
				DebugOutputFormatString("Missed at Reset Allocator.");
				return 0;
			}*/
			result = _cmdList->Reset(_cmdAllocator, nullptr);
			/*if (result != S_OK) {
				DebugOutputFormatString("Missed at Reset Command List.");
				return 0;
			}*/

			// 2.�����_�[�^�[�Q�b�g���o�b�N�o�b�t�@�ɃZ�b�g
			// ���݂̃o�b�N�o�b�t�@���擾
			UINT bbIdx = _swapchain->GetCurrentBackBufferIndex(); // �o�b�t�@�͂Q�Ȃ̂ŁA0��1�̂͂�
			auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
			rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			// ���\�[�X�o���A�Ńo�b�t�@�̎g������ GPU �ɒʒm����
			D3D12_RESOURCE_BARRIER BarrierDesc = {};
			BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; //�J��
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
			BarrierDesc.Transition.Subresource = 0;
			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			_cmdList->ResourceBarrier(1, &BarrierDesc); //�o���A�w����s
			// �����_�[�^�[�Q�b�g�Ƃ��Ďw�肷��
			_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

			// 3.�����_�[�^�[�Q�b�g���w��F�ŃN���A
			float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f }; //���F
			_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

			// �`�施��
			_cmdList->SetPipelineState(_pipelinestate);
			_cmdList->SetGraphicsRootSignature(rootSignature);
			_cmdList->RSSetViewports(1, &viewport);
			_cmdList->RSSetScissorRects(1, &scissorrect);
			_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			_cmdList->IASetVertexBuffers(0, 1, &vbView);
			_cmdList->IASetIndexBuffer(&ibView);
			_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			// 4.�����_�[�^�[�Q�b�g���N���[�Y
			_cmdList->Close();

			// 5.���܂����R�}���h���R�}���h���X�g�ɓ�����
			// �R�}���h���X�g���s
			ID3D12CommandList* cmdLists[] = { _cmdList };
			_cmdQueue->ExecuteCommandLists(1, cmdLists);
			// �t�F���X���쐬���Ă���
			ID3D12Fence* _fence = nullptr;
			UINT64 _fenceVal = 0;
			result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
			// GPU�̏�������������܂ő҂�
			_cmdQueue->Signal(_fence, ++_fenceVal);
			if (_fence->GetCompletedValue() != _fenceVal) {
				// �C�x���g�n���h�����擾
				auto event = CreateEvent(nullptr, false, false, nullptr);

				_fence->SetEventOnCompletion(_fenceVal, event);

				// �C�x���g����������܂őҋ@
				WaitForSingleObject(event, INFINITE);

				// �C�x���g�n���h�������
				CloseHandle(event);
			}
			while (_fence->GetCompletedValue() != _fenceVal) { ; }
			// �N���A
			result = _cmdAllocator->Reset();
			if (result != S_OK) {
				DebugOutputFormatString("Missed at Reset Allocator.");
				return 0;
			}
			result = _cmdList->Reset(_cmdAllocator, nullptr);
			if (result != S_OK) {
				DebugOutputFormatString("Missed at Reset CommandList.");
				return 0;
			}

			// 6.�X���b�v�`�F�[���̃t���b�v����
			// ��ԑJ��
			BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			result = _swapchain->Present(1, 0);
			if (result != S_OK) {
				DebugOutputFormatString("Missed at Present Swapchain.");
				return 0;
			}


		}

		// ���b�Z�[�W����
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
