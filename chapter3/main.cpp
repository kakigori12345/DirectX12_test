#include <Windows.h>
#ifdef _DEBUG 
#include < iostream >

#endif using namespace std;

// @brief コンソール 画面 に フォーマット 付き 文字列 を 表示 
// @param format フォーマット（% d とか% f とか の） 
// @param 可変 長 引数 
// @remarks この 関数 は デバッグ 用 です。 デバッグ 時 にしか 動作 し ませ ん
void DebugOutputFormatString( const char* format, ...) { 
#ifdef _DEBUG
	va_list valist;
	va_start( valist, format);
	printf( format, valist);
	va_end(valist);
#endif
}

#ifdef _DEBUG
int main() {
#else 
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	DebugOutputFormatString(" Show window test.");
	getchar();
	return 0;
}
