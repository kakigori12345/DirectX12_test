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
#include <d3dx12.h>
#include <DirectXMath.h>

//ComPtr
#include <wrl.h>


class Dx12Wrapper {
	//----------------------------------------------------
	// �R���X�g���N�^�֘A
	//----------------------------------------------------
private:
	// �V���O���g���Ȃ̂Ŕ���J
	Dx12Wrapper();
	Dx12Wrapper(const Dx12Wrapper&) = delete;
	Dx12Wrapper& operator=(const Dx12Wrapper&) = delete;
public:
	~Dx12Wrapper() {};
public:
	// �V���O���g���p�֐�
	static void Create();
	static void Destroy();
	static Dx12Wrapper* Instance();


	//----------------------------------------------------
	// ���\�b�h
	//----------------------------------------------------
public:
	// @brief �N���X�̏�����
	// @note ���̃N���X���g�p����O�Ɉ�x�K���Ăяo������
	bool Init();


	//------------------------------------------------------------------
	// Dx12�֘A�̃I�u�W�F�N�g�擾�n
	// TODO: ���t�@�N�^�ł�����K�v�Ȃ��֐��΂���ɂȂ�̂Ő�������
	//------------------------------------------------------------------
	// @brief �f�o�C�X���擾
	ID3D12Device* GetDevice();
	// @brief �t�@�N�g�����擾
	IDXGIFactory6* GetFactory();


	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
	Microsoft::WRL::ComPtr<IDXGIFactory6>				m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>				m_swapchain;

private:
	static std::unique_ptr<Dx12Wrapper> s_instance;
	bool m_isInitialized;
};