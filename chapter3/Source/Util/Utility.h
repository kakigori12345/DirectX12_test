#pragma once

#include <string>
#include <Windows.h>


//-----------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------
struct WindowInfo {
	UINT height;
	UINT width;
};



//! @brief �A���C�����g�ɂ��낦���T�C�Y��Ԃ�
//! @param[in] size ���̃T�C�Y
//! @param[in] alignment �A���C�����g�T�C�Y
//! @return �A���C�����g�ɂ��낦���T�C�Y
size_t GetAlignmentedSize(size_t size, size_t alignment);


//! @brief ���f���p�X�ƃe�N�X�`���p�X���獇���p�X�𓾂�
//! @param[in] modelPath �A�v�����猩�� pmd ���f���̃p�X
//! @param[in] texPath ���f�����猩���e�N�X�`���̃p�X
//! @return �A�v�����猩���e�N�X�`���̃p�X
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath
);


//! @brief �}���`�o�C�g���烏�C�h������ւ̕ϊ�
//! @param[in] str �}���`�o�C�g������
//! @return �ϊ���̃��C�h������
std::wstring GetWideStringFromString(const std::string& str);

//! @brief �t�@�C��������g���q���擾
//! @param[in] path �Ώۂ̃p�X������
//! @return �g���q
std::string GetExtension(const std::string& path);

//! @brief �e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
//! param[in] path �Ώۂ̃p�X������
//! param[in] splitter ��؂蕶��
//! return �����O��̕�����y�A
std::pair<std::string, std::string> SplitFileName(
	const std::string& path, const char splitter = '*'
);


// @brief �R���\�[�� ��� �� �t�H�[�}�b�g �t�� ������ �� �\�� 
	// @param format �t�H�[�}�b�g�i% d �Ƃ�% f �Ƃ� �́j 
	// @param �� �� ���� 
	// @remarks ���� �֐� �� �f�o�b�O �p �ł��B �f�o�b�O �� �ɂ��� ���� �� �܂� ��
void DebugOutputFormatString(const char* format, ...);


//! @brief ��ʃT�C�Y�擾
WindowInfo GetWindowInfo(HWND hwnd);

