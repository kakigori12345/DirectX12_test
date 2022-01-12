#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	// ���̃x�N�g���i���s�����j
	float3 light = normalize(float3(1, -1, 1));	// �����O����E������
	float diffuseB = dot(-light, input.normal);

	// ���̔��˃x�N�g��
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	// �X�t�B�A�}�b�v�p UV
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	// �e�N�X�`���F
	float4 texColor = tex.Sample(smp, input.uv);
	return max(
		  diffuseB								// �P�x
		* diffuse								// �f�B�q���[�Y�F
		* texColor								// �e�N�X�`���J���[
		* sph.Sample(smp, sphereMapUV)			// �X�t�B�A�}�b�v�i��Z�j
		+ spa.Sample(smp, sphereMapUV)			// �X�t�B�A�}�b�v�i���Z�j
		+ float4(specularB * specular.rgb, 1)	// �X�y�L����
		, float4(ambient * texColor, 1));		// �A���r�G���g

	//return float4(diffuseB, diffuseB, diffuseB, 1)	//�P�x
	//	* diffuse						//�f�B�q���[�Y
	//	* texColor						//�e�N�X�`���J���[
	//	* sph.Sample(smp, sphereMapUV)	//�X�t�B�A�}�b�v�i��Z�j
	//	+ spa.Sample(smp, sphereMapUV)	//�X�t�B�A�}�b�v�i���Z�j
	//	+ float4(texColor * ambient, 1);//�A���r�G���g
}