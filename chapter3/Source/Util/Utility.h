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



//! @brief アライメントにそろえたサイズを返す
//! @param[in] size 元のサイズ
//! @param[in] alignment アライメントサイズ
//! @return アライメントにそろえたサイズ
size_t GetAlignmentedSize(size_t size, size_t alignment);


//! @brief モデルパスとテクスチャパスから合成パスを得る
//! @param[in] modelPath アプリから見た pmd モデルのパス
//! @param[in] texPath モデルから見たテクスチャのパス
//! @return アプリから見たテクスチャのパス
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath
);


//! @brief マルチバイトからワイド文字列への変換
//! @param[in] str マルチバイト文字列
//! @return 変換後のワイド文字列
std::wstring GetWideStringFromString(const std::string& str);

//! @brief ファイル名から拡張子を取得
//! @param[in] path 対象のパス文字列
//! @return 拡張子
std::string GetExtension(const std::string& path);

//! @brief テクスチャのパスをセパレータ文字で分離する
//! param[in] path 対象のパス文字列
//! param[in] splitter 区切り文字
//! return 分離前後の文字列ペア
std::pair<std::string, std::string> SplitFileName(
	const std::string& path, const char splitter = '*'
);


// @brief コンソール 画面 に フォーマット 付き 文字列 を 表示 
	// @param format フォーマット（% d とか% f とか の） 
	// @param 可変 長 引数 
	// @remarks この 関数 は デバッグ 用 です。 デバッグ 時 にしか 動作 し ませ ん
void DebugOutputFormatString(const char* format, ...);


//! @brief 画面サイズ取得
WindowInfo GetWindowInfo(HWND hwnd);

