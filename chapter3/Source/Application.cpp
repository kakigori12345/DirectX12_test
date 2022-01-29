//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Application.h"

// Windows
#include <Windows.h>
#include <map>

// Direct3D
//#pragma comment( lib, "d3d12.lib")

// �V�F�[�_�[�̃R���p�C��
#include <d3dcompiler.h>

// DirectXTex���C�u����
#include <DirectXTex.h>

// ���̑�
#include "Utility.h"

// ���t�@�N�^
#include "Dx12Wrapper.h"


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

#pragma pack(1)//��������1�o�C�g�p�b�L���O�c�A���C�����g�͔������Ȃ�
	// PMD �}�e���A���\����
	struct PMDMaterial {
		XMFLOAT3 diffuse;	// �f�B�q���[�Y�F
		float alpha;		// �f�B�q���[�Y��
		float specularity;	// �X�y�L�����̋����i��Z�l�j
		XMFLOAT3 specular;	// �X�y�L�����F
		XMFLOAT3 ambient;	// �A���r�G���g�F
		unsigned char toonIdx;	// �g�D�[���ԍ�
		unsigned char edgeFlag;	// �}�e���A�����Ƃ̗֊s���t���O

		// 2�o�C�g�̃p�f�B���O������

		unsigned int indicesNum;	// ���̃}�e���A�������蓖�Ă���C���f�b�N�X��
		char texFilePath[20];		// �e�N�X�`���t�@�C���p�X + ��
	}; // 70�o�C�g�̂͂������A�p�f�B���O�ɂ��72�o�C�g�ɂȂ�
#pragma pack()//1�o�C�g�p�b�L���O����
	static_assert(sizeof(PMDMaterial) == 70, "assertion error.");
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

	// �f�o�b�O���C���[�̗L����
	void EnableDebugLayer() {
		ID3D12Debug* debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}

	// �e�N�X�`���Ăяo���p�̊֐���p��
	using LoadLambda_t = function<HRESULT(const wstring& path, TexMetadata*, ScratchImage&)>;
	map<string, LoadLambda_t> loadLambdaTable;

	// �e�N�X�`�������[�h���ă��\�[�X���쐬����
	ID3D12Resource* LoadTextureFromFile(const string& texPath, ID3D12Device* dev) {

		// �ǂݍ��񂾃e�N�X�`����ۑ����Ă����R���e�i
		static map<string, ID3D12Resource*> resourceTable;

		// ���łɓǂݍ��ݍς݂Ȃ炻���Ԃ�
		auto it = resourceTable.find(texPath);
		if (it != resourceTable.end()) {
			// �e�[�u�����ɂ���̂Ń}�b�v���̃��\�[�X��Ԃ�
			return it->second;
		}

		// WIC�e�N�X�`���̃��[�h
		TexMetadata metadata = {};
		ScratchImage scratchImg = {};

		wstring wtexpath = GetWideStringFromString(texPath).c_str();
		string extension = GetExtension(texPath);

		// �t�@�C���p�X�̎w��A�g���q���Ȃ��ꍇ�̃G���[�`�F�b�N
		assert(wtexpath.size() != 0 && extension.size() != 0);

		auto result = loadLambdaTable[extension](
			wtexpath,
			&metadata,
			scratchImg
			);

		if (FAILED(result)) {
			return nullptr;
		}

		auto img = scratchImg.GetImage(0, 0, 0);	//���f�[�^���o

		// WriteToSubresource �œ]������p�̃q�[�v�ݒ�
		D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
			D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
			D3D12_MEMORY_POOL_L0);

		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			metadata.format,
			metadata.width,
			metadata.height,
			metadata.arraySize,
			metadata.mipLevels);

		// �o�b�t�@�쐬
		ID3D12Resource* texBuff = nullptr;
		result = dev->CreateCommittedResource(
			&texHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&texBuff)
		);

		if (FAILED(result)) {
			return nullptr;
		}

		result = texBuff->WriteToSubresource(
			0,
			nullptr,		//�S�̈�փR�s�[
			img->pixels,	//���f�[�^�A�h���X
			img->rowPitch,	//1���C���T�C�Y
			img->slicePitch	//�S�T�C�Y
		);

		if (FAILED(result)) {
			return nullptr;
		}

		// �e�[�u���ɕۑ����Ă���
		resourceTable[texPath] = texBuff;
		return texBuff;
	}


	// ���e�N�X�`���쐬
	ID3D12Resource* CreateWhiteTexture(ID3D12Device* dev) {
		D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
			D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
			D3D12_MEMORY_POOL_L0);

		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

		ID3D12Resource* whiteBuff = nullptr;
		auto result = dev->CreateCommittedResource(
			&texHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&whiteBuff)
		);

		if (FAILED(result)) {
			return nullptr;
		}

		std::vector<unsigned char> data(4 * 4 * 4);
		std::fill(data.begin(), data.end(), 0xff);	//�S��255�Ŗ��߂�

		// �f�[�^�]��
		result = whiteBuff->WriteToSubresource(
			0,
			nullptr,
			data.data(),
			4 * 4,
			data.size()
		);

		return whiteBuff;
	}


	// ���e�N�X�`���쐬
	ID3D12Resource* CreateBlackTexture(ID3D12Device* dev) {
		D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
			D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
			D3D12_MEMORY_POOL_L0);

		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

		ID3D12Resource* blackBuff = nullptr;
		auto result = dev->CreateCommittedResource(
			&texHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&blackBuff)
		);

		if (FAILED(result)) {
			return nullptr;
		}

		std::vector<unsigned char> data(4 * 4 * 4);
		std::fill(data.begin(), data.end(), 0x00);	//�S��0�Ŗ��߂�

		// �f�[�^�]��
		result = blackBuff->WriteToSubresource(
			0,
			nullptr,
			data.data(),
			4 * 4,
			data.size()
		);

		return blackBuff;
	}


	// �f�t�H���g�O���f�[�V�����e�N�X�`���i�g�D�[���p�j
	ID3D12Resource* CreateGrayGradationTexture(ID3D12Device* dev) {
		D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
			D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
			D3D12_MEMORY_POOL_L0);

		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

		ID3D12Resource* gradBuff = nullptr;
		auto result = dev->CreateCommittedResource(
			&texHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&gradBuff)
		);

		if (FAILED(result)) {
			return nullptr;
		}

		std::vector<unsigned char> data(4 * 256);
		auto it = data.begin();
		unsigned int c = 0xff;
		for (; it != data.end(); it += 4) {
			auto test1 = c << 24;
			auto test2 = c << 16;
			auto test3 = c << 8;
			auto col = (c << 24) | (c << 16) | (c << 8) | c;
			fill(it, it + 4, col);
			--c;
		}

		// �f�[�^�]��
		result = gradBuff->WriteToSubresource(
			0,
			nullptr,
			data.data(),
			4 * sizeof(unsigned int),
			sizeof(unsigned int) * data.size()
		);

		return gradBuff;
	}
}


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

std::unique_ptr<Application> Application::s_instance = nullptr;

Application* Application::Instance() {
	assert(s_instance);
	return s_instance.get();
}

void Application::Create() {
	s_instance = unique_ptr<Application>(new Application());
}

void Application::Destroy() {
	s_instance.reset();
}

bool Application::Init() {
	HRESULT result = S_OK;

	// �E�B���h�E �N���X �� ������ �o�^
	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[�� �o�b�N �֐� �� �w��
	window.lpszClassName = L"DX12Sample"; // �A�v���P�[�V���� �N���X ���i �K�� �� �悢�j
	window.hInstance = GetModuleHandle(nullptr); // �n���h�� �� �擾
	RegisterClassEx(&window); // �A�v���P�[�V���� �N���X�i �E�B���h�E �N���X �� �w�� �� OS �� �`����j
	RECT wrc = { 0, 0, window_width, window_height };// �E�B���h�E�T�C�Y �� ���߂�

	// �֐� �� �g�� �� �E�B���h�E �� �T�C�Y �� �␳ ����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E �I�u�W�F�N�g �� ����
	HWND hwnd = CreateWindow(
		window.lpszClassName,// �N���X �� �w��
		L"DX12�e�X�g", // �^�C�g�� �o�[ �� ����
		WS_OVERLAPPEDWINDOW, // �^�C�g�� �o�[ �� ���E�� �� ���� �E�B���h�E
		CW_USEDEFAULT, // �\�� x ���W �� OS �� �� �C��
		CW_USEDEFAULT, // �\�� y ���W �� OS �� �� �C��
		wrc.right - wrc.left, // �E�B���h�E ��
		wrc.bottom - wrc.top, // �E�B���h�E ��
		nullptr, // �e �E�B���h�E �n���h��
		nullptr, // ���j���[ �n���h��
		window.hInstance, // �Ăяo�� �A�v���P�[�V���� �n���h��
		nullptr); // �ǉ� �p�����[�^�[

	// �E�B���h�E �\��
	ShowWindow(hwnd, SW_SHOW);


	// �e�N���X��������
	Dx12Wrapper* dxWrapper = Dx12Wrapper::Instance();
	if (!dxWrapper->Init(hwnd)) {
		DebugOutputFormatString("Dx12Wrapper �̏������Ɏ��s.");
		return 0;
	}

	// �K�v�ȃf�o�C�X���ꎞ�I�Ɏ擾
	// TODO: ���b�p�[�����������炱���͕K�v�Ȃ��Ȃ�͂��Ȃ̂ŏ���
	ID3D12Device* _dev = dxWrapper->GetDevice();
	IDXGIFactory6* _dxgiFactory = dxWrapper->GetFactory();
	IDXGISwapChain4* _swapchain = dxWrapper->GetSwapchain();


	// �f�B�X�N���v�^�q�[�v�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; //�\���̂Q��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf())); //���̒i�K�ł͂܂� RTV �ł͂Ȃ�
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating DescriptorHeap.");
		return 0;
	}

	// sRGB �p�̃����_�[�^�[�Q�b�g�r���[�ݒ���쐬���Ă���
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//�K���}�␳����
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	// �X���b�v�`�F�[���ƃr���[�̊֘A�t��
	_backBuffers.resize(COMMAND_BUFFER_COUNT);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (UINT idx = 0; idx < COMMAND_BUFFER_COUNT; ++idx) {
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Getting BackBuffer.");
			return 0;
		}
		// ��قǍ쐬�����f�B�X�N���v�^�q�[�v�� RTV �Ƃ��Đݒ肷��
		rtvDesc.Format = _backBuffers[idx]->GetDesc().Format;
		_dev->CreateRenderTargetView(
			_backBuffers[idx],
			&rtvDesc,
			handle);
		// �n���h��������炷
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}





	//---------------------------------------------------------
	// �������火�����f�����Ƃ̏�������
	//---------------------------------------------------------


	// PMD �̓ǂݍ���
	string strModelPath = "data/Model/�����~�Nmetal.pmd";

	// �w�b�_
	char signature[3] = {};		//�V�O�l�`��
	PMDHeader pmdheader = {};	//PMD �w�b�_
	FILE* fp;
	errno_t error = fopen_s(&fp, strModelPath.c_str(), "rb");
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
	D3D12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());

	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf()));
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
	resdesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(indices.size()) * sizeof(indices[0]));
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf()));
	// �o�b�t�@�ɃR�s�[
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);
	// �C���f�b�N�X�o�b�t�@�r���[���쐬
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

	// �e�N�X�`���t�@�C���̎�ނ��ƂɕʁX�̓ǂݍ��݊֐����g�p����
	loadLambdaTable["sph"]
		= loadLambdaTable["spa"]
		= loadLambdaTable["bmp"]
		= loadLambdaTable["png"]
		= loadLambdaTable["jpg"]
		= [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};

	loadLambdaTable["tga"]
		= [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	loadLambdaTable["dds"]
		= [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};

	materials.resize(materialNum);
	vector<ID3D12Resource*> textureResources(materialNum, nullptr);
	vector<ID3D12Resource*> sphResources(materialNum, nullptr);
	vector<ID3D12Resource*> spaResources(materialNum, nullptr);
	vector<ID3D12Resource*> toonResources(materialNum, nullptr);

	{
		// �R�s�[
		for (unsigned int i = 0; i < materialNum; ++i) {
			materials[i].indicesNum = pmdMaterials[i].indicesNum;
			materials[i].material.diffuse = pmdMaterials[i].diffuse;
			materials[i].material.alpha = pmdMaterials[i].alpha;
			materials[i].material.specular = pmdMaterials[i].specular;
			materials[i].material.specularity = pmdMaterials[i].specularity;
			materials[i].material.ambient = pmdMaterials[i].ambient;
		}

		// �e�N�X�`��
		for (unsigned int i = 0; i < materialNum; ++i) {
			if (strlen(pmdMaterials[i].texFilePath) == 0) {
				textureResources[i] = nullptr;
				continue;
			}

			// �g�D�[�����\�[�X�̓ǂݍ���
			string toonFilePath = "data/toon/";
			char toonFileName[16];

			sprintf_s(toonFileName, "toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
			toonFilePath += toonFileName;

			toonResources[i] = LoadTextureFromFile(toonFilePath, _dev);

			// �e��e�N�X�`����ǂݍ���
			string texFileName = pmdMaterials[i].texFilePath;
			string sphFileName = "";
			string spaFileName = "";

			// �X�t�B�A�t�@�C�������݂��Ă��邩�`�F�b�N
			if (std::count(texFileName.begin(), texFileName.end(), '*') > 0) {
				// �X�v���b�^������̂ŁA�t�@�C��������
				auto namepair = SplitFileName(texFileName);
				if (GetExtension(namepair.first) == "sph") {
					texFileName = namepair.second;
					sphFileName = namepair.first;
				}
				else if (GetExtension(namepair.first) == "spa")
				{
					texFileName = namepair.second;
					spaFileName = namepair.first;
				}
				else {
					texFileName = namepair.first;
					if (GetExtension(namepair.second) == "sph") {
						sphFileName = namepair.second;
					}
					else if (GetExtension(namepair.second) == "spa") {
						spaFileName = namepair.second;
					}
				}
			}
			else {
				// �P��̃t�@�C���Ȃ̂ŁA�ǂ̎�ނ����肷��
				if (GetExtension(texFileName) == "sph") {
					sphFileName = texFileName;
					texFileName = "";
				}
				else if (GetExtension(texFileName) == "spa") {
					spaFileName = texFileName;
					texFileName = "";
				}
				else {
					texFileName = texFileName;
				}
			}

			// ���f���ƃe�N�X�`���p�X����A�v���O�������猩���e�N�X�`���p�X���擾
			if (texFileName != "") {
				auto texFilePath = GetTexturePathFromModelAndTexPath(strModelPath, texFileName.c_str());
				textureResources[i] = LoadTextureFromFile(texFilePath, _dev);
			}
			if (sphFileName != "") {
				auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
				sphResources[i] = LoadTextureFromFile(sphFilePath, _dev);
			}
			if (spaFileName != "") {
				auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
				spaResources[i] = LoadTextureFromFile(spaFilePath, _dev);
			}
		}
	}

	// �V�F�[�_�Ƀ}�e���A������]������

	// �}�e���A���o�b�t�@���쐬
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	D3D12_HEAP_PROPERTIES materialHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC materialDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(materialBuffSize) * materialNum);

	result = _dev->CreateCommittedResource(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(materialBuff.ReleaseAndGetAddressOf())
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
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materialNum * 5;	//�}�e���A�������w�肵�Ă���
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(
		&matDescHeapDesc, IID_PPV_ARGS(materialDescHeap.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating DescriptorHeap For Material.");
		return 0;
	}

	// �ʏ�e�N�X�`���r���[�쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	//2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1;

	// �r���[�̍쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBuffSize;
	// �f�B�X�N���v�^�q�[�v�̐擪�A�h���X���L�^
	auto matDescHeapHandle = materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	whiteTex = CreateWhiteTexture(_dev);
	blackTex = CreateBlackTexture(_dev);
	gradTex = CreateGrayGradationTexture(_dev);

	for (unsigned int i = 0; i < materialNum; ++i) {
		// �}�e���A���p�萔�o�b�t�@�r���[
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapHandle);

		matCBVDesc.BufferLocation += materialBuffSize;
		matDescHeapHandle.ptr += incSize;


		// �e�N�X�`���p�r���[
		if (textureResources[i] != nullptr) {
			// �ǂݍ��񂾃e�N�X�`��
			srvDesc.Format = textureResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				textureResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// �����e�N�X�`���Ŗ��ߍ��킹
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex.Get(), &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;


		// sph �p�r���[
		if (sphResources[i] != nullptr) {
			srvDesc.Format = sphResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				sphResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// �����e�N�X�`���Ŗ��ߍ��킹
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex.Get(), &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;


		// spa �p�r���[
		if (spaResources[i] != nullptr) {
			srvDesc.Format = spaResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				spaResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// �����e�N�X�`���Ŗ��ߍ��킹
			srvDesc.Format = blackTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				blackTex.Get(), &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;


		// �g�D�[�����\�[�X�p�r���[
		if (toonResources[i] != nullptr) {
			srvDesc.Format = toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				toonResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// �O���C�O���f�[�V�����Ŗ��ߍ��킹
			srvDesc.Format = gradTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				gradTex.Get(), &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;
	}




	//------------------------------------------------------
	// �������火�������_���[�̎d���ȋC������
	//------------------------------------------------------

	// �V�F�[�_�[���\�[�X�r���[
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//�V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0;		// �A�_�v�^�͈�Ȃ̂�0���Z�b�g
	descHeapDesc.NumDescriptors = 1;// CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	//�V�F�[�_�[���\�[�X�r���[�p

	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Descriptor Heap For ShaderReosurceView.");
		return 0;
	}


	// �萔�o�b�t�@�[�̍쐬
	D3D12_HEAP_PROPERTIES constBufferHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC constBufferDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	_dev->CreateCommittedResource(
		&constBufferHeap,
		D3D12_HEAP_FLAG_NONE,
		&constBufferDesc,	// 0xff�A���C�����g
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Const Buffer.");
		return 0;
	}
	// �}�b�v�Œ萔�R�s�[
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);

	// �萔�o�b�t�@�[�r���[���쐬����
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;
	// �f�B�X�N���v�^�q�[�v��ł̃������ʒu�i�n���h���j���擾
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart(); //���̏�Ԃ��ƃV�F�[�_���\�[�X�r���[�̈ʒu������
	// ���ۂɒ萔�o�b�t�@�[�r���[���쐬
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);


	// �V�F�[�_�[�̓ǂݍ��݂Ɛ���
	result = D3DCompileFromFile(
		L"Resource/BasicVertexShader.hlsl",
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
		L"Resource/BasicPixelShader.hlsl",
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

	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	//�[�x�n�������ݗp�Ɏg��
		&depthClearValue,
		IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating depth stensil buffer.");
		return 0;
	}

	// �[�x�o�b�t�@�[�r���[�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf()));
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
		depthBuffer.Get(),
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);



	// �O���t�B�N�X�p�C�v���C�����쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	// ���_�V�F�[�_�[�A�s�N�Z���V�F�[�_�[��ݒ�
	gpipeline.pRootSignature = nullptr; //��X�ݒ�
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());
	// �T���v���}�X�N�ƃ��X�^���C�U�[�̐ݒ�
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; //�f�t�H���g�̃T���v���}�X�N�i0xffffffff�j
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; //�J�����O���Ȃ�

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
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
	//�����F�����ł̕������́A������ނ��������W�X�^�Ȃ�܂Ƃ߂Ă���̂��Ǝv���B
	// 	    �e�N�X�`���͎�ނ��S�������ŁA���������W�X�^�������Ȃ̂ŁA�����̃f�B�X�N���v�^���g���Ă���B	
	CD3DX12_DESCRIPTOR_RANGE descTblRange[3] = {};	//�e�N�X�`���ƒ萔�łQ��
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // ���W�ϊ�[b0]
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // �}�e���A��[b1]
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // �e�N�X�`���S��

	// ���[�g�p�����[�^�̍쐬
	CD3DX12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);	// ���W�ϊ�
	rootparam[1].InitAsDescriptorTable(2, &descTblRange[1]);	// �}�e���A������

	// �T���v���[�̍쐬
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	// ���[�g�V�O�l�`���̍쐬
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootparam;	//���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 2;		//���[�g�p�����[�^�̐�
	rootSignatureDesc.pStaticSamplers = samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 2;

	result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob,
		&errorBlob);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Serializing Root Signature.");
		return 0;
	}

	result = _dev->CreateRootSignature(
		0,	//nodemask
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(rootSignature.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Root Signature");
		return 0;
	}
	// �쐬�������[�g�V�O�l�`�����p�C�v���C���ɐݒ�
	gpipeline.pRootSignature = rootSignature.Get();


	// �O���t�B�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̐���
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Graphics Pipeline State.");
		return 0;
	}


	// �r���[�|�[�g�ƃV�U�[��`
	viewport = CD3DX12_VIEWPORT { _backBuffers[0] };
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + window_width;
	scissorrect.bottom = scissorrect.top + window_height;


	// ���[���h�s��
	angleY = 0;// XM_PIDIV4;
	// �r���[�s��
	eye = XMFLOAT3(0, 10, -15);
	target = XMFLOAT3(0, 10, 0);
	up = XMFLOAT3(0, 1, 0);
	viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	// �v���W�F�N�V�����s��
	projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,	//��p��90�x
		static_cast<float>(window_width) / static_cast<float>(window_height),	// �A�X�y�N�g��
		1.0f,	// �j�A�N���b�v
		100.0f	// �t�@�[�N���b�v
	);

	return true;
}


void Application::Run() {
	Dx12Wrapper* dxWrapper = Dx12Wrapper::Instance();
	ID3D12Device* _dev = dxWrapper->GetDevice();
	IDXGISwapChain4* _swapchain = dxWrapper->GetSwapchain();
	ID3D12GraphicsCommandList* _cmdList = dxWrapper->GetCommandList();

	MSG msg = {};

	while (true) {
		{ // �s��v�Z
			angleY += 0.01f;
			worldMat = XMMatrixRotationY(angleY);
			mapMatrix->world = worldMat;
			mapMatrix->view = viewMat;
			mapMatrix->proj = projMat;
			mapMatrix->eye = eye;
		}

		// 2.�����_�[�^�[�Q�b�g���o�b�N�o�b�t�@�ɃZ�b�g
		// ���݂̃o�b�N�o�b�t�@���擾
		SIZE_T bbIdx = _swapchain->GetCurrentBackBufferIndex(); // �o�b�t�@�͂Q�Ȃ̂ŁA0��1�̂͂�

		// ���\�[�X�o���A�Ńo�b�t�@�̎g������ GPU �ɒʒm����
		D3D12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		_cmdList->ResourceBarrier(1, &BarrierDesc); //�o���A�w����s

		// �����_�[�^�[�Q�b�g�Ƃ��Ďw�肷��
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		// �[�x�o�b�t�@�r���[���֘A�t��
		auto dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvHandle);
		// �[�x�o�b�t�@�̃N���A
		_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// 3.�����_�[�^�[�Q�b�g���w��F�ŃN���A
		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; //���F
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		// �`�施��
		_cmdList->SetPipelineState(_pipelinestate.Get());
		_cmdList->SetGraphicsRootSignature(rootSignature.Get());
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);
		//_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		{// �`�掞�̐ݒ�
			// �s��ϊ�
			ID3D12DescriptorHeap* bdh[] = { basicDescHeap.Get() };
			_cmdList->SetDescriptorHeaps(1, bdh);
			_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

			// �}�e���A��
			ID3D12DescriptorHeap* mdh[] = { materialDescHeap.Get() };
			_cmdList->SetDescriptorHeaps(1, mdh);

			auto materialHandle = materialDescHeap->GetGPUDescriptorHandleForHeapStart();
			unsigned int idxOffset = 0;
			auto cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cbvsrvIncSize *= 5;	//CBV, SRV, SRV, SRV, SRV �̂T��

			for (auto& m : materials) {
				_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
				_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

				// �q�[�v�|�C���^�ƃC���f�b�N�X�����ɐi�߂�
				materialHandle.ptr += cbvsrvIncSize;
				idxOffset += m.indicesNum;
			}
		}

		// ���\�[�X�o���A�Ńo�b�t�@�̎g������ GPU �ɒʒm����
		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
		);
		_cmdList->ResourceBarrier(1, &BarrierDesc); //�o���A�w����s

		// 4.�����_�[�^�[�Q�b�g���N���[�Y
		_cmdList->Close();


		dxWrapper->ExecuteCommandList();
		dxWrapper->ResetCommandList();

		// 6.�X���b�v�`�F�[���̃t���b�v����
		// ��ԑJ��
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

		dxWrapper->SwapchainPresent();


		// ���b�Z�[�W����
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg); DispatchMessage(&msg);
		}

		//�A�v���P�[�V���� �� �I��� �Ƃ� �� message �� WM_QUIT �� �Ȃ�
		if (msg.message == WM_QUIT) {
			break;
		}
	}
}

void Application::Terminate() {
	//���� �N���X �� �g�� �Ȃ� �̂� �o�^ ���� ����
	UnregisterClass(window.lpszClassName, window.hInstance);

	DebugOutputFormatString(" Show window test.");
}