#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "Util/SingletonDef.h"

#include <d3d12.h>

#include <wrl.h>
#include <string>

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

	enum class TextureType {
		White,
		Black,
		Grad,
	};
	//! @brief �f�t�H���g�e�N�X�`���擾
	ID3D12Resource* GetDefaultTexture(TextureType type);

public: 
	//! @brief �e�N�X�`�������[�h���ă��\�[�X���쐬����
	static ID3D12Resource* LoadTextureFromFile(const std::string& texPath, ID3D12Device* dev);

private:
	//! @brief �V�F�[�_�[�t�@�C���̓ǂݍ���
	bool _LoadShader();

	//! @brief �e�N�X�`���^�C�v���Ƃ̓ǂݍ��݊֐����Z�b�g����
	void _SetTextureLoader() const;

	//! @brief �p�C�v���C���֘A���쐬
	bool _CreatePipeline(ID3D12Device* device);

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

	ComPtr<ID3D12Resource> m_whiteTex;
	ComPtr<ID3D12Resource> m_blackTex;
	ComPtr<ID3D12Resource> m_gradTex;
};
