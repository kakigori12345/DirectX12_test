#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));	// 左上手前から右下奥へ
	float brightness = dot(-light, input.normal);
	return float4(brightness, brightness, brightness, 1);
}