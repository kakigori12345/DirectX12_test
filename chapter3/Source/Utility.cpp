//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "Utility.h"

#include <Windows.h>
#include <assert.h>

//! @brief �A���C�����g�ɂ��낦���T�C�Y��Ԃ�
//! @param[in] size ���̃T�C�Y
//! @param[in] alignment �A���C�����g�T�C�Y
//! @return �A���C�����g�ɂ��낦���T�C�Y
size_t GetAlignmentedSize(size_t size, size_t alignment) {
	return size + alignment - size % alignment;
}


//! @brief ���f���p�X�ƃe�N�X�`���p�X���獇���p�X�𓾂�
//! @param[in] modelPath �A�v�����猩�� pmd ���f���̃p�X
//! @param[in] texPath ���f�����猩���e�N�X�`���̃p�X
//! @return �A�v�����猩���e�N�X�`���̃p�X
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath
) {
	// �t�H���_��؂藼���Ή�
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderPath = modelPath.substr(0, pathIndex);
	return folderPath + '/' + texPath;
}


//! @brief �}���`�o�C�g���烏�C�h������ւ̕ϊ�
//! @param[in] str �}���`�o�C�g������
//! @return �ϊ���̃��C�h������
std::wstring GetWideStringFromString(const std::string& str) {
	// �܂��͕ϊ���̕����񐔂��擾����
	const int wstrNum1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0
	);

	std::wstring wstr;	//�ϊ���̕�����
	wstr.resize(wstrNum1);

	// �������ϊ�
	const int wstrNum2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		wstrNum1
	);

	assert(wstrNum1 == wstrNum2);
	return wstr;
}

//! @brief �t�@�C��������g���q���擾
//! @param[in] path �Ώۂ̃p�X������
//! @return �g���q
std::string GetExtension(const std::string& path) {
	const int idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

//! @brief �e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
//! param[in] path �Ώۂ̃p�X������
//! param[in] splitter ��؂蕶��
//! return �����O��̕�����y�A
std::pair<std::string, std::string> SplitFileName(
	const std::string& path, const char splitter
) {
	std::pair<std::string, std::string> retval;

	const int idx = path.find(splitter);
	retval.first = path.substr(0, idx);
	retval.second = path.substr(idx + 1, path.length() - idx - 1);

	return retval;
}
