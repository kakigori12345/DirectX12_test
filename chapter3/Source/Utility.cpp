//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "Utility.h"

#include <Windows.h>
#include <assert.h>

//! @brief アライメントにそろえたサイズを返す
//! @param[in] size 元のサイズ
//! @param[in] alignment アライメントサイズ
//! @return アライメントにそろえたサイズ
size_t GetAlignmentedSize(size_t size, size_t alignment) {
	return size + alignment - size % alignment;
}


//! @brief モデルパスとテクスチャパスから合成パスを得る
//! @param[in] modelPath アプリから見た pmd モデルのパス
//! @param[in] texPath モデルから見たテクスチャのパス
//! @return アプリから見たテクスチャのパス
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath
) {
	// フォルダ区切り両方対応
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderPath = modelPath.substr(0, pathIndex);
	return folderPath + '/' + texPath;
}


//! @brief マルチバイトからワイド文字列への変換
//! @param[in] str マルチバイト文字列
//! @return 変換後のワイド文字列
std::wstring GetWideStringFromString(const std::string& str) {
	// まずは変換後の文字列数を取得する
	const int wstrNum1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0
	);

	std::wstring wstr;	//変換後の文字列
	wstr.resize(wstrNum1);

	// 文字列を変換
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

//! @brief ファイル名から拡張子を取得
//! @param[in] path 対象のパス文字列
//! @return 拡張子
std::string GetExtension(const std::string& path) {
	const int idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

//! @brief テクスチャのパスをセパレータ文字で分離する
//! param[in] path 対象のパス文字列
//! param[in] splitter 区切り文字
//! return 分離前後の文字列ペア
std::pair<std::string, std::string> SplitFileName(
	const std::string& path, const char splitter
) {
	std::pair<std::string, std::string> retval;

	const int idx = path.find(splitter);
	retval.first = path.substr(0, idx);
	retval.second = path.substr(idx + 1, path.length() - idx - 1);

	return retval;
}
