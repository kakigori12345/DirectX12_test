#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "Util/SingletonDef.h"

#include <d3d12.h>

#include <wrl.h>


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

private:
	//! @brief シェーダーファイルの読み込み
	bool _LoadShader();

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
};
