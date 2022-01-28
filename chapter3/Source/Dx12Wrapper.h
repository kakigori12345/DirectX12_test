#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
// 標準ライブラリ
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
	// コンストラクタ関連
	//----------------------------------------------------
private:
	// シングルトンなので非公開
	Dx12Wrapper() {};
	Dx12Wrapper(const Dx12Wrapper&) = delete;
	Dx12Wrapper& operator=(const Dx12Wrapper&) = delete;
public:
	~Dx12Wrapper() {};
public:
	// シングルトン用関数
	static void Create();
	static void Destroy();
	static Dx12Wrapper* Instance();

	//----------------------------------------------------
	// メソッド
	//----------------------------------------------------
public:
	bool Init();

	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:

private:
	static std::unique_ptr<Dx12Wrapper> s_instance;
};