
// ���W�X�^��������擾���邽�߂̐ݒ�
Texture2D<float4> tex : register(t0);	//0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp : register(s0);		//0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
cbuffer cbuff0 : register(b0) {			//�萔�o�b�t�@�[
	matrix mat;	//�ϊ��s��
}

// ���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos : SV_POSITION;	// �V�X�e���p���_���W
	float4 normal : NORMAL;		// �@���x�N�g��
	float2 uv : TEXCOORD;		// uv
};
