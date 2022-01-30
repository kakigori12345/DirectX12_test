//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "PMD/PMDRenderer.h"

#include <d3dx12.h>
#include <d3dcompiler.h>

// ���̑�
#include "PMD/VertexLayoutDef.h"
#include "Util/Utility.h"
#include <assert.h>


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;

//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

//! @brief �R���X�g���N�^
PMDRenderer::PMDRenderer() 
	: m_isInitialized(false)
	, rootSignature(nullptr)
	, rootSigBlob(nullptr)
	, _pipelinestate(nullptr)
	, _vsBlob(nullptr)
	, _psBlob(nullptr)
	, errorBlob(nullptr) {
}

//! @brief �f�X�g���N�^
PMDRenderer::~PMDRenderer() {

}

// �V���O���g��
SINGLETON_CPP(PMDRenderer)



// @brief �N���X�̏�����
// @note ���̃N���X���g�p����O�Ɉ�x�K���Ăяo������
bool PMDRenderer::Init(ID3D12Device* device) {
	assert(!m_isInitialized);
	HRESULT result = S_OK;

	{// �V�F�[�_�[�̓ǂݍ��݂Ɛ���
		result = D3DCompileFromFile(
			L"Resource/BasicVertexShader.hlsl",
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicVS",
			"vs_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //�f�o�b�O�p ����� �œK���Ȃ�
			0,
			&_vsBlob,
			&errorBlob);
		if (result != S_OK) {
			// �ڍׂȃG���[�\��
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			OutputDebugStringA(errstr.c_str());

			DebugOutputFormatString("Missed at Compiling Vertex Shader.");
			return false;
		}

		result = D3DCompileFromFile(
			L"Resource/BasicPixelShader.hlsl",
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicPS",
			"ps_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //�f�o�b�O�p ����� �œK���Ȃ�
			0,
			&_psBlob,
			&errorBlob);
		if (result != S_OK) {
			// �ڍׂȃG���[�\��
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			OutputDebugStringA(errstr.c_str());

			DebugOutputFormatString("Missed at Compiling Pixel Shader.");
			return false;
		}
	}

	{// �O���t�B�N�X�p�C�v���C�����쐬
		D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
		// ���_�V�F�[�_�[�A�s�N�Z���V�F�[�_�[��ݒ�
		gpipeline.pRootSignature = nullptr; //��X�ݒ�
		gpipeline.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());
		gpipeline.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());
		// �T���v���}�X�N�ƃ��X�^���C�U�[�̐ݒ�
		gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; //�f�t�H���g�̃T���v���}�X�N�i0xffffffff�j
		gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; //�J�����O���Ȃ�

		D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
		renderTargetBlendDesc.BlendEnable = false;
		renderTargetBlendDesc.LogicOpEnable = false;
		renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

		// ���̓��C�A�E�g�ݒ�
		gpipeline.InputLayout.pInputElementDescs = INPUT_LAYOUT;	//���C�A�E�g�擪�A�h���X
		gpipeline.InputLayout.NumElements = _countof(INPUT_LAYOUT);	//���C�A�E�g�z��̗v�f��

		// �[�x�l�̐ݒ�
		gpipeline.DepthStencilState.DepthEnable = true;
		gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	//�s�N�Z���`�掞�ɁA�[�x�o�b�t�@�ɐ[�x�l����������
		gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			//�������ق����̗p
		gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		// ���̑�
		gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//�J�b�g�Ȃ�
		gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//�O�p�`
		gpipeline.NumRenderTargets = 1;	//���͈�i�}���`�����_�[�ł͂Ȃ��j
		gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//0�`1�ɐ��K�����ꂽRGBA
		gpipeline.SampleDesc.Count = 1;		//�T���v�����O�͂P�s�N�Z���ɂ��P
		gpipeline.SampleDesc.Quality = 0;	//�N�I���e�B�͍Œ�


		// �f�B�X�N���v�^�e�[�u�������W�̍쐬
		//�����F�����ł̕������́A������ނ��������W�X�^�Ȃ�܂Ƃ߂Ă���̂��Ǝv���B
		// 	    �e�N�X�`���͎�ނ��S�������ŁA���������W�X�^�������Ȃ̂ŁA�����̃f�B�X�N���v�^���g���Ă���B	
		CD3DX12_DESCRIPTOR_RANGE descTblRange[3] = {};	//�e�N�X�`���ƒ萔�łQ��
		descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // ���W�ϊ�[b0]
		descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // �}�e���A��[b1]
		descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // �e�N�X�`���S��

		// ���[�g�p�����[�^�̍쐬
		CD3DX12_ROOT_PARAMETER rootparam[2] = {};
		rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);	// ���W�ϊ�
		rootparam[1].InitAsDescriptorTable(2, &descTblRange[1]);	// �}�e���A������

		// �T���v���[�̍쐬
		CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
		samplerDesc[0].Init(0);
		samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		// ���[�g�V�O�l�`���̍쐬
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.pParameters = rootparam;	//���[�g�p�����[�^�̐擪�A�h���X
		rootSignatureDesc.NumParameters = 2;		//���[�g�p�����[�^�̐�
		rootSignatureDesc.pStaticSamplers = samplerDesc;
		rootSignatureDesc.NumStaticSamplers = 2;

		result = D3D12SerializeRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			&rootSigBlob,
			&errorBlob);
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Serializing Root Signature.");
			return 0;
		}

		result = device->CreateRootSignature(
			0,	//nodemask
			rootSigBlob->GetBufferPointer(),
			rootSigBlob->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Root Signature");
			return 0;
		}
		// �쐬�������[�g�V�O�l�`�����p�C�v���C���ɐݒ�
		gpipeline.pRootSignature = rootSignature.Get();


		// �O���t�B�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̐���
		result = device->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Graphics Pipeline State.");
			return 0;
		}
	}


	m_isInitialized = true;
	return true;
}