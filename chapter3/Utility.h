#pragma once

//! @brief �A���C�����g�ɂ��낦���T�C�Y��Ԃ�
//! @brief size ���̃T�C�Y
//! @brief alignment �A���C�����g�T�C�Y
//! @return �A���C�����g�ɂ��낦���T�C�Y
size_t GetAlignmentedSize(size_t size, size_t alignment) {
	return size + alignment - size % alignment;
}