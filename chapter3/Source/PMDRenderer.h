#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include<memory>

class PMDRenderer {
	//----------------------------------------------------
	// �R���X�g���N�^�֘A
	//----------------------------------------------------
private:
	// �V���O���g���Ȃ̂Ŕ���J
	PMDRenderer();
	PMDRenderer(const PMDRenderer&) = delete;
	PMDRenderer& operator=(const PMDRenderer&) = delete;
public:
	~PMDRenderer() {};
public:
	// �V���O���g���p�֐�
	static void Create();
	static void Destroy();
	static PMDRenderer* Instance();


	//----------------------------------------------------
	// ���\�b�h
	//----------------------------------------------------
public:


	//----------------------------------------------------
	// �����o�ϐ�
	//----------------------------------------------------
private:
	static std::unique_ptr<PMDRenderer> s_instance;
};
