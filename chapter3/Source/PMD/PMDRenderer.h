#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "Util/SingletonDef.h"

#include <d3d12.h>

#include <wrl.h>


class PMDRenderer {
	SINGLETON_HEADER(PMDRenderer)
	//----------------------------------------------------
	// ���\�b�h
	//----------------------------------------------------
public:
	// @brief �N���X�̏�����
	// @note ���̃N���X���g�p����O�Ɉ�x�K���Ăяo������
	bool Init(ID3D12Device* device);

	//! @brief �`��O����
	void BeginDraw(ID3D12GraphicsCommandList* cmdList);


	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	bool m_isInitialized;

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature>			m_rootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob>					m_rootSigBlob;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			m_pipelinestate;

	Microsoft::WRL::ComPtr<ID3DBlob>					m_vsBlob;
	Microsoft::WRL::ComPtr<ID3DBlob>					m_psBlob;
	Microsoft::WRL::ComPtr<ID3DBlob>					m_errorBlob;
};
