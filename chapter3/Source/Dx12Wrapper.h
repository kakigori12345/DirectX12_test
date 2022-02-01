#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
// 標準ライブラリ
#include <vector>
#include<memory>

// Direct3D
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>


//ComPtr
#include <wrl.h>

#include "Util/SingletonDef.h"


// 型依存
struct DrawActorInfo;


class Dx12Wrapper {
	SINGLETON_HEADER(Dx12Wrapper)

	//-----------------------------------------------------------------
	// Type Definition
	//-----------------------------------------------------------------
	// シェーダー側に渡すための基本的な行列データ
	struct SceneData {
		// TODO: 16バイトアライメントを施す
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMFLOAT3 eye;
	};

	//----------------------------------------------------
	// メソッド
	//----------------------------------------------------
public:
	// @brief クラスの初期化
	// @note このクラスを使用する前に一度必ず呼び出すこと
	bool Init(HWND hwnd);

	// 描画命令
	//! @brief シーンデータをセット
	void SetSceneData();
	//! @brief 描画前処理
	void BeginDraw();
	//! @brief 描画
	//! @param[in] actor 描画対象
	void Draw(const DrawActorInfo& drawInfo);
	//! @brief 描画終了時処理
	void EndDraw();

	// @brief デバイスを取得
	ID3D12Device* GetDevice();
	// @brief コマンドリストを取得
	ID3D12GraphicsCommandList* GetCommandList();

private:
	//! @brief コマンドリスト実行
	//! @note 処理が完了するまで内部で待機する
	bool _ExecuteCommandList();

	//! @brief コマンドリストをリセット
	bool _ResetCommandList();

	//! @brief スワップチェーンのフリップ処理
	bool _SwapchainPresent();

	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:
	bool m_isInitialized;

private:
	Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
	Microsoft::WRL::ComPtr<IDXGIFactory6>				m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>				m_swapchain;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_cmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_cmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_cmdQueue;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_rtvHeaps;
	std::vector<ID3D12Resource*>						m_backBuffers;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_basicDescHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_constBuff;

	SceneData*											m_mapMatrix;

	Microsoft::WRL::ComPtr<ID3D12Resource>				m_depthBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_dsvHeap;

	CD3DX12_VIEWPORT									m_viewport;
	D3D12_RECT											m_scissorrect;

private:
	float												m_angleY;
	DirectX::XMFLOAT3									m_eye;
	DirectX::XMFLOAT3									m_target;
	DirectX::XMFLOAT3									m_up;
	DirectX::XMMATRIX									m_worldMat;
	DirectX::XMMATRIX									m_viewMat;
	DirectX::XMMATRIX									m_projMat;
};