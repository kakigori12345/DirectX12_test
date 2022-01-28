//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "Dx12Wrapper.h"

// Direct3D
#pragma comment( lib, "d3d12.lib")
#pragma comment( lib, "dxgi.lib")

// �V�F�[�_�[�̃R���p�C��
#include <d3dcompiler.h>
#pragma comment( lib, "d3dcompiler.lib")

// DirectXTex���C�u����
#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")

// ���̑�
#include "Utility.h"
#include <assert.h>

#ifdef _DEBUG 
#include < iostream >
#endif


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------
namespace {
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

	// @brief �f�o�b�O���C���[�̗L����
	void EnableDebugLayer() {
		ID3D12Debug* debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

// @brief �R���X�g���N�^
Dx12Wrapper::Dx12Wrapper()
	: m_device(nullptr)
	, m_dxgiFactory(nullptr)
	, m_swapchain(nullptr)
	, m_isInitialized(false)
{}

// �V���O���g���֘A
std::unique_ptr<Dx12Wrapper> Dx12Wrapper::s_instance = nullptr;
Dx12Wrapper* Dx12Wrapper::Instance() {
	assert(s_instance);
	return s_instance.get();
}
void Dx12Wrapper::Create() {
	s_instance = unique_ptr<Dx12Wrapper>(new Dx12Wrapper());
}
void Dx12Wrapper::Destroy() {
	s_instance.reset();
}



bool Dx12Wrapper::Init() {
	assert(!m_isInitialized);

#ifdef _DEBUG
	EnableDebugLayer();
#endif

	// �t�@�N�g���[
#ifdef _DEBUG
	auto result = CreateDXGIFactory2(
		DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()));
#else
	auto result = CreateDXGIFactory1(
		IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
#endif

	// �A�_�v�^�[
	std::vector <IDXGIAdapter*> adapters; //�����ɃA�_�v�^�[��񋓂���
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; m_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
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
	D3D12CreateDevice(
		tmpAdapter,
		D3D_FEATURE_LEVEL_12_1,
		IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()));

	m_isInitialized = true;
	return true;
}


// @brief �f�o�C�X���擾
	// @return �f�o�C�X
ID3D12Device* Dx12Wrapper::GetDevice() {
	return m_device.Get();
}

// @brief �t�@�N�g�����擾
IDXGIFactory6* Dx12Wrapper::GetFactory(){
	return m_dxgiFactory.Get();
}