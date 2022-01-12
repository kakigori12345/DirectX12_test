#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));	// 左上手前から右下奥へ
	float brightness = dot(-light, input.normal);

	// 光の反射ベクトル
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);


	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	float4 texColor = tex.Sample(smp, input.uv);
	return max(diffuse * diffuse * texColor + float4(specularB * specular.rgb, 1), float4(ambient * texColor, 1));

	return float4(brightness, brightness, brightness, 1)	//輝度
		* diffuse						//ディヒューズ
		* texColor						//テクスチャカラー
		* sph.Sample(smp, sphereMapUV)	//スフィアマップ（乗算）
		+ spa.Sample(smp, sphereMapUV)	//スフィアマップ（加算）
		+ float4(texColor * ambient, 1);//アンビエント
}