//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "PMD/PMDRenderer.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>

// その他
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

	// 白テクスチャ作成
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
		std::fill(data.begin(), data.end(), 0xff);	//全部255で埋める

		// データ転送
		result = whiteBuff->WriteToSubresource(
			0,
			nullptr,
			data.data(),
			4 * 4,
			data.size()
		);

		return whiteBuff;
	}


	// 黒テクスチャ作成
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
		std::fill(data.begin(), data.end(), 0x00);	//全部0で埋める

		// データ転送
		result = blackBuff->WriteToSubresource(
			0,
			nullptr,
			data.data(),
			4 * 4,
			data.size()
		);

		return blackBuff;
	}


	// デフォルトグラデーションテクスチャ（トゥーン用）
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

		// データ転送
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


//! @brief コンストラクタ
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

	if (!_LoadShader()) {
		return false;
	}

	_SetTextureLoader();

	// デフォルトテクスチャ作成
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


//! @brief 描画前処理
void PMDRenderer::BeginDraw(ID3D12GraphicsCommandList* cmdList) {
	cmdList->SetPipelineState(m_pipelinestate.Get());
	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
}

//! @brief 指定されたテクスチャを取得
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

//! @brief テクスチャをロードしてリソースを作成する
ID3D12Resource* PMDRenderer::LoadTextureFromFile(const string& texPath, ID3D12Device* dev) {

	// 読み込んだテクスチャを保存しておくコンテナ
	static map<string, ID3D12Resource*> resourceTable;

	// すでに読み込み済みならそれを返す
	auto it = resourceTable.find(texPath);
	if (it != resourceTable.end()) {
		// テーブル内にあるのでマップ内のリソースを返す
		return it->second;
	}

	// WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	wstring wtexpath = GetWideStringFromString(texPath).c_str();
	string extension = GetExtension(texPath);

	// ファイルパスの指定、拡張子がない場合のエラーチェック
	assert(wtexpath.size() != 0 && extension.size() != 0);

	auto result = loadLambdaTable[extension](
		wtexpath,
		&metadata,
		scratchImg
		);

	if (FAILED(result)) {
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0);	//生データ抽出

	// WriteToSubresource で転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		metadata.height,
		metadata.arraySize,
		metadata.mipLevels);

	// バッファ作成
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
		nullptr,		//全領域へコピー
		img->pixels,	//元データアドレス
		img->rowPitch,	//1ラインサイズ
		img->slicePitch	//全サイズ
	);

	if (FAILED(result)) {
		return nullptr;
	}

	// テーブルに保存しておく
	resourceTable[texPath] = texBuff;
	return texBuff;
}


//! @brief シェーダーファイルの読み込み
bool PMDRenderer::_LoadShader() {
	HRESULT result = S_OK;

	result = D3DCompileFromFile(
		L"Resource/BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //デバッグ用 および 最適化なし
		0,
		&m_vsBlob,
		&m_errorBlob);
	if (result != S_OK) {
		// 詳細なエラー表示
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
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //デバッグ用 および 最適化なし
		0,
		&m_psBlob,
		&m_errorBlob);
	if (result != S_OK) {
		// 詳細なエラー表示
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

//! @brief テクスチャタイプごとの読み込み関数をセットする
void PMDRenderer::_SetTextureLoader() const {
	// テクスチャファイルの種類ごとに別々の読み込み関数を使用する
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

//! @brief パイプライン関連を作成
bool PMDRenderer::_CreatePipeline(ID3D12Device* device) {
	HRESULT result;

	// グラフィクスパイプラインを作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	// 頂点シェーダー、ピクセルシェーダーを設定
	gpipeline.pRootSignature = nullptr; //後々設定
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(m_vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(m_psBlob.Get());
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
	CD3DX12_DESCRIPTOR_RANGE descTblRange[4] = {};
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // 定数[b0]（ビュープロジェクション）
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // 定数[b1]（ワールド、ボーン）
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // マテリアル[b2]
	descTblRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // テクスチャ４つ

	// ルートパラメータの作成
	CD3DX12_ROOT_PARAMETER rootparam[3] = {};
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);	// ビュープロジェクション
	rootparam[1].InitAsDescriptorTable(1, &descTblRange[1]);	// ワールド、ボーン
	rootparam[2].InitAsDescriptorTable(2, &descTblRange[2]);	// マテリアル周り

	// サンプラーの作成
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	// ルートシグネチャの作成
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootparam;	//ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 3;		//ルートパラメータの数
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
	// 作成したルートシグネチャをパイプラインに設定
	gpipeline.pRootSignature = m_rootSignature.Get();


	// グラフィクスパイプラインステートオブジェクトの生成
	result = device->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(m_pipelinestate.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Graphics Pipeline State.");
		return false;
	}

	assert(result == S_OK);
	return true;
}
