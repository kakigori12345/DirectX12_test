//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Dx12Wrapper.h"

#include <d3dcompiler.h>

#include "PMD/PMDActor.h"

// ���̑�
#include <assert.h>


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
	, m_windowInfo()
	, m_device(nullptr)
	, m_dxgiFactory(nullptr)
	, m_swapchain(nullptr)
	, m_cmdAllocator(nullptr)
	, m_cmdList(nullptr)
	, m_cmdQueue(nullptr)
	, m_rtvHeaps(nullptr)
	, m_backBuffers()
	, m_basicDescHeap(nullptr)
	, m_constBuff(nullptr)
	, m_mapMatrix(nullptr)
	, m_depthBuffer(nullptr)
	, m_dsvHeap(nullptr)
	, m_viewport()
	, m_scissorrect()
	, m_eye()
	, m_target()
	, m_up()
	, m_viewMat()
	, m_projMat(){
}

// @brief �f�X�g���N�^
Dx12Wrapper::~Dx12Wrapper() {
}

// �V���O���g��
SINGLETON_CPP(Dx12Wrapper)



bool Dx12Wrapper::Init(HWND hwnd) {
	assert(!m_isInitialized);
	HRESULT result = S_OK;
	// �E�B���h�E�T�C�Y�擾
	m_windowInfo = GetWindowInfo(hwnd);

#ifdef _DEBUG
	EnableDebugLayer();
#endif

	if(!_CreateDevice()){
		return false;
	}

	if (!_CreateCommandContainer()) {
		return false;
	}

	if (!_CreateSwapchain(hwnd)) {
		return false;
	}

	if (!_CreateRenderTarget()) {
		return false;
	}

	if (!_CreateView()) {
		return false;
	}

	if (!_CreateDepthStencilView()) {
		return false;
	}

	_InitlalizeSceneData();


	m_isInitialized = true;
	return true;
}


//! @brief �V�[���f�[�^���Z�b�g
void Dx12Wrapper::SetSceneData() {
	// �Z�b�g
	m_mapMatrix->view = m_viewMat;
	m_mapMatrix->proj = m_projMat;
	m_mapMatrix->eye = m_eye;
}

//! @brief �`��O����
void Dx12Wrapper::BeginDraw() {
	// �����_�[�^�[�Q�b�g���o�b�N�o�b�t�@�ɃZ�b�g
		// ���݂̃o�b�N�o�b�t�@���擾
	const SIZE_T bbIdx = m_swapchain->GetCurrentBackBufferIndex(); // �o�b�t�@�͂Q�Ȃ̂ŁA0��1�̂͂�

	// ���\�[�X�o���A�Ńo�b�t�@�̎g������ GPU �ɒʒm����
	D3D12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		m_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_cmdList->ResourceBarrier(1, &BarrierDesc); //�o���A�w����s

	// �����_�[�^�[�Q�b�g�Ƃ��Ďw�肷��
	auto rtvH = m_rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// �[�x�o�b�t�@�r���[���֘A�t��
	auto dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvHandle);
	// �[�x�o�b�t�@�̃N���A
	m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// �����_�[�^�[�Q�b�g���w��F�ŃN���A
	float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; //���F
	m_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	m_cmdList->RSSetViewports(1, &m_viewport);
	m_cmdList->RSSetScissorRects(1, &m_scissorrect);
}

//! @brief �`��
	//! @param[in] actor �`��Ώ�
void Dx12Wrapper::Draw(const DrawActorInfo& drawInfo) {
	m_cmdList->IASetPrimitiveTopology(drawInfo.topology);
	m_cmdList->IASetVertexBuffers(0, 1, drawInfo.vbView);
	m_cmdList->IASetIndexBuffer(drawInfo.ibView);

	{// �`�掞�̐ݒ�
		// �V�[���f�[�^
		ID3D12DescriptorHeap* bdh[] = { m_basicDescHeap.Get() };
		m_cmdList->SetDescriptorHeaps(1, bdh);
		m_cmdList->SetGraphicsRootDescriptorTable(0, m_basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		// �ȉ����f���f�[�^
		// ���[���h
		ID3D12DescriptorHeap* transHeaps[] = { drawInfo.transformDescHeap };
		m_cmdList->SetDescriptorHeaps(1, transHeaps);
		m_cmdList->SetGraphicsRootDescriptorTable(1, drawInfo.transformDescHeap->GetGPUDescriptorHandleForHeapStart());

		// �}�e���A��
		ID3D12DescriptorHeap* mdh[] = { drawInfo.materialDescHeap };
		m_cmdList->SetDescriptorHeaps(1, mdh);

		auto materialHandle = drawInfo.materialDescHeap->GetGPUDescriptorHandleForHeapStart();
		unsigned int idxOffset = 0;
		auto cbvsrvIncSize = m_device->GetDescriptorHandleIncrementSize(drawInfo.descHeapType);
		cbvsrvIncSize *= drawInfo.incCount;	//CBV, SRV, SRV, SRV, SRV �̂T��

		for (auto& m : *(drawInfo.materials)) {
			m_cmdList->SetGraphicsRootDescriptorTable(2, materialHandle);
			m_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

			// �q�[�v�|�C���^�ƃC���f�b�N�X�����ɐi�߂�
			materialHandle.ptr += cbvsrvIncSize;
			idxOffset += m.indicesNum;
		}
	}
}

//! @brief �`��I��������
void Dx12Wrapper::EndDraw() {
	const SIZE_T bbIdx = m_swapchain->GetCurrentBackBufferIndex();

	// ���\�[�X�o���A�Ńo�b�t�@�̎g������ GPU �ɒʒm����
	D3D12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		m_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	);
	m_cmdList->ResourceBarrier(1, &BarrierDesc); //�o���A�w����s

	m_cmdList->Close();
	_ExecuteCommandList();
	_ResetCommandList();

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	_SwapchainPresent();
}


// @brief �f�o�C�X���擾
	// @return �f�o�C�X
ID3D12Device* Dx12Wrapper::GetDevice() {
	return m_device.Get();
}

// @brief �R�}���h���X�g���擾
ID3D12GraphicsCommandList* Dx12Wrapper::GetCommandList() {
	return m_cmdList.Get();
}


//---------------------------------------------------------
// �f�o�C�X�̐����n
//---------------------------------------------------------

bool Dx12Wrapper::_CreateDevice() {
	HRESULT result;

	{// �t�@�N�g���[
#ifdef _DEBUG
		result = CreateDXGIFactory2(
			DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()));
#else
		result = CreateDXGIFactory1(
			IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
#endif
		if (result != S_OK) {
			DebugOutputFormatString("Missed at creating factory.");
			return false;
		}
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
		result = D3D12CreateDevice(
			tmpAdapter,
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at creating device.");
			return false;
		}
	}

	return true;
}

bool Dx12Wrapper::_CreateCommandContainer() {
	HRESULT result = S_OK;

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
	return true;
}

bool Dx12Wrapper::_CreateSwapchain(HWND hwnd) {
	HRESULT result = S_OK;

	{// �X���b�v�`�F�[���̍쐬
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.Width = m_windowInfo.width;
		swapchainDesc.Height = m_windowInfo.height;
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

	return true;
}

bool Dx12Wrapper::_CreateRenderTarget() {
	HRESULT result = S_OK;

	// �f�B�X�N���v�^�q�[�v�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; //�\���̂Q��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_rtvHeaps.ReleaseAndGetAddressOf())); //���̒i�K�ł͂܂� RTV �ł͂Ȃ�
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating DescriptorHeap.");
		return false;
	}

	// sRGB �p�̃����_�[�^�[�Q�b�g�r���[�ݒ���쐬���Ă���
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//�K���}�␳����
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	// �X���b�v�`�F�[���ƃr���[�̊֘A�t��
	m_backBuffers.resize(COMMAND_BUFFER_COUNT);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (UINT idx = 0; idx < COMMAND_BUFFER_COUNT; ++idx) {
		result = m_swapchain->GetBuffer(idx, IID_PPV_ARGS(&m_backBuffers[idx]));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Getting BackBuffer.");
			return false;
		}
		// ��قǍ쐬�����f�B�X�N���v�^�q�[�v�� RTV �Ƃ��Đݒ肷��
		rtvDesc.Format = m_backBuffers[idx]->GetDesc().Format;
		m_device->CreateRenderTargetView(
			m_backBuffers[idx],
			&rtvDesc,
			handle);
		// �n���h��������炷
		handle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	m_viewport = CD3DX12_VIEWPORT{ m_backBuffers[0] };
	m_scissorrect.top = 0;
	m_scissorrect.left = 0;
	m_scissorrect.right = m_scissorrect.left + m_windowInfo.width;
	m_scissorrect.bottom = m_scissorrect.top + m_windowInfo.height;

	return true;
}

bool Dx12Wrapper::_CreateView() {
	HRESULT result = S_OK;

	{// �V�F�[�_�[���\�[�X�r���[
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//�V�F�[�_�[���猩����悤��
		descHeapDesc.NodeMask = 0;		// �A�_�v�^�͈�Ȃ̂�0���Z�b�g
		descHeapDesc.NumDescriptors = 1;// CBV
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	//�V�F�[�_�[���\�[�X�r���[�p

		result = m_device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(m_basicDescHeap.ReleaseAndGetAddressOf()));
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
			IID_PPV_ARGS(m_constBuff.ReleaseAndGetAddressOf())
		);
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Const Buffer.");
			return false;
		}
		// �}�b�v�Œ萔�R�s�[
		result = m_constBuff->Map(0, nullptr, (void**)&m_mapMatrix);

		// �萔�o�b�t�@�[�r���[���쐬����
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_constBuff->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_constBuff->GetDesc().Width;
		// �f�B�X�N���v�^�q�[�v��ł̃������ʒu�i�n���h���j���擾
		auto basicHeapHandle = m_basicDescHeap->GetCPUDescriptorHandleForHeapStart(); //���̏�Ԃ��ƃV�F�[�_���\�[�X�r���[�̈ʒu������
		// ���ۂɒ萔�o�b�t�@�[�r���[���쐬
		m_device->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
	}

	return true;
}

bool Dx12Wrapper::_CreateDepthStencilView() {
	HRESULT result = S_OK;

	// �[�x�o�b�t�@�̍쐬
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = m_windowInfo.width;
	depthResDesc.Height = m_windowInfo.height;
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
		IID_PPV_ARGS(m_depthBuffer.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating depth stensil buffer.");
		return false;
	}

	// �[�x�o�b�t�@�[�r���[�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	result = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_dsvHeap.ReleaseAndGetAddressOf()));
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
		m_depthBuffer.Get(),
		&dsvDesc,
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	return true;
}

bool Dx12Wrapper::_InitlalizeSceneData() {
	// �r���[�s��
	m_eye = XMFLOAT3(0, 10, -15);
	m_target = XMFLOAT3(0, 10, 0);
	m_up = XMFLOAT3(0, 1, 0);
	m_viewMat = XMMatrixLookAtLH(XMLoadFloat3(&m_eye), XMLoadFloat3(&m_target), XMLoadFloat3(&m_up));
	// �v���W�F�N�V�����s��
	m_projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,	//��p��90�x
		static_cast<float>(m_windowInfo.width) / static_cast<float>(m_windowInfo.height),	// �A�X�y�N�g��
		1.0f,	// �j�A�N���b�v
		100.0f	// �t�@�[�N���b�v
	);

	return true;
}



//! @brief �R�}���h���X�g���s
//! @note ��������������܂œ����őҋ@����
bool Dx12Wrapper::_ExecuteCommandList() {
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
bool Dx12Wrapper::_ResetCommandList() {
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
bool Dx12Wrapper::_SwapchainPresent() {
	HRESULT result = m_swapchain->Present(1, 0);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Present Swapchain.");
		return false;
	}

	return true;
}


