#pragma once

//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include<memory>


// シングルトン用のマクロ

//! @brief シングルトン ヘッダの記述
#define SINGLETON_HEADER(className)						\
private:												\
	className();										\
	className(const className&) = delete;				\
	className& operator=(const className&) = delete;	\
public:													\
	~className();										\
public:													\
	static void Create();								\
	static void Destroy();								\
	static className* Instance();						\
private:												\
	static std::unique_ptr<className> s_instance;		\

//! @brief シングルトン cpp の記述
//! @attention コンストラクタ・デストラクタは独自に実装すること
#define SINGLETON_CPP(className)							\
std::unique_ptr<className> className::s_instance = nullptr; \
className* className::Instance() {							\
	assert(s_instance);										\
	return s_instance.get();								\
}															\
void className::Create() {									\
	s_instance = unique_ptr<className>(new className());	\
}															\
void className::Destroy() {									\
	s_instance.reset();										\
}															\

