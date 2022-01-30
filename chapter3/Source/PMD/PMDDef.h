#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include <DirectXMath.h>


//-----------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------

// PMD �w�b�_�\����
struct PMDHeader {
	float version;
	char modelName[20];
	char comment[256];
};

// PMD ���_�\����
struct PMDVertex {
	DirectX::XMFLOAT3 pos;		// ���_���(12)
	DirectX::XMFLOAT3 normal;	// �@���x�N�g��(12)
	DirectX::XMFLOAT2 uv;		// uv ���W(8)
	unsigned short boneNo[2];	// �{�[���ԍ�(4)
	unsigned char boneWeigth;	// �{�[���e���x(1)
	unsigned char edgeFlag;		// �֊s���t���O(1)
};
constexpr size_t pmdVertexSize = 38;	// ���_�������̃T�C�Y

#pragma pack(1)//��������1�o�C�g�p�b�L���O�c�A���C�����g�͔������Ȃ�
	// PMD �}�e���A���\����
struct PMDMaterial {
	DirectX::XMFLOAT3 diffuse;	// �f�B�q���[�Y�F
	float alpha;		// �f�B�q���[�Y��
	float specularity;	// �X�y�L�����̋����i��Z�l�j
	DirectX::XMFLOAT3 specular;	// �X�y�L�����F
	DirectX::XMFLOAT3 ambient;	// �A���r�G���g�F
	unsigned char toonIdx;	// �g�D�[���ԍ�
	unsigned char edgeFlag;	// �}�e���A�����Ƃ̗֊s���t���O

	// 2�o�C�g�̃p�f�B���O������

	unsigned int indicesNum;	// ���̃}�e���A�������蓖�Ă���C���f�b�N�X��
	char texFilePath[20];		// �e�N�X�`���t�@�C���p�X + ��
}; // 70�o�C�g�̂͂������A�p�f�B���O�ɂ��72�o�C�g�ɂȂ�
#pragma pack()//1�o�C�g�p�b�L���O����
static_assert(sizeof(PMDMaterial) == 70, "assertion error.");
