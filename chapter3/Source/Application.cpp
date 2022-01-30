//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Application.h"
#include "PMD/PMDActor.h" //TODO:Renderer���ł����炢��Ȃ�

// Windows
#include <Windows.h>
#include <map>

// �V�F�[�_�[�̃R���p�C��
#include <d3dcompiler.h>

// DirectXTex���C�u����
#include <DirectXTex.h>

// ���̑�
#include "Util/Utility.h"

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
}


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

//! @brief �R���X�g���N�^
Application::Application() {

}

//! @brief �f�X�g���N�^
Application::~Application() {

}

// �V���O���g��
SINGLETON_CPP(Application)


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

	// �쐬�����E�B���h�E�̏����擾
	WindowInfo wInfo = GetWindowInfo(hwnd);

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

	//------------------------------------------------------
	// �������火�������_���[�̎d���ȋC������
	//------------------------------------------------------

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

	
	// �O���t�B�N�X�p�C�v���C�����쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	// ���_�V�F�[�_�[�A�s�N�Z���V�F�[�_�[��ݒ�
	gpipeline.pRootSignature = nullptr; //��X�ݒ�
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(dxWrapper->_vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(dxWrapper->_psBlob.Get());
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
		&(dxWrapper->errorBlob));
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
	viewport = CD3DX12_VIEWPORT { dxWrapper->_backBuffers[0] };
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + wInfo.width;
	scissorrect.bottom = scissorrect.top + wInfo.height;


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
		static_cast<float>(wInfo.width) / static_cast<float>(wInfo.height),	// �A�X�y�N�g��
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

	// ���Ń��f�����쐬
	PMDActor actor("data/Model/�����~�Nmetal.pmd");
	if (!actor.Init(_dev)) {
		DebugOutputFormatString("Failed Creating Model");
		return;
	}

	MSG msg = {};

	while (true) {
		{ // �s��v�Z
			angleY += 0.01f;
			worldMat = XMMatrixRotationY(angleY);
			dxWrapper->mapMatrix->world = worldMat;
			dxWrapper->mapMatrix->view = viewMat;
			dxWrapper->mapMatrix->proj = projMat;
			dxWrapper->mapMatrix->eye = eye;
		}

		// 2.�����_�[�^�[�Q�b�g���o�b�N�o�b�t�@�ɃZ�b�g
		// ���݂̃o�b�N�o�b�t�@���擾
		SIZE_T bbIdx = _swapchain->GetCurrentBackBufferIndex(); // �o�b�t�@�͂Q�Ȃ̂ŁA0��1�̂͂�

		// ���\�[�X�o���A�Ńo�b�t�@�̎g������ GPU �ɒʒm����
		D3D12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			dxWrapper->_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		_cmdList->ResourceBarrier(1, &BarrierDesc); //�o���A�w����s

		// �����_�[�^�[�Q�b�g�Ƃ��Ďw�肷��
		auto rtvH = dxWrapper->rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		// �[�x�o�b�t�@�r���[���֘A�t��
		auto dsvHandle = dxWrapper->dsvHeap->GetCPUDescriptorHandleForHeapStart();
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
		_cmdList->IASetVertexBuffers(0, 1, &(actor.m_vbView));
		_cmdList->IASetIndexBuffer(&(actor.m_ibView));
		//_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		{// �`�掞�̐ݒ�
			// �s��ϊ�
			ID3D12DescriptorHeap* bdh[] = { dxWrapper->basicDescHeap.Get() };
			_cmdList->SetDescriptorHeaps(1, bdh);
			_cmdList->SetGraphicsRootDescriptorTable(0, dxWrapper->basicDescHeap->GetGPUDescriptorHandleForHeapStart());

			// �}�e���A��
			ID3D12DescriptorHeap* mdh[] = { actor.m_materialDescHeap.Get() };
			_cmdList->SetDescriptorHeaps(1, mdh);

			auto materialHandle = actor.m_materialDescHeap->GetGPUDescriptorHandleForHeapStart();
			unsigned int idxOffset = 0;
			auto cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cbvsrvIncSize *= 5;	//CBV, SRV, SRV, SRV, SRV �̂T��

			for (auto& m : actor.m_materials) {
				_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
				_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

				// �q�[�v�|�C���^�ƃC���f�b�N�X�����ɐi�߂�
				materialHandle.ptr += cbvsrvIncSize;
				idxOffset += m.indicesNum;
			}
		}

		// ���\�[�X�o���A�Ńo�b�t�@�̎g������ GPU �ɒʒm����
		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			dxWrapper->_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
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