#pragma once

//! @brief アライメントにそろえたサイズを返す
//! @brief size 元のサイズ
//! @brief alignment アライメントサイズ
//! @return アライメントにそろえたサイズ
size_t GetAlignmentedSize(size_t size, size_t alignment) {
	return size + alignment - size % alignment;
}