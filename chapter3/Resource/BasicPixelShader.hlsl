#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	// 光のベクトル（平行光線）
	float3 light = normalize(float3(1, -1, 1));	// 左上手前から右下奥へ
	float diffuseB = saturate(dot(-light, input.normal));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	// 光の反射ベクトル
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	// スフィアマップ用 UV
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	// テクスチャ色
	float4 texColor = tex.Sample(smp, input.uv);

	return max(
		saturate(
			  toonDif								// 輝度
			* diffuse								// ディヒューズ色
			* texColor								// テクスチャカラー
			* sph.Sample(smp, sphereMapUV)			// スフィアマップ（乗算）
		)
		+ saturate(
			  spa.Sample(smp, sphereMapUV)		 // スフィアマップ（加算）
			* texColor	
			+ float4(specularB * specular.rgb, 1)// スペキュラ
		)
		, float4(ambient * texColor, 1));		 // アンビエント
}