#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "Util/SingletonDef.h"

#include <d3d12.h>

#include <wrl.h>


class PMDRenderer {
	SINGLETON_HEADER(PMDRenderer)

	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//----------------------------------------------------
	// ���\�b�h
	//----------------------------------------------------
public:
	// @brief �N���X�̏�����
	// @note ���̃N���X���g�p����O�Ɉ�x�K���Ăяo������
	bool Init(ID3D12Device* device);

	//! @brief �`��O����
	void BeginDraw(ID3D12GraphicsCommandList* cmdList);

private:
	//! @brief �V�F�[�_�[�t�@�C���̓ǂݍ���
	bool _LoadShader();

	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	bool m_isInitialized;

private:
	ComPtr<ID3D12RootSignature>		m_rootSignature;
	ComPtr<ID3DBlob>				m_rootSigBlob;
	ComPtr<ID3D12PipelineState>		m_pipelinestate;

	ComPtr<ID3DBlob>				m_vsBlob;
	ComPtr<ID3DBlob>				m_psBlob;
	ComPtr<ID3DBlob>				m_errorBlob;
};
