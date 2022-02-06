#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	// ���̃x�N�g���i���s�����j
	float3 light = normalize(float3(1, -1, 1));	// �����O����E������
	float diffuseB = saturate(dot(-light, input.normal));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	// ���̔��˃x�N�g��
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	// �X�t�B�A�}�b�v�p UV
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	// �e�N�X�`���F
	float4 texColor = tex.Sample(smp, input.uv);

	return max(
		saturate(
			  toonDif								// �P�x
			* diffuse								// �f�B�q���[�Y�F
			* texColor								// �e�N�X�`���J���[
			* sph.Sample(smp, sphereMapUV)			// �X�t�B�A�}�b�v�i��Z�j
		)
		+ saturate(
			  spa.Sample(smp, sphereMapUV)		 // �X�t�B�A�}�b�v�i���Z�j
			* texColor	
			+ float4(specularB * specular.rgb, 1)// �X�y�L����
		)
		, float4(ambient * texColor, 1));		 // �A���r�G���g
}