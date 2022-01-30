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


class Dx12Wrapper {
	SINGLETON_HEADER(Dx12Wrapper)

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


	//------------------------------------------------------------------
	// Dx12�֘A�̃I�u�W�F�N�g�擾�n
	// TODO: ���t�@�N�^�ł�����K�v�Ȃ��֐��΂���ɂȂ�̂Ő�������
	//------------------------------------------------------------------
	// @brief �f�o�C�X���擾
	ID3D12Device* GetDevice();
	// @brief �t�@�N�g�����擾
	IDXGIFactory6* GetFactory();
	// @brief �X���b�v�`�F�[�����擾
	IDXGISwapChain4* GetSwapchain();
	// @brief �R�}���h���X�g���擾
	ID3D12GraphicsCommandList* GetCommandList();


	//! @brief �R�}���h���X�g���s
	//! @note ��������������܂œ����őҋ@����
	bool ExecuteCommandList();

	//! @brief �R�}���h���X�g�����Z�b�g
	bool ResetCommandList();

	//! @brief �X���b�v�`�F�[���̃t���b�v����
	bool SwapchainPresent();

	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	bool m_isInitialized;

private:
	Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
	Microsoft::WRL::ComPtr<IDXGIFactory6>				m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>				m_swapchain;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_cmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_cmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_cmdQueue;

public: //�ꎞ�I��public�ɂ��Ă����BTODO:���t�@�N�^���private�ɂ��Ă���
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		rtvHeaps;
	std::vector<ID3D12Resource*>						_backBuffers;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		basicDescHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource>				constBuff;

	SceneData*											mapMatrix;

	Microsoft::WRL::ComPtr<ID3D12Resource>				depthBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		dsvHeap;

	CD3DX12_VIEWPORT									viewport;
	D3D12_RECT											scissorrect;
};