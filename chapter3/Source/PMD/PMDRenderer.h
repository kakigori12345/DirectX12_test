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
	// メソッド
	//----------------------------------------------------
public:
	// @brief クラスの初期化
	// @note このクラスを使用する前に一度必ず呼び出すこと
	bool Init(ID3D12Device* device);

	//! @brief 描画前処理
	void BeginDraw(ID3D12GraphicsCommandList* cmdList);


	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:
	bool m_isInitialized;

private:
public: //TODO: リファクタ後にprivateにする
	Microsoft::WRL::ComPtr<ID3D12RootSignature>			rootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob>					rootSigBlob;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			_pipelinestate;

	Microsoft::WRL::ComPtr<ID3DBlob>					_vsBlob;
	Microsoft::WRL::ComPtr<ID3DBlob>					_psBlob;
	Microsoft::WRL::ComPtr<ID3DBlob>					errorBlob;
};
