//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Application.h"

// Windows
#include <Windows.h>

// ���̑�
#include "Util/Utility.h"

// ���t�@�N�^
#include "Dx12Wrapper.h"
#include "PMD/PMDRenderer.h"
#include "PMD/PMDActor.h" //TODO:Renderer���ł����炢��Ȃ�


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
}


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

//! @brief �R���X�g���N�^
Application::Application()
	: window{}
	, m_hwnd(nullptr){

}

//! @brief �f�X�g���N�^
Application::~Application() {

}

// �V���O���g��
SINGLETON_CPP(Application)

HWND Application::GetWindowHandle() {
	return m_hwnd;
}

bool Application::Init() {
	HRESULT result = S_OK;

	// �E�B���h�E �N���X �� ������ �o�^
	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[�� �o�b�N �֐� �� �w��
	window.lpszClassName = L"DX12Sample"; // �A�v���P�[�V���� �N���X ��
	window.hInstance = GetModuleHandle(nullptr); // �n���h�� �� �擾
	RegisterClassEx(&window); // �A�v���P�[�V���� �N���X�i �E�B���h�E �N���X �� �w�� �� OS �� �`����j
	RECT wrc = { 0, 0, window_width, window_height };// �E�B���h�E�T�C�Y �� ���߂�

	// �֐� �� �g�� �� �E�B���h�E �� �T�C�Y �� �␳ ����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E �I�u�W�F�N�g �� ����
	m_hwnd = CreateWindow(
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
	WindowInfo wInfo = GetWindowInfo(m_hwnd);

	// �E�B���h�E �\��
	ShowWindow(m_hwnd, SW_SHOW);

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
	PMDRenderer* renderer = PMDRenderer::Instance();

	ID3D12Device* _dev = dxWrapper->GetDevice();
	IDXGISwapChain4* _swapchain = dxWrapper->GetSwapchain();
	ID3D12GraphicsCommandList* _cmdList = dxWrapper->GetCommandList();

	// ���Ń��f�����쐬
	PMDActor actor("data/Model/�����~�Nmetal.pmd");
	if (!actor.Init(_dev)) {
		DebugOutputFormatString("Failed Creating Model.");
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
		_cmdList->SetPipelineState(renderer->_pipelinestate.Get());
		_cmdList->SetGraphicsRootSignature(renderer->rootSignature.Get());
		_cmdList->RSSetViewports(1, &(dxWrapper->viewport));
		_cmdList->RSSetScissorRects(1, &(dxWrapper->scissorrect));
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
			TranslateMessage(&msg);
			DispatchMessage(&msg);
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