#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));	// 左上手前から右下奥へ
	float brightness = dot(-light, input.normal);

	float2 normalUV = (input.normal.xy + float2(1, -1)) * float2(0.5, -0.5);

	return float4(brightness, brightness, brightness, 1)	//輝度
		* diffuse						//ディヒューズ
		* tex.Sample(smp, input.uv)		//テクスチャカラー
		* sph.Sample(smp, normalUV);	//スフィアマップ
}