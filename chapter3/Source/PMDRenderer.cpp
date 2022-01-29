//-----------------------------------------------------------------
// File Include
//-----------------------------------------------------------------
#include "PreCompileHeader.h"
#include "PMDRenderer.h"

#include <d3dcompiler.h>
//#include <DirectXTex.h>

// ���̑�
#include "Utility.h"
#include <assert.h>


//-----------------------------------------------------------------
// Namespace Depend
//-----------------------------------------------------------------
using namespace std;

//-----------------------------------------------------------------
// Method Definition
//-----------------------------------------------------------------

//! @brief �R���X�g���N�^
PMDRenderer::PMDRenderer() {

}

// �V���O���g���֘A
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
