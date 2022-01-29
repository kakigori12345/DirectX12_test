//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "Application.h"

// Windows
#include <Windows.h>
#include <map>

// Direct3D
//#pragma comment( lib, "d3d12.lib")

// シェーダーのコンパイル
#include <d3dcompiler.h>

// DirectXTexライブラリ
#include <DirectXTex.h>

// その他
#include "Utility.h"

// リファクタ
#include "Dx12Wrapper.h"


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;


//-----------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------
namespace {
	// 定数
	int window_width = 1280;
	int window_height = 760;

	// PMD ヘッダ構造体
	struct PMDHeader {
		float version;
		char modelName[20];
		char comment[256];
	};

	// PMD 頂点構造体
	struct PMDVertex {
		XMFLOAT3 pos;				// 頂点情報(12)
		XMFLOAT3 normal;			// 法線ベクトル(12)
		XMFLOAT2 uv;				// uv 座標(8)
		unsigned short boneNo[2];	// ボーン番号(4)
		unsigned char boneWeigth;	// ボーン影響度(1)
		unsigned char edgeFlag;		// 輪郭線フラグ(1)
	};
	constexpr size_t pmdVertexSize = 38;	// 頂点一つ当たりのサイズ

#pragma pack(1)//ここから1バイトパッキング…アライメントは発生しない
	// PMD マテリアル構造体
	struct PMDMaterial {
		XMFLOAT3 diffuse;	// ディヒューズ色
		float alpha;		// ディヒューズα
		float specularity;	// スペキュラの強さ（乗算値）
		XMFLOAT3 specular;	// スペキュラ色
		XMFLOAT3 ambient;	// アンビエント色
		unsigned char toonIdx;	// トゥーン番号
		unsigned char edgeFlag;	// マテリアルごとの輪郭線フラグ

		// 2バイトのパディングが発生

		unsigned int indicesNum;	// このマテリアルが割り当てられるインデックス数
		char texFilePath[20];		// テクスチャファイルパス + α
	}; // 70バイトのはずだが、パディングにより72バイトになる
#pragma pack()//1バイトパッキング解除
	static_assert(sizeof(PMDMaterial) == 70, "assertion error.");
}

// 関数定義
namespace {
	// 面倒 だ けど 書か なけれ ば いけ ない 関数
	LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		// ウィンドウ が 破棄 さ れ たら 呼ば れる
		if (msg == WM_DESTROY) {
			PostQuitMessage(0);
			// OS に対して「 もうこ の アプリ は 終わる」 と 伝える
			return 0;
		}

		return DefWindowProc(hwnd, msg, wparam, lparam); // 既定 の 処理 を 行う
	}

	// デバッグレイヤーの有効化
	void EnableDebugLayer() {
		ID3D12Debug* debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}

	// テクスチャ呼び出し用の関数を用意
	using LoadLambda_t = function<HRESULT(const wstring& path, TexMetadata*, ScratchImage&)>;
	map<string, LoadLambda_t> loadLambdaTable;

	// テクスチャをロードしてリソースを作成する
	ID3D12Resource* LoadTextureFromFile(const string& texPath, ID3D12Device* dev) {

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


//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

std::unique_ptr<Application> Application::s_instance = nullptr;

Application* Application::Instance() {
	assert(s_instance);
	return s_instance.get();
}

void Application::Create() {
	s_instance = unique_ptr<Application>(new Application());
}

void Application::Destroy() {
	s_instance.reset();
}

bool Application::Init() {
	HRESULT result = S_OK;

	// ウィンドウ クラス の 生成＆ 登録
	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure; // コール バック 関数 の 指定
	window.lpszClassName = L"DX12Sample"; // アプリケーション クラス 名（ 適当 で よい）
	window.hInstance = GetModuleHandle(nullptr); // ハンドル の 取得
	RegisterClassEx(&window); // アプリケーション クラス（ ウィンドウ クラス の 指定 を OS に 伝える）
	RECT wrc = { 0, 0, window_width, window_height };// ウィンドウサイズ を 決める

	// 関数 を 使っ て ウィンドウ の サイズ を 補正 する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウ オブジェクト の 生成
	HWND hwnd = CreateWindow(
		window.lpszClassName,// クラス 名 指定
		L"DX12テスト", // タイトル バー の 文字
		WS_OVERLAPPEDWINDOW, // タイトル バー と 境界線 が ある ウィンドウ
		CW_USEDEFAULT, // 表示 x 座標 は OS に お 任せ
		CW_USEDEFAULT, // 表示 y 座標 は OS に お 任せ
		wrc.right - wrc.left, // ウィンドウ 幅
		wrc.bottom - wrc.top, // ウィンドウ 高
		nullptr, // 親 ウィンドウ ハンドル
		nullptr, // メニュー ハンドル
		window.hInstance, // 呼び出し アプリケーション ハンドル
		nullptr); // 追加 パラメーター

	// ウィンドウ 表示
	ShowWindow(hwnd, SW_SHOW);


	// 各クラスを初期化
	Dx12Wrapper* dxWrapper = Dx12Wrapper::Instance();
	if (!dxWrapper->Init(hwnd)) {
		DebugOutputFormatString("Dx12Wrapper の初期化に失敗.");
		return 0;
	}

	// 必要なデバイスを一時的に取得
	// TODO: ラッパーが完成したらこれらは必要なくなるはずなので消す
	ID3D12Device* _dev = dxWrapper->GetDevice();
	IDXGIFactory6* _dxgiFactory = dxWrapper->GetFactory();
	IDXGISwapChain4* _swapchain = dxWrapper->GetSwapchain();


	// ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビュー
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; //表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf())); //この段階ではまだ RTV ではない
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating DescriptorHeap.");
		return 0;
	}

	// sRGB 用のレンダーターゲットビュー設定を作成しておく
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//ガンマ補正あり
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	// スワップチェーンとビューの関連付け
	_backBuffers.resize(COMMAND_BUFFER_COUNT);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (UINT idx = 0; idx < COMMAND_BUFFER_COUNT; ++idx) {
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		if (result != S_OK) {
			DebugOutputFormatString("Missed at Getting BackBuffer.");
			return 0;
		}
		// 先ほど作成したディスクリプタヒープを RTV として設定する
		rtvDesc.Format = _backBuffers[idx]->GetDesc().Format;
		_dev->CreateRenderTargetView(
			_backBuffers[idx],
			&rtvDesc,
			handle);
		// ハンドルを一つずらす
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}





	//---------------------------------------------------------
	// ここから↓がモデルごとの処理かな
	//---------------------------------------------------------


	// PMD の読み込み
	string strModelPath = "data/Model/初音ミクmetal.pmd";

	// ヘッダ
	char signature[3] = {};		//シグネチャ
	PMDHeader pmdheader = {};	//PMD ヘッダ
	FILE* fp;
	errno_t error = fopen_s(&fp, strModelPath.c_str(), "rb");
	if (error != 0) {
		// 詳細なエラー表示
		DebugOutputFormatString("Missed at Reading PMD File");
		return 0;
	}
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	// 頂点
	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, fp);
	std::vector<unsigned char> vertices(vertNum * pmdVertexSize);
	fread(vertices.data(), vertices.size(), 1, fp);

	// 頂点バッファの作成
	D3D12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());

	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating CommittedResource.");
		return 0;
	}
	// 頂点情報のコピー
	unsigned char* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Mapping Vertex.");
		return 0;
	}
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr); // verMap の情報を渡したので、マップを解除する
	// 頂点バッファビューの作成
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // バッファの仮想アドレス
	vbView.SizeInBytes = vertices.size(); //全バイト数
	vbView.StrideInBytes = pmdVertexSize; //１頂点当たりのバイト数


	// インデックス情報作成
	std::vector<unsigned short> indices;
	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	// インデックスバッファの作成
	resdesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(indices.size()) * sizeof(indices[0]));
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf()));
	// バッファにコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);
	// インデックスバッファビューを作成
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);


	// マテリアル情報を読み込む
	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, fp);
	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1,
		fp
	);

	fclose(fp);

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

	materials.resize(materialNum);
	vector<ID3D12Resource*> textureResources(materialNum, nullptr);
	vector<ID3D12Resource*> sphResources(materialNum, nullptr);
	vector<ID3D12Resource*> spaResources(materialNum, nullptr);
	vector<ID3D12Resource*> toonResources(materialNum, nullptr);

	{
		// コピー
		for (unsigned int i = 0; i < materialNum; ++i) {
			materials[i].indicesNum = pmdMaterials[i].indicesNum;
			materials[i].material.diffuse = pmdMaterials[i].diffuse;
			materials[i].material.alpha = pmdMaterials[i].alpha;
			materials[i].material.specular = pmdMaterials[i].specular;
			materials[i].material.specularity = pmdMaterials[i].specularity;
			materials[i].material.ambient = pmdMaterials[i].ambient;
		}

		// テクスチャ
		for (unsigned int i = 0; i < materialNum; ++i) {
			if (strlen(pmdMaterials[i].texFilePath) == 0) {
				textureResources[i] = nullptr;
				continue;
			}

			// トゥーンリソースの読み込み
			string toonFilePath = "data/toon/";
			char toonFileName[16];

			sprintf_s(toonFileName, "toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
			toonFilePath += toonFileName;

			toonResources[i] = LoadTextureFromFile(toonFilePath, _dev);

			// 各種テクスチャを読み込む
			string texFileName = pmdMaterials[i].texFilePath;
			string sphFileName = "";
			string spaFileName = "";

			// スフィアファイルが混在しているかチェック
			if (std::count(texFileName.begin(), texFileName.end(), '*') > 0) {
				// スプリッタがあるので、ファイル名分割
				auto namepair = SplitFileName(texFileName);
				if (GetExtension(namepair.first) == "sph") {
					texFileName = namepair.second;
					sphFileName = namepair.first;
				}
				else if (GetExtension(namepair.first) == "spa")
				{
					texFileName = namepair.second;
					spaFileName = namepair.first;
				}
				else {
					texFileName = namepair.first;
					if (GetExtension(namepair.second) == "sph") {
						sphFileName = namepair.second;
					}
					else if (GetExtension(namepair.second) == "spa") {
						spaFileName = namepair.second;
					}
				}
			}
			else {
				// 単一のファイルなので、どの種類か特定する
				if (GetExtension(texFileName) == "sph") {
					sphFileName = texFileName;
					texFileName = "";
				}
				else if (GetExtension(texFileName) == "spa") {
					spaFileName = texFileName;
					texFileName = "";
				}
				else {
					texFileName = texFileName;
				}
			}

			// モデルとテクスチャパスから、プログラムから見たテクスチャパスを取得
			if (texFileName != "") {
				auto texFilePath = GetTexturePathFromModelAndTexPath(strModelPath, texFileName.c_str());
				textureResources[i] = LoadTextureFromFile(texFilePath, _dev);
			}
			if (sphFileName != "") {
				auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
				sphResources[i] = LoadTextureFromFile(sphFilePath, _dev);
			}
			if (spaFileName != "") {
				auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
				spaResources[i] = LoadTextureFromFile(spaFilePath, _dev);
			}
		}
	}

	// シェーダにマテリアル情報を転送する

	// マテリアルバッファを作成
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	D3D12_HEAP_PROPERTIES materialHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC materialDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(materialBuffSize) * materialNum);

	result = _dev->CreateCommittedResource(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(materialBuff.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Material Buffer.");
		return 0;
	}

	// マップマテリアルにコピー
	char* mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Mapping Material.");
		return 0;
	}

	for (auto& m : materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;	//データコピー
		mapMaterial += materialBuffSize;	//次のアライメント位置まで進める
	}

	materialBuff->Unmap(0, nullptr);

	// マテリアル用ディスクリプタヒープとビューの作成
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materialNum * 5;	//マテリアル数を指定しておく
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(
		&matDescHeapDesc, IID_PPV_ARGS(materialDescHeap.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating DescriptorHeap For Material.");
		return 0;
	}

	// 通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;

	// ビューの作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBuffSize;
	// ディスクリプタヒープの先頭アドレスを記録
	auto matDescHeapHandle = materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	whiteTex = CreateWhiteTexture(_dev);
	blackTex = CreateBlackTexture(_dev);
	gradTex = CreateGrayGradationTexture(_dev);

	for (unsigned int i = 0; i < materialNum; ++i) {
		// マテリアル用定数バッファビュー
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapHandle);

		matCBVDesc.BufferLocation += materialBuffSize;
		matDescHeapHandle.ptr += incSize;


		// テクスチャ用ビュー
		if (textureResources[i] != nullptr) {
			// 読み込んだテクスチャ
			srvDesc.Format = textureResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				textureResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// 白いテクスチャで埋め合わせ
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex.Get(), &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;


		// sph 用ビュー
		if (sphResources[i] != nullptr) {
			srvDesc.Format = sphResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				sphResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// 白いテクスチャで埋め合わせ
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex.Get(), &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;


		// spa 用ビュー
		if (spaResources[i] != nullptr) {
			srvDesc.Format = spaResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				spaResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// 黒いテクスチャで埋め合わせ
			srvDesc.Format = blackTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				blackTex.Get(), &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;


		// トゥーンリソース用ビュー
		if (toonResources[i] != nullptr) {
			srvDesc.Format = toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				toonResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// グレイグラデーションで埋め合わせ
			srvDesc.Format = gradTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				gradTex.Get(), &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;
	}




	//------------------------------------------------------
	// ここから↓がレンダラーの仕事な気がする
	//------------------------------------------------------

	// シェーダーリソースビュー
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//シェーダーから見えるように
	descHeapDesc.NodeMask = 0;		// アダプタは一つなので0をセット
	descHeapDesc.NumDescriptors = 1;// CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	//シェーダーリソースビュー用

	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Descriptor Heap For ShaderReosurceView.");
		return 0;
	}


	// 定数バッファーの作成
	D3D12_HEAP_PROPERTIES constBufferHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC constBufferDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	_dev->CreateCommittedResource(
		&constBufferHeap,
		D3D12_HEAP_FLAG_NONE,
		&constBufferDesc,	// 0xffアライメント
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Const Buffer.");
		return 0;
	}
	// マップで定数コピー
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);

	// 定数バッファービューを作成する
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;
	// ディスクリプタヒープ上でのメモリ位置（ハンドル）を取得
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart(); //この状態だとシェーダリソースビューの位置を示す
	// 実際に定数バッファービューを作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);


	// シェーダーの読み込みと生成
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
		DebugOutputFormatString("Missed at Compiling Vertex Shader.");
		return 0;
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
		return 0;
	}

	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{	// 座標
			"POSITION",		// セマンティクス名
			0,				// 同じセマンティクス名の時に使うインデックス
			DXGI_FORMAT_R32G32B32_FLOAT,	// フォーマット（要素数とビット数で型を表す）
			0,								// 入力スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,	// データのオフセット位置
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	// 
			0				// 一度に描画するインスタンスの数
		},
		{
			// 法線
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
			// ボーン番号
			"BONE_NO",
			0,
			DXGI_FORMAT_R16G16_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			// ウェイト
			"WEIGHT",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			// 輪郭線フラグ
			"EDGE_FLG",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};


	// 深度バッファの作成
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = window_width;
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;	//配列でも3Dテクスチャでもない
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResDesc.SampleDesc.Count = 1;	//サンプルは1ピクセルあたり一つ
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;	// 深さ1.0fでクリア
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	//32ビット float 値としてクリア

	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	//深度地書き込み用に使う
		&depthClearValue,
		IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating depth stensil buffer.");
		return 0;
	}

	// 深度バッファービューの作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Depth Heap.");
		return 0;
	}

	// 深度ビューの作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(
		depthBuffer.Get(),
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);



	// グラフィクスパイプラインを作成
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
	gpipeline.InputLayout.pInputElementDescs = inputLayout;		//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);	//レイアウト配列の要素数

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

	result = _dev->CreateRootSignature(
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
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Graphics Pipeline State.");
		return 0;
	}


	// ビューポートとシザー矩形
	viewport = CD3DX12_VIEWPORT { _backBuffers[0] };
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + window_width;
	scissorrect.bottom = scissorrect.top + window_height;


	// ワールド行列
	angleY = 0;// XM_PIDIV4;
	// ビュー行列
	eye = XMFLOAT3(0, 10, -15);
	target = XMFLOAT3(0, 10, 0);
	up = XMFLOAT3(0, 1, 0);
	viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	// プロジェクション行列
	projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,	//画角は90度
		static_cast<float>(window_width) / static_cast<float>(window_height),	// アスペクト比
		1.0f,	// ニアクリップ
		100.0f	// ファークリップ
	);

	return true;
}


void Application::Run() {
	Dx12Wrapper* dxWrapper = Dx12Wrapper::Instance();
	ID3D12Device* _dev = dxWrapper->GetDevice();
	IDXGISwapChain4* _swapchain = dxWrapper->GetSwapchain();
	ID3D12GraphicsCommandList* _cmdList = dxWrapper->GetCommandList();

	MSG msg = {};

	while (true) {
		{ // 行列計算
			angleY += 0.01f;
			worldMat = XMMatrixRotationY(angleY);
			mapMatrix->world = worldMat;
			mapMatrix->view = viewMat;
			mapMatrix->proj = projMat;
			mapMatrix->eye = eye;
		}

		// 2.レンダーターゲットをバックバッファにセット
		// 現在のバックバッファを取得
		SIZE_T bbIdx = _swapchain->GetCurrentBackBufferIndex(); // バッファは２つなので、0か1のはず

		// リソースバリアでバッファの使い道を GPU に通知する
		D3D12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		_cmdList->ResourceBarrier(1, &BarrierDesc); //バリア指定実行

		// レンダーターゲットとして指定する
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		// 深度バッファビューを関連付け
		auto dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvHandle);
		// 深度バッファのクリア
		_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// 3.レンダーターゲットを指定色でクリア
		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; //白色
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		// 描画命令
		_cmdList->SetPipelineState(_pipelinestate.Get());
		_cmdList->SetGraphicsRootSignature(rootSignature.Get());
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);
		//_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		{// 描画時の設定
			// 行列変換
			ID3D12DescriptorHeap* bdh[] = { basicDescHeap.Get() };
			_cmdList->SetDescriptorHeaps(1, bdh);
			_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

			// マテリアル
			ID3D12DescriptorHeap* mdh[] = { materialDescHeap.Get() };
			_cmdList->SetDescriptorHeaps(1, mdh);

			auto materialHandle = materialDescHeap->GetGPUDescriptorHandleForHeapStart();
			unsigned int idxOffset = 0;
			auto cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cbvsrvIncSize *= 5;	//CBV, SRV, SRV, SRV, SRV の５つ分

			for (auto& m : materials) {
				_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
				_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

				// ヒープポインタとインデックスを次に進める
				materialHandle.ptr += cbvsrvIncSize;
				idxOffset += m.indicesNum;
			}
		}

		// リソースバリアでバッファの使い道を GPU に通知する
		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
		);
		_cmdList->ResourceBarrier(1, &BarrierDesc); //バリア指定実行

		// 4.レンダーターゲットをクローズ
		_cmdList->Close();


		dxWrapper->ExecuteCommandList();
		dxWrapper->ResetCommandList();

		// 6.スワップチェーンのフリップ処理
		// 状態遷移
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

		dxWrapper->SwapchainPresent();


		// メッセージ処理
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg); DispatchMessage(&msg);
		}

		//アプリケーション が 終わる とき に message が WM_QUIT に なる
		if (msg.message == WM_QUIT) {
			break;
		}
	}
}

void Application::Terminate() {
	//もう クラス は 使わ ない ので 登録 解除 する
	UnregisterClass(window.lpszClassName, window.hInstance);

	DebugOutputFormatString(" Show window test.");
}