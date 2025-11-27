/*==============================================================================

   2D描画用頂点シェーダー [shader_vertex_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/


//先生のやつ
// 定数バッファ
float4x4 mtx; //C言語から渡されたデータが入っている

//入力用頂点構造体
struct VS_INPUT
{ //              V コロン！
    float4 posL : POSITION0; //頂点座標 オーでなくゼロ！
    float4 color : COLOR0; //頂点カラー（R,G,B,A）
    float2 texcoord : TEXCOORD0;
};

//出力用頂点構造体
struct VS_OUTPUT
{
    float4 posH : SV_POSITION; //変換済頂点座標
    float4 color : COLOR0; //頂点カラー
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out; //出力用構造体変数
    
    //頂点を行列で変換
    vs_out.posH = mul(vs_in.posL, mtx);
    //頂点カラーはそのまま出力
    vs_out.color = vs_in.color;
    
    vs_out.texcoord = vs_in.texcoord;
    //結果を出力する
    return vs_out;
}



////自分で作ったやつ
////// 定数バッファ
//float4x4 mtx;

////入力頂点構造体
//struct VS_INPUT
//{
//    float4 posL : POSITIONT0; //POSITIONは入れなくてもよい。
//    float4 color : COLOR0;
//};

////出力頂点構造体
//struct VS_OUTPUT
//{
//    float4 posH : SV_POSITION0;
//    float4 color : COLOR0;
//};

//VS_OUTPUT main(VS_INPUT vs_in)
//{
//    VS_OUTPUT vs_out;
//    //頂点を行列で変換
//    vs_out.posH = mul(vs_in.posL, mtx);
//    //頂点カラーはそのまま出力
//    vs_out.color = vs_in.color;
//    //結果を出力する
//    return vs_out;
//}

//=============================================================================
// 頂点シェーダ
//=============================================================================
//float4 main(in float4 posL : POSITION0) : SV_POSITION
//{
//    return mul(posL, mtx);
//}
