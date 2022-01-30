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



// @brief クラスの初期化
// @note このクラスを使用する前に一度必ず呼び出すこと
bool PMDRenderer::Init(ID3D12Device* device) {



	return true;
}