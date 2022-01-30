#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include <d3d12.h>



// ���_���C�A�E�g
const D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[] = {
	{	// ���W
		"POSITION",		// �Z�}���e�B�N�X��
		0,				// �����Z�}���e�B�N�X���̎��Ɏg���C���f�b�N�X
		DXGI_FORMAT_R32G32B32_FLOAT,	// �t�H�[�}�b�g�i�v�f���ƃr�b�g���Ō^��\���j
		0,								// ���̓X���b�g�C���f�b�N�X
		D3D12_APPEND_ALIGNED_ELEMENT,	// �f�[�^�̃I�t�Z�b�g�ʒu
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	// 
		0				// ��x�ɕ`�悷��C���X�^���X�̐�
	},
	{
		// �@��
		"NORMAL",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
	{	// uv
		"TEXCOORD",
		0,
		DXGI_FORMAT_R32G32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
	{
		// �{�[���ԍ�
		"BONE_NO",
		0,
		DXGI_FORMAT_R16G16_UINT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
	{
		// �E�F�C�g
		"WEIGHT",
		0,
		DXGI_FORMAT_R8_UINT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
	{
		// �֊s���t���O
		"EDGE_FLG",
		0,
		DXGI_FORMAT_R8_UINT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	},
};