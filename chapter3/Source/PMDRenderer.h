#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include<memory>

class PMDRenderer {
	//----------------------------------------------------
	// コンストラクタ関連
	//----------------------------------------------------
private:
	// シングルトンなので非公開
	PMDRenderer();
	PMDRenderer(const PMDRenderer&) = delete;
	PMDRenderer& operator=(const PMDRenderer&) = delete;
public:
	~PMDRenderer() {};
public:
	// シングルトン用関数
	static void Create();
	static void Destroy();
	static PMDRenderer* Instance();


	//----------------------------------------------------
	// メソッド
	//----------------------------------------------------
public:


	//----------------------------------------------------
	// メンバ変数
	//----------------------------------------------------
private:
	static std::unique_ptr<PMDRenderer> s_instance;
};
