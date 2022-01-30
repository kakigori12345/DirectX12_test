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


//! アプリケーションクラス（シングルトン）
class Application {
	SINGLETON_HEADER(Application)
	//----------------------------------------------------
	// メソッド
	//----------------------------------------------------
public:
	//static Application* Instance();

	bool Init();

	void Run();

	void Terminate();

	//----------------------------------------------------
	// メンバ変数
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