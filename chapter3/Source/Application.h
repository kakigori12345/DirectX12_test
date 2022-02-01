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
	WNDCLASSEX m_window;
	HWND m_hwnd;
};