#include "BasicShaderHeader.hlsli"

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output; //ピクセルシェーダーへ渡す
	output.svpos = pos;
	output.uv = uv;
	return output;
}