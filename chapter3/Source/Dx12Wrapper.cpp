//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.pch"
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
	, _cmdAllocator(nullptr)
	, _cmdList(nullptr)
	, _cmdQueue(nullptr)
	, m_windowHeight(0)
	, m_windowWidth(0)
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



bool Dx12Wrapper::Init(HWND hwnd) {
	assert(!m_isInitialized);
	HRESULT result = S_OK;

	{// �E�B���h�E�T�C�Y���擾
		RECT rect;
		GetWindowRect(hwnd, &rect);
		m_windowHeight = rect.bottom - rect.top;
		m_windowWidth = rect.right - rect.left;
	}

#ifdef _DEBUG
	EnableDebugLayer();
#endif

	{// �t�@�N�g���[
#ifdef _DEBUG
		result = CreateDXGIFactory2(
			DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()));
#else
		auto result = CreateDXGIFactory1(
			IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
#endif
	}

	{
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
	}


	{// �R�}���h
		result = m_device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandAllocator.");
			return 0;
		}

		result = m_device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			_cmdAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandList.");
			return 0;
		}
	}

	{// �L���[�̍쐬
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //�^�C���A�E�g�Ȃ�
		cmdQueueDesc.NodeMask = 0; //�A�_�v�^�[��Ȃ̂łO�ł����i�炵���j
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		// ����
		result = m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandQueue.");
			return 0;
		}
	}

	{// �X���b�v�`�F�[���̍쐬
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.Width = m_windowWidth;
		swapchainDesc.Height = m_windowHeight;
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

		result = m_dxgiFactory->CreateSwapChainForHwnd(
			_cmdQueue.Get(),
			hwnd,
			&swapchainDesc,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)m_swapchain.ReleaseAndGetAddressOf());
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating SwapChain.");
			return 0;
		}
	}


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

// @brief �X���b�v�`�F�[�����擾
IDXGISwapChain4* Dx12Wrapper::GetSwapchain() {
	return m_swapchain.Get();
}

// @brief �R�}���h���X�g���擾
ID3D12GraphicsCommandList* Dx12Wrapper::GetCommandList() {
	return _cmdList.Get();
}


//! @brief �R�}���h���X�g���s
//! @note ��������������܂œ����őҋ@����
bool Dx12Wrapper::ExecuteCommandList() {
	// �R�}���h���X�g���s
	ID3D12CommandList* cmdLists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdLists);
	// �t�F���X���쐬���Ă���
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;
	HRESULT result = m_device->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));
	// GPU�̏�������������܂ő҂�
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
	if (_fence->GetCompletedValue() != _fenceVal) {
		// �C�x���g�n���h�����擾
		auto event = CreateEvent(nullptr, false, false, nullptr);
		if (event == nullptr) {
			DebugOutputFormatString("Missed at Creating Event.");
			return false;
		}

		_fence->SetEventOnCompletion(_fenceVal, event);

		// �C�x���g����������܂őҋ@
		WaitForSingleObject(event, INFINITE);

		// �C�x���g�n���h�������
		CloseHandle(event);
	}
	// �ҋ@
	while (_fence->GetCompletedValue() != _fenceVal) { ; }

	return true;
}


//! @brief �R�}���h���X�g�����Z�b�g
bool Dx12Wrapper::ResetCommandList() {
	HRESULT result = _cmdAllocator->Reset();
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Reset Allocator.");
		return false;
	}
	result = _cmdList->Reset(_cmdAllocator.Get(), nullptr);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Reset CommandList.");
		return false;
	}

	return true;
}


//! @brief �X���b�v�`�F�[���̃t���b�v����
bool Dx12Wrapper::SwapchainPresent() {
	HRESULT result = m_swapchain->Present(1, 0);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Present Swapchain.");
		return false;
	}

	return true;
}


