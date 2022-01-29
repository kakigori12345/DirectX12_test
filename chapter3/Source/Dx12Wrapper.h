#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
// �W�����C�u����
#include <vector>
#include<memory>

// Direct3D
#include <d3d12.h>
#include <dxgi1_6.h>

//ComPtr
#include <wrl.h>

#include "Util/SingletonDef.h"


class Dx12Wrapper {
	SINGLETON_HEADER(Dx12Wrapper)
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
	Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
	Microsoft::WRL::ComPtr<IDXGIFactory6>				m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>				m_swapchain;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		_cmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	_cmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			_cmdQueue;

private:
	UINT m_windowHeight;
	UINT m_windowWidth;

private:
	bool m_isInitialized;
};