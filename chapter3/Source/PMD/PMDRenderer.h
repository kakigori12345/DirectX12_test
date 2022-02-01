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
public: //TODO: ���t�@�N�^���private�ɂ���
	Microsoft::WRL::ComPtr<ID3D12RootSignature>			rootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob>					rootSigBlob;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			_pipelinestate;

	Microsoft::WRL::ComPtr<ID3DBlob>					_vsBlob;
	Microsoft::WRL::ComPtr<ID3DBlob>					_psBlob;
	Microsoft::WRL::ComPtr<ID3DBlob>					errorBlob;
};
