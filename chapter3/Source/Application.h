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


//-----------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------
// �V�F�[�_�[���ɓ�������}�e���A���f�[�^
struct MaterialForHlsl {
	DirectX::XMFLOAT3 diffuse;	// �f�B�q���[�Y�F
	float alpha;				// �f�B�q���[�Y��
	DirectX::XMFLOAT3 specular;	// �X�y�L�����F
	float specularity;			// �X�y�L�����̋����i��Z�l�j
	DirectX::XMFLOAT3 ambient;	// �A���r�G���g�F
};

// ����ȊO�̃}�e���A���f�[�^
struct AdditionalMaterial {
	std::string texPath;	// �e�N�X�`���t�@�C���p�X
	int toonIdx;			// �g�D�[���ԍ�
	bool edgeFlag;			// �}�e���A�����Ƃ̗֊s���t���O
};

// �S�̂��܂Ƃ߂�
struct Material {
	unsigned int indicesNum;	// �C���f�b�N�X��
	MaterialForHlsl material;
	AdditionalMaterial additional;
};

// �V�F�[�_�[���ɓn�����߂̊�{�I�ȍs��f�[�^
struct SceneData {
	// TODO: 16�o�C�g�A���C�����g���{��
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
	DirectX::XMFLOAT3 eye;
};


//! �A�v���P�[�V�����N���X�i�V���O���g���j
class Application {
	//----------------------------------------------------
	// �R���X�g���N�^�֘A
	//----------------------------------------------------
private:
	// �V���O���g���Ȃ̂Ŕ���J
	Application() {};
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
public:
	~Application() {};
public:
	// �V���O���g���p�������֐�
	static void Create();
	static void Destroy();

	//----------------------------------------------------
	// ���\�b�h
	//----------------------------------------------------
public:
	static Application* Instance();

	bool Init();

	void Run();

	void Terminate();

	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	WNDCLASSEX window;
	/*Microsoft::WRL::ComPtr<IDXGISwapChain4>				_swapchain		= nullptr;*/

	/*Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		_cmdAllocator	= nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	_cmdList		= nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			_cmdQueue		= nullptr;*/

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		rtvHeaps		= nullptr;
	std::vector<ID3D12Resource*>						_backBuffers;

	Microsoft::WRL::ComPtr<ID3D12Resource>				vertBuff		= nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>				idxBuff			= nullptr;
	D3D12_VERTEX_BUFFER_VIEW							vbView;
	D3D12_INDEX_BUFFER_VIEW								ibView;

	Microsoft::WRL::ComPtr<ID3DBlob>					_vsBlob			= nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob>					_psBlob			= nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob>					errorBlob		= nullptr;

	std::vector<Material>								materials;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		materialDescHeap= nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>				 materialBuff	= nullptr;
	
	Microsoft::WRL::ComPtr<ID3D12Resource> whiteTex;
	Microsoft::WRL::ComPtr<ID3D12Resource> blackTex;
	Microsoft::WRL::ComPtr<ID3D12Resource> gradTex;


	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		basicDescHeap	= nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>				constBuff		= nullptr;

	SceneData*											mapMatrix		= nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource>				depthBuffer		= nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		dsvHeap			= nullptr;

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

private:
	static std::unique_ptr<Application> s_instance;
};