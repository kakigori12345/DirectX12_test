//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "PMD/PMDRenderer.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>

// ���̑�
#include "PMD/VertexLayoutDef.h"
#include "Util/Utility.h"
#include <assert.h>
#include <map>


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;
using namespace DirectX;

//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

namespace {
	using LoadLambda_t = function<HRESULT(const wstring& path, TexMetadata*, ScratchImage&)>;
	std::map<string, LoadLambda_t> loadLambdaTable;

	// ���e�N�X�`���쐬
	ID3D12Resource* CreateWhiteTexture(ID3D12Device* dev) {
		D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
			D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
			D3D12_MEMORY_POOL_L0);

		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

		ID3D12Resource* whiteBuff = nullptr;
		auto result = dev->CreateCommittedResource(
			&texHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&whiteBuff)
		);

		if (FAILED(result)) {
			return nullptr;
		}

		std::vector<unsigned char> data(4 * 4 * 4);
		std::fill(data.begin(), data.end(), 0xff);	//�S��255�Ŗ��߂�

		// �f�[�^�]��
		result = whiteBuff->WriteToSubresource(
			0,
			nullptr,
			data.data(),
			4 * 4,
			data.size()
		);

		return whiteBuff;
	}


	// ���e�N�X�`���쐬
	ID3D12Resource* CreateBlackTexture(ID3D12Device* dev) {
		D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
			D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
			D3D12_MEMORY_POOL_L0);

		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

		ID3D12Resource* blackBuff = nullptr;
		auto result = dev->CreateCommittedResource(
			&texHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&blackBuff)
		);

		if (FAILED(result)) {
			return nullptr;
		}

		std::vector<unsigned char> data(4 * 4 * 4);
		std::fill(data.begin(), data.end(), 0x00);	//�S��0�Ŗ��߂�

		// �f�[�^�]��
		result = blackBuff->WriteToSubresource(
			0,
			nullptr,
			data.data(),
			4 * 4,
			data.size()
		);

		return blackBuff;
	}


	// �f�t�H���g�O���f�[�V�����e�N�X�`���i�g�D�[���p�j
	ID3D12Resource* CreateGrayGradationTexture(ID3D12Device* dev) {
		D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
			D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
			D3D12_MEMORY_POOL_L0);

		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

		ID3D12Resource* gradBuff = nullptr;
		auto result = dev->CreateCommittedResource(
			&texHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&gradBuff)
		);

		if (FAILED(result)) {
			return nullptr;
		}

		std::vector<unsigned char> data(4 * 256);
		auto it = data.begin();
		unsigned int c = 0xff;
		for (; it != data.end(); it += 4) {
			auto test1 = c << 24;
			auto test2 = c << 16;
			auto test3 = c << 8;
			auto col = (c << 24) | (c << 16) | (c << 8) | c;
			fill(it, it + 4, col);
			--c;
		}

		// �f�[�^�]��
		result = gradBuff->WriteToSubresource(
			0,
			nullptr,
			data.data(),
			4 * sizeof(unsigned int),
			sizeof(unsigned int) * data.size()
		);

		return gradBuff;
	}
}


//! @brief �R���X�g���N�^
PMDRenderer::PMDRenderer() 
	: m_isInitialized(false)
	, m_rootSignature(nullptr)
	, m_rootSigBlob(nullptr)
	, m_pipelinestate(nullptr)
	, m_vsBlob(nullptr)
	, m_psBlob(nullptr)
	, m_errorBlob(nullptr)
	, m_whiteTex(nullptr)
	, m_blackTex(nullptr)
	, m_gradTex(nullptr){
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

	if (!_LoadShader()) {
		return false;
	}

	_SetTextureLoader();

	// �f�t�H���g�e�N�X�`���쐬
	m_whiteTex = CreateWhiteTexture(device);
	m_blackTex = CreateBlackTexture(device);
	m_gradTex = CreateGrayGradationTexture(device);

	if (!_CreatePipeline(device)) {
		return false;
	}

	assert(SUCCEEDED(result));
	m_isInitialized = true;
	return true;
}


//! @brief �`��O����
void PMDRenderer::BeginDraw(ID3D12GraphicsCommandList* cmdList) {
	cmdList->SetPipelineState(m_pipelinestate.Get());
	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
}

//! @brief �w�肳�ꂽ�e�N�X�`�����擾
ID3D12Resource* PMDRenderer::GetDefaultTexture(TextureType type) {
	switch (type) {
	case TextureType::White:
		return m_whiteTex.Get();
	case TextureType::Black:
		return m_blackTex.Get();
	case TextureType::Grad:
		return m_gradTex.Get();
	default:
		assert(false);
	}

	return nullptr;
}

//! @brief �e�N�X�`�������[�h���ă��\�[�X���쐬����
ID3D12Resource* PMDRenderer::LoadTextureFromFile(const string& texPath, ID3D12Device* dev) {

	// �ǂݍ��񂾃e�N�X�`����ۑ����Ă����R���e�i
	static map<string, ID3D12Resource*> resourceTable;

	// ���łɓǂݍ��ݍς݂Ȃ炻���Ԃ�
	auto it = resourceTable.find(texPath);
	if (it != resourceTable.end()) {
		// �e�[�u�����ɂ���̂Ń}�b�v���̃��\�[�X��Ԃ�
		return it->second;
	}

	// WIC�e�N�X�`���̃��[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	wstring wtexpath = GetWideStringFromString(texPath).c_str();
	string extension = GetExtension(texPath);

	// �t�@�C���p�X�̎w��A�g���q���Ȃ��ꍇ�̃G���[�`�F�b�N
	assert(wtexpath.size() != 0 && extension.size() != 0);

	auto result = loadLambdaTable[extension](
		wtexpath,
		&metadata,
		scratchImg
		);

	if (FAILED(result)) {
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0);	//���f�[�^���o

	// WriteToSubresource �œ]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		metadata.height,
		metadata.arraySize,
		metadata.mipLevels);

	// �o�b�t�@�쐬
	ID3D12Resource* texBuff = nullptr;
	result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	if (FAILED(result)) {
		return nullptr;
	}

	result = texBuff->WriteToSubresource(
		0,
		nullptr,		//�S�̈�փR�s�[
		img->pixels,	//���f�[�^�A�h���X
		img->rowPitch,	//1���C���T�C�Y
		img->slicePitch	//�S�T�C�Y
	);

	if (FAILED(result)) {
		return nullptr;
	}

	// �e�[�u���ɕۑ����Ă���
	resourceTable[texPath] = texBuff;
	return texBuff;
}


//! @brief �V�F�[�_�[�t�@�C���̓ǂݍ���
bool PMDRenderer::_LoadShader() {
	HRESULT result = S_OK;

	result = D3DCompileFromFile(
		L"Resource/BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //�f�o�b�O�p ����� �œK���Ȃ�
		0,
		&m_vsBlob,
		&m_errorBlob);
	if (result != S_OK) {
		// �ڍׂȃG���[�\��
		std::string errstr;
		errstr.resize(m_errorBlob->GetBufferSize());
		std::copy_n(
			(char*)m_errorBlob->GetBufferPointer(),
			m_errorBlob->GetBufferSize(),
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
		&m_psBlob,
		&m_errorBlob);
	if (result != S_OK) {
		// �ڍׂȃG���[�\��
		std::string errstr;
		errstr.resize(m_errorBlob->GetBufferSize());
		std::copy_n(
			(char*)m_errorBlob->GetBufferPointer(),
			m_errorBlob->GetBufferSize(),
			errstr.begin());
		OutputDebugStringA(errstr.c_str());

		DebugOutputFormatString("Missed at Compiling Pixel Shader.");
		return false;
	}

	assert(result == S_OK);
	return true;
}

//! @brief �e�N�X�`���^�C�v���Ƃ̓ǂݍ��݊֐����Z�b�g����
void PMDRenderer::_SetTextureLoader() const {
	// �e�N�X�`���t�@�C���̎�ނ��ƂɕʁX�̓ǂݍ��݊֐����g�p����
	loadLambdaTable["sph"]
		= loadLambdaTable["spa"]
		= loadLambdaTable["bmp"]
		= loadLambdaTable["png"]
		= loadLambdaTable["jpg"]
		= [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};

	loadLambdaTable["tga"]
		= [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	loadLambdaTable["dds"]
		= [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};
}

//! @brief �p�C�v���C���֘A���쐬
bool PMDRenderer::_CreatePipeline(ID3D12Device* device) {
	HRESULT result;

	// �O���t�B�N�X�p�C�v���C�����쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	// ���_�V�F�[�_�[�A�s�N�Z���V�F�[�_�[��ݒ�
	gpipeline.pRootSignature = nullptr; //��X�ݒ�
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(m_vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(m_psBlob.Get());
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
	CD3DX12_DESCRIPTOR_RANGE descTblRange[4] = {};
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // �萔[b0]�i�r���[�v���W�F�N�V�����j
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // �萔[b1]�i���[���h�A�{�[���j
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // �}�e���A��[b2]
	descTblRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // �e�N�X�`���S��

	// ���[�g�p�����[�^�̍쐬
	CD3DX12_ROOT_PARAMETER rootparam[3] = {};
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);	// �r���[�v���W�F�N�V����
	rootparam[1].InitAsDescriptorTable(1, &descTblRange[1]);	// ���[���h�A�{�[��
	rootparam[2].InitAsDescriptorTable(2, &descTblRange[2]);	// �}�e���A������

	// �T���v���[�̍쐬
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	// ���[�g�V�O�l�`���̍쐬
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootparam;	//���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 3;		//���[�g�p�����[�^�̐�
	rootSignatureDesc.pStaticSamplers = samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 2;

	result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		&m_rootSigBlob,
		&m_errorBlob);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Serializing Root Signature.");
		return false;
	}

	result = device->CreateRootSignature(
		0,	//nodemask
		m_rootSigBlob->GetBufferPointer(),
		m_rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Root Signature");
		return false;
	}
	// �쐬�������[�g�V�O�l�`�����p�C�v���C���ɐݒ�
	gpipeline.pRootSignature = m_rootSignature.Get();


	// �O���t�B�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̐���
	result = device->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(m_pipelinestate.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Graphics Pipeline State.");
		return false;
	}

	assert(result == S_OK);
	return true;
}
