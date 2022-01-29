//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "PMD/PMDRenderer.h"

#include <d3dcompiler.h>
//#include <DirectXTex.h>

// その他
#include "Util/Utility.h"
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
