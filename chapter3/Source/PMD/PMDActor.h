#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include <d3d12.h>
#include <DirectXMath.h>

#include <string>
#include <vector>

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


class PMDActor {
	//----------------------------------------------------
	// �R���X�g���N�^�֘A
	//----------------------------------------------------
public:
	//! @brief �R���X�g���N�^
	explicit PMDActor(std::string modelPath);
	//! @brief �f�X�g���N�^
	~PMDActor();

	//----------------------------------------------------
	// ���\�b�h
	//----------------------------------------------------
public:
	//! @brief ������
	bool Init(ID3D12Device* device);


	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	std::string m_modelPath;

private:
public: //TODO:��Upublic�ɂ��Ă��邾���B���t�@�N�^��ɖ߂�
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_vertBuff;
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_idxBuff;
	D3D12_VERTEX_BUFFER_VIEW							m_vbView;
	D3D12_INDEX_BUFFER_VIEW								m_ibView;

	std::vector<Material>								m_materials;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_materialDescHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_materialBuff;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_whiteTex;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_blackTex;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_gradTex;
};