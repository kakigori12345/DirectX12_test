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


//-----------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------
// �V�F�[�_�[���ɓn�����߂̊�{�I�ȍs��f�[�^
//struct SceneData {
//	// TODO: 16�o�C�g�A���C�����g���{��
//	DirectX::XMMATRIX world;
//	DirectX::XMMATRIX view;
//	DirectX::XMMATRIX proj;
//	DirectX::XMFLOAT3 eye;
//};


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

	/*Microsoft::WRL::ComPtr<ID3DBlob>					_vsBlob			= nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob>					_psBlob			= nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob>					errorBlob		= nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		basicDescHeap	= nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>				constBuff		= nullptr;

	SceneData*											mapMatrix		= nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource>				depthBuffer		= nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		dsvHeap			= nullptr;*/

	Microsoft::WRL::ComPtr<ID3D12RootSignature>			rootSignature	= nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob>					rootSigBlob		= nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState>			_pipelinestate	= nullptr;

	CD3DX12_VIEWPORT									viewport;
	D3D12_RECT											scissorrect;

	float												angleY;
	DirectX::XMFLOAT3									eye;
	DirectX::XMFLOAT3									target;
	DirectX::XMFLOAT3									up;
	DirectX::XMMATRIX									worldMat;
	DirectX::XMMATRIX									viewMat;
	DirectX::XMMATRIX									projMat;
};