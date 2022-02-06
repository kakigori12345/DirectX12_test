
// レジスタから情報を取得するための設定
Texture2D<float4> tex : register(t0);	//0 番スロットに設定されたテクスチャ
Texture2D<float4> sph : register(t1);	//1 ''
Texture2D<float4> spa : register(t2);	//2 ''
Texture2D<float4> toon: register(t3);	//3 ''

SamplerState smp : register(s0);		//0 番スロットに設定されたサンプラー
SamplerState smpToon : register(s1);	//1 ''（トゥーン用）


//定数バッファー
cbuffer SceneBuffer : register(b0) {
	matrix view;
	matrix proj;
	float3 eye;
}
cbuffer Transform : register(b1) {
	matrix world;
}
// マテリアル用
cbuffer Material : register(b2) {
	float4 diffuse;		//ディヒューズ色
	float4 specular;	//スペキュラ
	float3 ambient;		//アンビエント
}

// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION;	// システム用頂点座標
	float4 pos : POSITION;		// 頂点座標
	float4 normal : NORMAL0;	// 法線ベクトル
	float4 vnormal: NORMAL1;	// ビュー変換後の法線ベクトル
	float2 uv : TEXCOORD;		// uv
	float3 ray : VECTOR;		// 視線ベクトル
};
