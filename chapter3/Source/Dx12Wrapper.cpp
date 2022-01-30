//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Dx12Wrapper.h"

#include <d3dcompiler.h>

// ���̑�
#include "Util/Utility.h"
#include <assert.h>


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;
//using namespace DirectX;
using namespace Microsoft::WRL;


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------
namespace {
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
	: m_isInitialized(false)
	, m_device(nullptr)
	, m_dxgiFactory(nullptr)
	, m_swapchain(nullptr)
	, m_cmdAllocator(nullptr)
	, m_cmdList(nullptr)
	, m_cmdQueue(nullptr)
	, rtvHeaps(nullptr)
	, _backBuffers()
	, _vsBlob(nullptr)
	, _psBlob(nullptr)
	, errorBlob(nullptr)
	, basicDescHeap(nullptr)
	, constBuff(nullptr)
	, mapMatrix(nullptr)
	, depthBuffer(nullptr)
	, dsvHeap(nullptr)
{}

// @brief �f�X�g���N�^
Dx12Wrapper::~Dx12Wrapper() {

}

// �V���O���g��
SINGLETON_CPP(Dx12Wrapper)



bool Dx12Wrapper::Init(HWND hwnd) {
	assert(!m_isInitialized);
	HRESULT result = S_OK;
	// �E�B���h�E�T�C�Y�擾
	WindowInfo wInfo = GetWindowInfo(hwnd);

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
			D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmdAllocator.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandAllocator.");
			return false;
		}

		result = m_device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_cmdAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(m_cmdList.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandList.");
			return false;
		}
	}

	{// �L���[�̍쐬
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //�^�C���A�E�g�Ȃ�
		cmdQueueDesc.NodeMask = 0; //�A�_�v�^�[��Ȃ̂łO�ł����i�炵���j
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		// ����
		result = m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(m_cmdQueue.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating CommandQueue.");
			return false;
		}
	}

	{// �X���b�v�`�F�[���̍쐬
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.Width = wInfo.width;
		swapchainDesc.Height = wInfo.height;
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
			m_cmdQueue.Get(),
			hwnd,
			&swapchainDesc,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)m_swapchain.ReleaseAndGetAddressOf());
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating SwapChain.");
			return false;
		}
	}

	{// �f�B�X�N���v�^�q�[�v�̍쐬
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = 2; //�\���̂Q��
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		result = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf())); //���̒i�K�ł͂܂� RTV �ł͂Ȃ�
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating DescriptorHeap.");
			return false;
		}

		// sRGB �p�̃����_�[�^�[�Q�b�g�r���[�ݒ���쐬���Ă���
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//�K���}�␳����
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		// �X���b�v�`�F�[���ƃr���[�̊֘A�t��
		_backBuffers.resize(COMMAND_BUFFER_COUNT);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		for (UINT idx = 0; idx < COMMAND_BUFFER_COUNT; ++idx) {
			result = m_swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
			if (result != S_OK) {
				DebugOutputFormatString("Missed at Getting BackBuffer.");
				return false;
			}
			// ��قǍ쐬�����f�B�X�N���v�^�q�[�v�� RTV �Ƃ��Đݒ肷��
			rtvDesc.Format = _backBuffers[idx]->GetDesc().Format;
			m_device->CreateRenderTargetView(
				_backBuffers[idx],
				&rtvDesc,
				handle);
			// �n���h��������炷
			handle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}
	}


	{// �V�F�[�_�[���\�[�X�r���[
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//�V�F�[�_�[���猩����悤��
		descHeapDesc.NodeMask = 0;		// �A�_�v�^�͈�Ȃ̂�0���Z�b�g
		descHeapDesc.NumDescriptors = 1;// CBV
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	//�V�F�[�_�[���\�[�X�r���[�p

		result = m_device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Descriptor Heap For ShaderReosurceView.");
			return false;
		}
	}


	{// �萔�o�b�t�@�[�̍쐬
		D3D12_HEAP_PROPERTIES constBufferHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC constBufferDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
		m_device->CreateCommittedResource(
			&constBufferHeap,
			D3D12_HEAP_FLAG_NONE,
			&constBufferDesc,	// 0xff�A���C�����g
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf())
		);
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Const Buffer.");
			return false;
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
		m_device->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
	}


	{// �V�F�[�_�[�̓ǂݍ��݂Ɛ���
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
			return false;
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
			return false;
		}
	}

	{// �[�x�o�b�t�@�̍쐬
		D3D12_RESOURCE_DESC depthResDesc = {};
		depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthResDesc.Width = wInfo.width;
		depthResDesc.Height = wInfo.height;
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

		result = m_device->CreateCommittedResource(
			&depthHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&depthResDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,	//�[�x�n�������ݗp�Ɏg��
			&depthClearValue,
			IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf())
		);
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating depth stensil buffer.");
			return false;
		}

		// �[�x�o�b�t�@�[�r���[�̍쐬
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

		result = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Depth Heap.");
			return false;
		}

		// �[�x�r���[�̍쐬
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		m_device->CreateDepthStencilView(
			depthBuffer.Get(),
			&dsvDesc,
			dsvHeap->GetCPUDescriptorHandleForHeapStart()
		);
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
	return m_cmdList.Get();
}


//! @brief �R�}���h���X�g���s
//! @note ��������������܂œ����őҋ@����
bool Dx12Wrapper::ExecuteCommandList() {
	// �R�}���h���X�g���s
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdLists);
	// �t�F���X���쐬���Ă���
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;
	HRESULT result = m_device->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));
	// GPU�̏�������������܂ő҂�
	m_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
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
	HRESULT result = m_cmdAllocator->Reset();
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Reset Allocator.");
		return false;
	}
	result = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
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


