
// ���W�X�^��������擾���邽�߂̐ݒ�
Texture2D<float4> tex : register(t0);	//0 �ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph : register(t1);	//1 ''
Texture2D<float4> spa : register(t2);	//2 ''
Texture2D<float4> toon: register(t3);	//3 ''

SamplerState smp : register(s0);		//0 �ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerState smpToon : register(s1);	//1 ''�i�g�D�[���p�j


//�萔�o�b�t�@�[
cbuffer SceneBuffer : register(b0) {
	matrix view;
	matrix proj;
	float3 eye;
}
cbuffer Transform : register(b1) {
	matrix world;
}
// �}�e���A���p
cbuffer Material : register(b2) {
	float4 diffuse;		//�f�B�q���[�Y�F
	float4 specular;	//�X�y�L����
	float3 ambient;		//�A���r�G���g
}

// ���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos : SV_POSITION;	// �V�X�e���p���_���W
	float4 pos : POSITION;		// ���_���W
	float4 normal : NORMAL0;	// �@���x�N�g��
	float4 vnormal: NORMAL1;	// �r���[�ϊ���̖@���x�N�g��
	float2 uv : TEXCOORD;		// uv
	float3 ray : VECTOR;		// �����x�N�g��
};
