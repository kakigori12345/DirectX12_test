//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "Dx12Wrapper.h"

// Direct3D
#pragma comment( lib, "d3d12.lib")
#pragma comment( lib, "dxgi.lib")

// シェーダーのコンパイル
#include <d3dcompiler.h>
#pragma comment( lib, "d3dcompiler.lib")

// DirectXTexライブラリ
#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")

// その他
#include "Utility.h"

#ifdef _DEBUG 
#include < iostream >
#endif


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------
namespace {
	// デバッグレイヤーの有効化
	void EnableDebugLayer() {
		ID3D12Debug* debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}


// シングルトン関連
std::unique_ptr<Dx12Wrapper> Dx12Wrapper::s_instance = nullptr;
Dx12Wrapper* Dx12Wrapper::Instance() {
	assert(s_instance);
	return s_instance.get();
}
void Dx12Wrapper::Create() {
	s_instance = unique_ptr<Dx12Wrapper>(new Dx12Wrapper());
}
void Dx12Wrapper::Destroy() {
	s_instance.reset();
}



bool Dx12Wrapper::Init() {
#ifdef _DEBUG
	EnableDebugLayer();
#endif
	return true;
}