#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
// �W�����C�u����
#include <vector>
#include<memory>

// Direct3D
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>


//ComPtr
#include <wrl.h>

#include "Util/SingletonDef.h"
#include "Util/Utility.h"


// �^�ˑ�
struct DrawActorInfo;


class Dx12Wrapper {
	SINGLETON_HEADER(Dx12Wrapper)

	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//-----------------------------------------------------------------
	// Type Definition
	//-----------------------------------------------------------------
	// �V�F�[�_�[���ɓn�����߂̊�{�I�ȍs��f�[�^
	struct SceneData {
		// TODO: 16�o�C�g�A���C�����g���{��
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMFLOAT3 eye;
	};

	//----------------------------------------------------
	// ���\�b�h
	//----------------------------------------------------
public:
	// @brief �N���X�̏�����
	// @note ���̃N���X���g�p����O�Ɉ�x�K���Ăяo������
	bool Init(HWND hwnd);

	// �`�施��
	//! @brief �V�[���f�[�^���Z�b�g
	void SetSceneData();
	//! @brief �`��O����
	void BeginDraw();
	//! @brief �`��
	//! @param[in] actor �`��Ώ�
	void Draw(const DrawActorInfo& drawInfo);
	//! @brief �`��I��������
	void EndDraw();

	// @brief �f�o�C�X���擾
	ID3D12Device* GetDevice();
	// @brief �R�}���h���X�g���擾
	ID3D12GraphicsCommandList* GetCommandList();

private: // �K�v�ȃI�u�W�F�N�g�̐���
	bool _CreateDevice();
	bool _CreateCommandContainer();
	bool _CreateSwapchain(HWND hwnd);
	bool _CreateView();
	bool _CreateDepthStencilView();
	bool _CreateViewport();
	bool _InitlalizeSceneData();

private:
	//! @brief �R�}���h���X�g���s
	//! @note ��������������܂œ����őҋ@����
	bool _ExecuteCommandList();

	//! @brief �R�}���h���X�g�����Z�b�g
	bool _ResetCommandList();

	//! @brief �X���b�v�`�F�[���̃t���b�v����
	bool _SwapchainPresent();

	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	bool m_isInitialized;
	WindowInfo m_windowInfo;

private:
	ComPtr<ID3D12Device>				m_device;
	ComPtr<IDXGIFactory6>				m_dxgiFactory;
	ComPtr<IDXGISwapChain4>				m_swapchain;

	ComPtr<ID3D12CommandAllocator>		m_cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList>	m_cmdList;
	ComPtr<ID3D12CommandQueue>			m_cmdQueue;

	ComPtr<ID3D12DescriptorHeap>		m_rtvHeaps;
	std::vector<ID3D12Resource*>		m_backBuffers;

	ComPtr<ID3D12DescriptorHeap>		m_basicDescHeap;
	ComPtr<ID3D12Resource>				m_constBuff;

	SceneData*							m_mapMatrix;

	ComPtr<ID3D12Resource>				m_depthBuffer;
	ComPtr<ID3D12DescriptorHeap>		m_dsvHeap;

	CD3DX12_VIEWPORT					m_viewport;
	D3D12_RECT							m_scissorrect;

private:
	float								m_angleY;
	DirectX::XMFLOAT3					m_eye;
	DirectX::XMFLOAT3					m_target;
	DirectX::XMFLOAT3					m_up;
	DirectX::XMMATRIX					m_worldMat;
	DirectX::XMMATRIX					m_viewMat;
	DirectX::XMMATRIX					m_projMat;
};