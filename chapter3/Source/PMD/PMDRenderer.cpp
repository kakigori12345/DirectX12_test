//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "PMD/PMDRenderer.h"

#include <d3dx12.h>
#include <d3dcompiler.h>

// その他
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

//! @brief コンストラクタ
PMDRenderer::PMDRenderer() 
	: m_isInitialized(false)
	, rootSignature(nullptr)
	, rootSigBlob(nullptr)
	, _pipelinestate(nullptr)
	, _vsBlob(nullptr)
	, _psBlob(nullptr)
	, errorBlob(nullptr) {
}

//! @brief デストラクタ
PMDRenderer::~PMDRenderer() {

}

// シングルトン
SINGLETON_CPP(PMDRenderer)



// @brief クラスの初期化
// @note このクラスを使用する前に一度必ず呼び出すこと
bool PMDRenderer::Init(ID3D12Device* device) {
	assert(!m_isInitialized);
	HRESULT result = S_OK;

	{// シェーダーの読み込みと生成
		result = D3DCompileFromFile(
			L"Resource/BasicVertexShader.hlsl",
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"BasicVS",
			"vs_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //デバッグ用 および 最適化なし
			0,
			&_vsBlob,
			&errorBlob);
		if (result != S_OK) {
			// 詳細なエラー表示
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
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //デバッグ用 および 最適化なし
			0,
			&_psBlob,
			&errorBlob);
		if (result != S_OK) {
			// 詳細なエラー表示
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

	{// グラフィクスパイプラインを作成
		D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
		// 頂点シェーダー、ピクセルシェーダーを設定
		gpipeline.pRootSignature = nullptr; //後々設定
		gpipeline.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());
		gpipeline.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());
		// サンプルマスクとラスタライザーの設定
		gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; //デフォルトのサンプルマスク（0xffffffff）
		gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; //カリングしない

		D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
		renderTargetBlendDesc.BlendEnable = false;
		renderTargetBlendDesc.LogicOpEnable = false;
		renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

		// 入力レイアウト設定
		gpipeline.InputLayout.pInputElementDescs = INPUT_LAYOUT;	//レイアウト先頭アドレス
		gpipeline.InputLayout.NumElements = _countof(INPUT_LAYOUT);	//レイアウト配列の要素数

		// 深度値の設定
		gpipeline.DepthStencilState.DepthEnable = true;
		gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	//ピクセル描画時に、深度バッファに深度値を書き込む
		gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			//小さいほうを採用
		gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		// その他
		gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//カットなし
		gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//三角形
		gpipeline.NumRenderTargets = 1;	//今は一つ（マルチレンダーではない）
		gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//0～1に正規化されたRGBA
		gpipeline.SampleDesc.Count = 1;		//サンプリングは１ピクセルにつき１
		gpipeline.SampleDesc.Quality = 0;	//クオリティは最低


		// ディスクリプタテーブルレンジの作成
		//メモ：ここでの分け方は、同じ種類かつ同じレジスタならまとめているのだと思う。
		// 	    テクスチャは種類が全部同じで、かつ扱うレジスタも同じなので、複数のディスクリプタを使っている。	
		CD3DX12_DESCRIPTOR_RANGE descTblRange[3] = {};	//テクスチャと定数で２つ
		descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // 座標変換[b0]
		descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // マテリアル[b1]
		descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // テクスチャ４つ

		// ルートパラメータの作成
		CD3DX12_ROOT_PARAMETER rootparam[2] = {};
		rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);	// 座標変換
		rootparam[1].InitAsDescriptorTable(2, &descTblRange[1]);	// マテリアル周り

		// サンプラーの作成
		CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
		samplerDesc[0].Init(0);
		samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		// ルートシグネチャの作成
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.pParameters = rootparam;	//ルートパラメータの先頭アドレス
		rootSignatureDesc.NumParameters = 2;		//ルートパラメータの数
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
		// 作成したルートシグネチャをパイプラインに設定
		gpipeline.pRootSignature = rootSignature.Get();


		// グラフィクスパイプラインステートオブジェクトの生成
		result = device->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf()));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Creating Graphics Pipeline State.");
			return 0;
		}
	}


	m_isInitialized = true;
	return true;
}