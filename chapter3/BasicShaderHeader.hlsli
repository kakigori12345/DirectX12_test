
// レジスタから情報を取得するための設定
Texture2D<float4> tex : register(t0);	//0番スロットに設定されたテクスチャ
SamplerState smp : register(s0);		//0番スロットに設定されたサンプラー
cbuffer cbuff0 : register(b0) {			//定数バッファー
	matrix world;
	matrix viewproj;
}

// 定数バッファ1
// マテリアル用
cbuffer Material : register(b1) {
	float4 diffuse;		//ディヒューズ色
	float4 specular;	//スペキュラ
	float3 ambient;		//アンビエント
}

// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION;	// システム用頂点座標
	float4 normal : NORMAL;		// 法線ベクトル
	float2 uv : TEXCOORD;		// uv
};
