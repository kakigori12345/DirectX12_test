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

//! @brief �`�掞�̏����W�߂��\����
struct DrawActorInfo {
	D3D_PRIMITIVE_TOPOLOGY topology; //CA:�����Ƀg�|���W�[���܂܂��Ă������̂��Y��
	const D3D12_VERTEX_BUFFER_VIEW* vbView;
	const D3D12_INDEX_BUFFER_VIEW* ibView;
	D3D12_DESCRIPTOR_HEAP_TYPE descHeapType;
	unsigned int incCount;
	ID3D12DescriptorHeap* transformDescHeap;
	ID3D12DescriptorHeap* materialDescHeap;
	const std::vector<Material>* materials;
};

struct Transform {
	//�����Ɏ����Ă�XMMATRIX�����o��16�o�C�g�A���C�����g�ł��邽��
	//Transform��new����ۂɂ�16�o�C�g���E�Ɋm�ۂ���
	void* operator new(size_t size);
	DirectX::XMMATRIX world;
};

#pragma pack(1)
struct PMDBone {
	char boneName[20];		// �{�[����
	unsigned short parentNo;// �e�{�[���ԍ�
	unsigned short nextNo;	// ��[�̃{�[���ԍ��B�������������߂̏��
	unsigned char type;		// �{�[����ʁB����́u��]�v�݂̂�����
	unsigned short ikBoneNo;// IK�{�[���ԍ�
	DirectX::XMFLOAT3 pos;			// �{�[���̊�_���W
};
#pragma pack()


class PMDActor {
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

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

	//! @brief �`����擾
	void GetDrawInfo(DrawActorInfo& output) const;

	//! @brief �`����X�V
	void Update();

private:
	//���W�ϊ��p�r���[�̐���
	bool _CreateTransformView(ID3D12Device* device);


	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	std::string m_modelPath;
	float		m_angleY;

private:
	Transform						m_transform;
	Transform*						m_mappedTransform;
	ComPtr<ID3D12Resource>			m_transformBuff;

	ComPtr<ID3D12Resource>			m_transformMat;//���W�ϊ��s��(���̓��[���h�̂�)
	ComPtr<ID3D12DescriptorHeap>	m_transformHeap;//���W�ϊ��q�[�v

private:
	ComPtr<ID3D12Resource>			m_vertBuff;
	ComPtr<ID3D12Resource>			m_idxBuff;
	D3D12_VERTEX_BUFFER_VIEW		m_vbView;
	D3D12_INDEX_BUFFER_VIEW			m_ibView;

	std::vector<Material>			m_materials;
	ComPtr<ID3D12DescriptorHeap>	m_materialDescHeap;
	ComPtr<ID3D12Resource>			m_materialBuff;
};