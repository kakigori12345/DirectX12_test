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
	Dx12Wrapper() {};
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
	bool Init();

	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:

private:
	static std::unique_ptr<Dx12Wrapper> s_instance;
};