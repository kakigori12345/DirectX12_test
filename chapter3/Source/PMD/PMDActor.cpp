//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "PMD/PMDActor.h"

#include <d3dx12.h>

#include "PMD/PMDRenderer.h"
#include "PMD/PMDDef.h"
#include "Util/Utility.h"

#include <map>

//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;
using namespace DirectX;

//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

//! @brief コンストラクタ
PMDActor::PMDActor(string modelPath)
	: m_modelPath(modelPath)
	, m_vertBuff(nullptr)
	, m_idxBuff(nullptr)
	, m_vbView()
	, m_ibView()
	, m_materials()
	, m_materialDescHeap(nullptr)
	, m_materialBuff(nullptr){
}

//! @brief デストラクタ
PMDActor::~PMDActor() {
}

//! @brief 初期化
bool PMDActor::Init(ID3D12Device* device) {
	// パスチェック
	if (m_modelPath.empty()) {
		DebugOutputFormatString("Model's Path is not setting.");
		return false;
	}

	HRESULT result = S_OK;

	// ヘッダ
	char signature[3] = {};		//シグネチャ
	PMDHeader pmdheader = {};	//PMD ヘッダ
	FILE* fp;
	errno_t error = fopen_s(&fp, m_modelPath.c_str(), "rb");
	if (error != 0) {
		// 詳細なエラー表示
		DebugOutputFormatString("Missed at Reading PMD File");
		return false;
	}
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	// 頂点
	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, fp);
	vector<unsigned char> vertices(vertNum * pmdVertexSize);
	fread(vertices.data(), vertices.size(), 1, fp);

	// 頂点バッファの作成
	D3D12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());

	result = device->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vertBuff.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Vertex Buffer.");
		return false;
	}
	// 頂点情報のコピー
	unsigned char* vertMap = nullptr;
	result = m_vertBuff->Map(0, nullptr, (void**)&vertMap);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Mapping Vertex.");
		return false;
	}
	copy(begin(vertices), end(vertices), vertMap);
	m_vertBuff->Unmap(0, nullptr); // verMap の情報を渡したので、マップを解除する
	// 頂点バッファビューの作成
	m_vbView.BufferLocation = m_vertBuff->GetGPUVirtualAddress(); // バッファの仮想アドレス
	m_vbView.SizeInBytes = vertices.size(); //全バイト数
	m_vbView.StrideInBytes = pmdVertexSize; //１頂点当たりのバイト数


	// インデックス情報作成
	vector<unsigned short> indices;
	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	// インデックスバッファの作成
	resdesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(indices.size()) * sizeof(indices[0]));
	result = device->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_idxBuff.ReleaseAndGetAddressOf()));
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Index Buffer.");
		return false;
	}

	// バッファにコピー
	unsigned short* mappedIdx = nullptr;
	result = m_idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Mapping Index.");
		return false;
	}
	copy(begin(indices), end(indices), mappedIdx);
	m_idxBuff->Unmap(0, nullptr);
	// インデックスバッファビューを作成
	m_ibView.BufferLocation = m_idxBuff->GetGPUVirtualAddress();
	m_ibView.Format = DXGI_FORMAT_R16_UINT;
	m_ibView.SizeInBytes = indices.size() * sizeof(indices[0]);


	// マテリアル情報を読み込む
	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, fp);
	vector<PMDMaterial> pmdMaterials(materialNum);
	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1,
		fp
	);

	fclose(fp);

	m_materials.resize(materialNum);
	vector<ID3D12Resource*> textureResources(materialNum, nullptr);
	vector<ID3D12Resource*> sphResources(materialNum, nullptr);
	vector<ID3D12Resource*> spaResources(materialNum, nullptr);
	vector<ID3D12Resource*> toonResources(materialNum, nullptr);

	{
		// コピー
		for (unsigned int i = 0; i < materialNum; ++i) {
			m_materials[i].indicesNum = pmdMaterials[i].indicesNum;
			m_materials[i].material.diffuse = pmdMaterials[i].diffuse;
			m_materials[i].material.alpha = pmdMaterials[i].alpha;
			m_materials[i].material.specular = pmdMaterials[i].specular;
			m_materials[i].material.specularity = pmdMaterials[i].specularity;
			m_materials[i].material.ambient = pmdMaterials[i].ambient;
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

			toonResources[i] = PMDRenderer::LoadTextureFromFile(toonFilePath, device);

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
				auto texFilePath = GetTexturePathFromModelAndTexPath(m_modelPath, texFileName.c_str());
				textureResources[i] = PMDRenderer::LoadTextureFromFile(texFilePath, device);
			}
			if (sphFileName != "") {
				auto sphFilePath = GetTexturePathFromModelAndTexPath(m_modelPath, sphFileName.c_str());
				sphResources[i] = PMDRenderer::LoadTextureFromFile(sphFilePath, device);
			}
			if (spaFileName != "") {
				auto spaFilePath = GetTexturePathFromModelAndTexPath(m_modelPath, spaFileName.c_str());
				spaResources[i] = PMDRenderer::LoadTextureFromFile(spaFilePath, device);
			}
		}
	}


	// マテリアルバッファを作成
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	D3D12_HEAP_PROPERTIES materialHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC materialDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(materialBuffSize) * materialNum);

	result = device->CreateCommittedResource(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_materialBuff.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating Material Buffer.");
		return false;
	}

	// マップマテリアルにコピー
	char* mapMaterial = nullptr;
	result = m_materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Mapping Material.");
		return false;
	}

	for (auto& m : m_materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;	//データコピー
		mapMaterial += materialBuffSize;	//次のアライメント位置まで進める
	}

	m_materialBuff->Unmap(0, nullptr);

	// マテリアル用ディスクリプタヒープとビューの作成
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materialNum * 5;	//マテリアル数を指定しておく
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = device->CreateDescriptorHeap(
		&matDescHeapDesc, IID_PPV_ARGS(m_materialDescHeap.ReleaseAndGetAddressOf())
	);
	if (result != S_OK) {
		DebugOutputFormatString("Missed at Creating DescriptorHeap For Material.");
		return false;
	}

	// 通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;

	// ビューの作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = m_materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBuffSize;
	// ディスクリプタヒープの先頭アドレスを記録
	auto matDescHeapHandle = m_materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto incSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	// デフォルトテクスチャ取得
	PMDRenderer* renderer = PMDRenderer::Instance();
	ID3D12Resource* whiteTex = renderer->GetDefaultTexture(PMDRenderer::TextureType::White);
	ID3D12Resource* blackTex = renderer->GetDefaultTexture(PMDRenderer::TextureType::Black);
	ID3D12Resource* gradTex	 = renderer->GetDefaultTexture(PMDRenderer::TextureType::Grad);

	for (unsigned int i = 0; i < materialNum; ++i) {
		// マテリアル用定数バッファビュー
		device->CreateConstantBufferView(&matCBVDesc, matDescHeapHandle);

		matCBVDesc.BufferLocation += materialBuffSize;
		matDescHeapHandle.ptr += incSize;


		// テクスチャ用ビュー
		if (textureResources[i] != nullptr) {
			// 読み込んだテクスチャ
			srvDesc.Format = textureResources[i]->GetDesc().Format;
			device->CreateShaderResourceView(
				textureResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// 白いテクスチャで埋め合わせ
			srvDesc.Format = whiteTex->GetDesc().Format;
			device->CreateShaderResourceView(
				whiteTex, &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;


		// sph 用ビュー
		if (sphResources[i] != nullptr) {
			srvDesc.Format = sphResources[i]->GetDesc().Format;
			device->CreateShaderResourceView(
				sphResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// 白いテクスチャで埋め合わせ
			srvDesc.Format = whiteTex->GetDesc().Format;
			device->CreateShaderResourceView(
				whiteTex, &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;


		// spa 用ビュー
		if (spaResources[i] != nullptr) {
			srvDesc.Format = spaResources[i]->GetDesc().Format;
			device->CreateShaderResourceView(
				spaResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// 黒いテクスチャで埋め合わせ
			srvDesc.Format = blackTex->GetDesc().Format;
			device->CreateShaderResourceView(
				blackTex, &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;


		// トゥーンリソース用ビュー
		if (toonResources[i] != nullptr) {
			srvDesc.Format = toonResources[i]->GetDesc().Format;
			device->CreateShaderResourceView(
				toonResources[i], &srvDesc, matDescHeapHandle
			);
		}
		else {
			// グレイグラデーションで埋め合わせ
			srvDesc.Format = gradTex->GetDesc().Format;
			device->CreateShaderResourceView(
				gradTex, &srvDesc, matDescHeapHandle
			);
		}
		matDescHeapHandle.ptr += incSize;
	}

	return true;
}


//! @brief 描画情報取得
void PMDActor::GetDrawInfo(DrawActorInfo& output) const {
	output.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	output.vbView = &m_vbView;
	output.ibView = &m_ibView;
	output.descHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	output.incCount = 5;
	output.materialDescHeap = m_materialDescHeap.Get();
	output.materials = &m_materials;
}
