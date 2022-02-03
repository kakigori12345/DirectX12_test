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
	// メソッド
	//----------------------------------------------------
public:
	// @brief クラスの初期化
	// @note このクラスを使用する前に一度必ず呼び出すこと
	bool Init(ID3D12Device* device);

	//! @brief 描画前処理
	void BeginDraw(ID3D12GraphicsCommandList* cmdList);

	enum class TextureType {
		White,
		Black,
		Grad,
	};
	//! @brief デフォルトテクスチャ取得
	ID3D12Resource* GetDefaultTexture(TextureType type);

public: 
	//! @brief テクスチャをロードしてリソースを作成する
	static ID3D12Resource* LoadTextureFromFile(const std::string& texPath, ID3D12Device* dev);

private:
	//! @brief シェーダーファイルの読み込み
	bool _LoadShader();

	//! @brief テクスチャタイプごとの読み込み関数をセットする
	void _SetTextureLoader() const;

	//! @brief パイプライン関連を作成
	bool _CreatePipeline(ID3D12Device* device);

	//----------------------------------------------------
	// メンバ変数
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
