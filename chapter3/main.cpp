#include <Windows.h>
#include <vector>

// Direct3D
#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment( lib, "d3d12.lib")
#pragma comment( lib, "dxgi.lib")

// �w���p�[
#include <d3dx12.h>

// �V�F�[�_�[�̃R���p�C��
#include <d3dcompiler.h>
#pragma comment( lib, "d3dcompiler.lib")

// ���w�֐�
#include <DirectXMath.h>

// DirectXTex���C�u����
#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")


#ifdef _DEBUG 
#include < iostream >

#endif
using namespace std;
using namespace DirectX;

namespace {

	// �萔
	int window_width = 800;
	int window_height = 480;

	// ���_���
	struct Vertex {
		XMFLOAT3 pos;	// xyz ���W
		XMFLOAT2 uv;	// uv ���W
	};

	// �e�N�X�`���f�[�^
	struct TexRGBA {
		unsigned char R, G, B, A;
	};

	// PMD �w�b�_�\����
	struct PMDHeader {
		float version;
		char modelName[20];
		char comment[256];
	};

	// PMD ���_�\����
	struct PMDVertex {
		XMFLOAT3 pos;				// ���_���(12)
		XMFLOAT3 normal;			// �@���x�N�g��(12)
		XMFLOAT2 uv;				// uv ���W(8)
		unsigned short boneNo[2];	// �{�[���ԍ�(4)
		unsigned char boneWeigth;	// �{�[���e���x(1)
		unsigned char edgeFlag;		// �֊s���t���O(1)
	};
	constexpr size_t pmdVertexSize = 38;	// ���_�������̃T�C�Y

	// PMD �}�e���A���\����
	struct PMDMaterial {
		XMFLOAT3 diffuse;	// �f�B�q���[�Y�F
		float alpha;		// �f�B�q���[�Y��
		float specularity;	// �X�y�L�����̋����i��Z�l�j
		XMFLOAT3 specular;	// �X�y�L�����F
		XMFLOAT3 ambient;	// �A���r�G���g�F
		unsigned char toonIdx;	// �g�D�[���ԍ�
		unsigned char edgeFlag;	// �}�e���A�����Ƃ̗֊s���t���O
		unsigned char padding[2];	//�p�f�B���O
		unsigned int indicesNum;	// ���̃}�e���A�������蓖�Ă���C���f�b�N�X��
		char texFilePath[20];		// �e�N�X�`���t�@�C���p�X + ��
	}; // 70�o�C�g�̂͂������A�p�f�B���O�ɂ��72�o�C�g�ɂȂ�

		// �V�F�[�_�[���ɓ�������}�e���A���f�[�^
		struct MaterialForHlsl {
			XMFLOAT3 diffuse;	// �f�B�q���[�Y�F
			float alpha;		// �f�B�q���[�Y��
			XMFLOAT3 specular;	// �X�y�L�����F
			float specularity;	// �X�y�L�����̋����i��Z�l�j
			XMFLOAT3 ambient;	// �A���r�G���g�F
		};

		// ����ȊO�̃}�e���A���f�[�^
		struct AdditionalMaterial {
			std::string texPath;	// �e�N�X�`���t�@�C���p�X
			int toonIdx;			// �g�D�[���ԍ�
			bool edgeFlag;			// �}�e���A�����Ƃ̗֊s���t���O
		};

		// �S�̂��܂Ƃ߂�
		struct Material {
			unsigned int indicesNum;	// �C���f�b�N�X��
			MaterialForHlsl material;
			AdditionalMaterial additional;
		};

	// �V�F�[�_�[���ɓn�����߂̊�{�I�ȍs��f�[�^
	struct MatricesData {
		XMMATRIX world;		//���f���{�̂���]��������ړ��������肷��s��
		XMMATRIX viewproj;	//�r���[�ƃv���W�F�N�V�����̍����s��
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

	// sRGB �p�̃����_�[�^�[�Q�b�g�r���[�ݒ���쐬���Ă���
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//�K���}�␳����
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
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
			&rtvDesc,
			handle);
		// �n���h��������炷
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}


	// PMD �̓ǂݍ���

	// �w�b�_
	char signature[3] = {};		//�V�O�l�`��
	PMDHeader pmdheader = {};	//PMD �w�b�_
	FILE* fp;
	errno_t error = fopen_s(&fp, "data/Model/�����~�N.pmd", "rb");
	if (error != 0) {
		// �ڍׂȃG���[�\��
		DebugOutputFormatString("Missed at Reading PMD File");
		return 0;
	}
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	// ���_
	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, fp);
	std::vector<unsigned char> vertices(vertNum * pmdVertexSize);
	fread(vertices.data(), vertices.size(), 1, fp);

	// ���_�o�b�t�@�̍쐬
	D3D12_HEAP_PROPERTIES heapprop = {};
	D3D12_RESOURCE_DESC resdesc = {};
	heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());
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
	unsigned char* vertMap = nullptr;
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
	vbView.SizeInBytes = vertices.size(); //�S�o�C�g��
	vbView.StrideInBytes = pmdVertexSize; //�P���_������̃o�C�g��


	// �C���f�b�N�X���쐬
	std::vector<unsigned short> indices;
	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	// �C���f�b�N�X�o�b�t�@�̍쐬
	ID3D12Resource* idxBuff = nullptr;
	resdesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(indices.size()) * sizeof(indices[0]));
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
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);


	// �}�e���A������ǂݍ���
	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, fp);
	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1,
		fp
	);

	fclose(fp);

	std::vector<Material> materials(pmdMaterials.size());
	// �R�s�[
	for (int i = 0; i < pmdMaterials.size(); ++i) {
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
	}

	// �V�F�[�_�Ƀ}�e���A������]������
	
	// �}�e���A���o�b�t�@���쐬
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	D3D12_HEAP_PROPERTIES materialHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC materialDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * materialNum);

	ID3D12Resource* materialBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&materialBuff)
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Material Buffer.");
		return 0;
	}

	// �}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Mapping Material.");
		return 0;
	}

	for (auto& m : materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;	//�f�[�^�R�s�[
		mapMaterial += materialBuffSize;	//���̃A���C�����g�ʒu�܂Ői�߂�
	}

	materialBuff->Unmap(0, nullptr);

	// �}�e���A���p�f�B�X�N���v�^�q�[�v�ƃr���[�̍쐬
	ID3D12DescriptorHeap* materialDescHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materialNum;	//�}�e���A�������w�肵�Ă���
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(
		&matDescHeapDesc, IID_PPV_ARGS(&materialDescHeap)
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating DescriptorHeap For Material.");
		return 0;
	}

	// �r���[�̍쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBuffSize;
	// �f�B�X�N���v�^�q�[�v�̐擪�A�h���X���L�^
	auto matDescHeapHandle = materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < materialNum; ++i) {
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapHandle);
		matCBVDesc.BufferLocation += materialBuffSize;
	}


	// �e�N�X�`���ɉ摜�f�[�^��p�ӂ���
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(
		L"data/img/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Loading Image Data.");
		return 0;
	}
	const Image* img = scratchImg.GetImage(0, 0, 0);	//���f�[�^���o

	// �e�N�X�`���o�b�t�@�̍쐬
	D3D12_HEAP_PROPERTIES heappropTex = {};
	heappropTex.Type = D3D12_HEAP_TYPE_CUSTOM;	//����Ȑݒ�Ȃ̂� DEFAULT �ł� UPLOAD �ł��Ȃ�
	heappropTex.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;	//���C�g�o�b�N
	heappropTex.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;	//�]����L0,�܂�CPU�����璼�ڍs��
	heappropTex.CreationNodeMask = 0;	//�P��A�_�v�^�[�Ȃ̂� 0
	heappropTex.VisibleNodeMask = 0;
	// ���\�[�X�ݒ�
	D3D12_RESOURCE_DESC resDescTex = {};
	resDescTex.Format	= metadata.format;	// RGBA�t�H�[�}�b�g
	resDescTex.Width	= metadata.width;
	resDescTex.Height	= metadata.height;
	resDescTex.DepthOrArraySize		= metadata.arraySize;
	resDescTex.SampleDesc.Count		= 1;	//�ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	resDescTex.SampleDesc.Quality	= 0;	//�N�I���e�B�͍Œ�
	resDescTex.MipLevels	= metadata.mipLevels;
	resDescTex.Dimension	= static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDescTex.Layout		= D3D12_TEXTURE_LAYOUT_UNKNOWN;			//���C�A�E�g�͌��肵�Ȃ�
	resDescTex.Flags		= D3D12_RESOURCE_FLAG_NONE;				//���Ƀt���O�Ȃ�
	// ���\�[�X�̐���
	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heappropTex,
		D3D12_HEAP_FLAG_NONE,
		&resDescTex,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, //�e�N�X�`���p�w��
		nullptr,
		IID_PPV_ARGS(&texbuff));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Texture Resource.");
		return 0;
	}
	// �f�[�^�]��
	result = texbuff->WriteToSubresource(
		0,			// �T�u���\�[�X�C���f�b�N�X
		nullptr,	// �������ݗ̈�̎w��i����͐擪����S�̈�j
		img->pixels,	// �������݂����f�[�^�̃A�h���X
		img->rowPitch,	// �P�s������̃f�[�^�T�C�Y
		img->slicePitch	// �X���C�X������̃f�[�^�T�C�Y�i����͑S�T�C�Y�j
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Writing to Subresource.");
		return 0;
	}

	// �V�F�[�_�[���\�[�X�r���[
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//�V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0;		// �A�_�v�^�͈�Ȃ̂�0���Z�b�g
	descHeapDesc.NumDescriptors = 2;// SRV �� CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	//�V�F�[�_�[���\�[�X�r���[�p
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Descriptor Heap For ShaderReosurceView.");
		return 0;
	}
	// �V�F�[�_�[���\�[�X�r���[�����
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	// 2D�e�N�X�`��
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;	// �~�j�}�b�v�͎g�p���Ȃ��̂�1
	_dev->CreateShaderResourceView(
		texbuff,	// �r���[�Ɗ֘A�t����o�b�t�@
		&srvDesc,	// �e�N�X�`���ݒ���
		basicDescHeap->GetCPUDescriptorHandleForHeapStart()	// �q�[�v�̂ǂ��Ɋ��蓖�Ă邩
		 // �����e�N�X�`���r���[����������Ȃ�A�����͎擾�����n���h������̃I�t�Z�b�g���w�肷��K�v������
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Shader Resource View.");
		return 0;
	}

	// ���[���h�s��
	float angleY = XM_PIDIV4;
	XMMATRIX worldMat = XMMatrixRotationY(angleY);
	// �r���[�s��
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	XMMATRIX viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	// �v���W�F�N�V�����s��
	XMMATRIX projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,	//��p��90�x
		static_cast<float>(window_width) / static_cast<float>(window_height),	// �A�X�y�N�g��
		1.0f,	// �j�A�N���b�v
		100.0f	// �t�@�[�N���b�v
	);


	// �萔�o�b�t�@�[�̍쐬
	D3D12_HEAP_PROPERTIES constBufferHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC constBufferDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);
	ID3D12Resource* constBuff = nullptr;
	_dev->CreateCommittedResource(
		&constBufferHeap,
		D3D12_HEAP_FLAG_NONE,
		&constBufferDesc,	// 0xff�A���C�����g
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Const Buffer.");
		return 0;
	}
	// �}�b�v�Œ萔�R�s�[
	MatricesData* mapMatrix;	//�}�b�v��
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);

	// �萔�o�b�t�@�[�r���[���쐬����
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;
	// �f�B�X�N���v�^�q�[�v��ł̃������ʒu�i�n���h���j���擾
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart(); //���̏�Ԃ��ƃV�F�[�_���\�[�X�r���[�̈ʒu������
	// �C���N�������g���Ē萔�o�b�t�@�[�r���[�̈ʒu�������悤��
	basicHeapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// ���ۂɒ萔�o�b�t�@�[�r���[���쐬
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);


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
		{
			// �@��
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
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
		{
			// �{�[���ԍ�
			"BONE_NO",
			0,
			DXGI_FORMAT_R16G16_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			// �E�F�C�g
			"WEIGHT",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			// �֊s���t���O
			"EDGE_FLG",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};


	// �[�x�o�b�t�@�̍쐬
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = window_width;
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;	//�z��ł�3D�e�N�X�`���ł��Ȃ�
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResDesc.SampleDesc.Count = 1;	//�T���v����1�s�N�Z����������
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	
	// �[�x�l�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;	// �[��1.0f�ŃN���A
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	//32�r�b�g float �l�Ƃ��ăN���A

	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	//�[�x�n�������ݗp�Ɏg��
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer)
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating depth stensil buffer.");
		return 0;
	}

	// �[�x�o�b�t�@�[�r���[�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Depth Heap.");
		return 0;
	}

	// �[�x�r���[�̍쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(
		depthBuffer,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);



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

	// �[�x�l�̐ݒ�
	gpipeline.DepthStencilState.DepthEnable = true;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	//�s�N�Z���`�掞�ɁA�[�x�o�b�t�@�ɐ[�x�l����������
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			//�������ق����̗p
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// ���̑�
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//�J�b�g�Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//�O�p�`
	gpipeline.NumRenderTargets = 1;	//���͈�i�}���`�����_�[�ł͂Ȃ��j
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//0�`1�ɐ��K�����ꂽRGBA
	gpipeline.SampleDesc.Count = 1;		//�T���v�����O�͂P�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;	//�N�I���e�B�͍Œ�


	// �f�B�X�N���v�^�e�[�u�������W�̍쐬
	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};	//�e�N�X�`���ƒ萔�łQ��

	// �萔�P�i���W�ϊ��j
	descTblRange[0].NumDescriptors = 1;	
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;	//��ʂ͒萔
	descTblRange[0].BaseShaderRegister = 0;
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �萔�Q�i�}�e���A���j
	descTblRange[1].NumDescriptors = 1;
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;	//��ʂ͒萔
	descTblRange[1].BaseShaderRegister = 1;
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	// ���[�g�p�����[�^�̍쐬
	D3D12_ROOT_PARAMETER rootparam[2] = {};

	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];
	rootparam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


	// �T���v���[�̍쐬
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//�������̌J��Ԃ�
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//�c�����̌J��Ԃ�
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//���s�̌J��Ԃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	//�{�[�_�[�͍�
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;	//���`���
	//samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;	//��Ԃ��Ȃ�
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;	//�~�b�v�}�b�v�ő�l
	samplerDesc.MinLOD = 0.0f;				//�~�b�v�}�b�v�ŏ��l
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//�s�N�Z���V�F�[�_�[���猩����
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;	//���T���v�����O���Ȃ�


	// ���[�g�V�O�l�`���̍쐬
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootparam;	//���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 2;		//���[�g�p�����[�^�̐�
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

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
		{// �`�掞�̐ݒ�
			// ���[�g�V�O�l�`���̎w��
			_cmdList->SetGraphicsRootSignature(rootSignature);

			// �f�B�X�N���v�^�q�[�v�̎w��
			_cmdList->SetDescriptorHeaps(1, &basicDescHeap);

			// ���[�g�p�����[�^�ƃf�B�X�N���v�^�q�[�v�̊֘A�t��
			auto heapHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
			// �e�N�X�`���p
			_cmdList->SetGraphicsRootDescriptorTable(0, heapHandle);
			heapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			// �}�e���A���p
			_cmdList->SetGraphicsRootDescriptorTable(1, materialDescHeap->GetGPUDescriptorHandleForHeapStart());
		}

		{ // �s��v�Z
			angleY += 0.1f;
			worldMat = XMMatrixRotationY(angleY);
			mapMatrix->world = worldMat;
			mapMatrix->viewproj = viewMat * projMat;
		}

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
		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		_cmdList->ResourceBarrier(1, &BarrierDesc); //�o���A�w����s
		// �[�x�o�b�t�@�r���[���֘A�t��
		auto dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		// �����_�[�^�[�Q�b�g�Ƃ��Ďw�肷��
		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvHandle);

		// 3.�����_�[�^�[�Q�b�g���w��F�ŃN���A
		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; //���F
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		// �`�施��
		_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		_cmdList->SetPipelineState(_pipelinestate);
		_cmdList->SetGraphicsRootSignature(rootSignature);
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);
		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

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


		// �[�x�o�b�t�@�̃N���A
		_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


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
