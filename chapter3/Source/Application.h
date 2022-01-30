#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------

// Direct3D
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <DirectXMath.h>

//ComPtr
#include <wrl.h>

#include "Util/SingletonDef.h"


//! �A�v���P�[�V�����N���X�i�V���O���g���j
class Application {
	SINGLETON_HEADER(Application)
	//----------------------------------------------------
	// ���\�b�h
	//----------------------------------------------------
public:
	//static Application* Instance();

	bool Init();

	void Run();

	void Terminate();

	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	WNDCLASSEX window;

	float												angleY;
	DirectX::XMFLOAT3									eye;
	DirectX::XMFLOAT3									target;
	DirectX::XMFLOAT3									up;
	DirectX::XMMATRIX									worldMat;
	DirectX::XMMATRIX									viewMat;
	DirectX::XMMATRIX									projMat;
};