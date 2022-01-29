//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "PMDRenderer.h"

#include <d3dcompiler.h>
//#include <DirectXTex.h>

// その他
#include "Utility.h"
#include <assert.h>


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;

//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

//! @brief コンストラクタ
PMDRenderer::PMDRenderer() {

}

// シングルトン関連
std::unique_ptr<PMDRenderer> PMDRenderer::s_instance = nullptr;
PMDRenderer* PMDRenderer::Instance() {
	assert(s_instance);
	return s_instance.get();
}
void PMDRenderer::Create() {
	s_instance = unique_ptr<PMDRenderer>(new PMDRenderer());
}
void PMDRenderer::Destroy() {
	s_instance.reset();
}
