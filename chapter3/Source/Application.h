#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------

// Direct3D
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
	HWND GetWindowHandle();

	bool Init();

	void Run();

	void Terminate();


	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	WNDCLASSEX window;
	HWND m_hwnd;

	float												angleY;
	DirectX::XMFLOAT3									eye;
	DirectX::XMFLOAT3									target;
	DirectX::XMFLOAT3									up;
	DirectX::XMMATRIX									worldMat;
	DirectX::XMMATRIX									viewMat;
	DirectX::XMMATRIX									projMat;
};