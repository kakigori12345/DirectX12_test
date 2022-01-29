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

//! @brief デストラクタ
PMDRenderer::~PMDRenderer() {

}

// シングルトン
SINGLETON_CPP(PMDRenderer)
