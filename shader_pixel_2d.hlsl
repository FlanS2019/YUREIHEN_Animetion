/*==============================================================================

   2D描画用ピクセルシェーダー [shader_pixel_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);
//入力頂点構造体

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

float4 main(PS_INPUT ps_in) : SV_Target
{
    float4 col;
    col = g_Texture.Sample(g_SamplerState, ps_in.texcoord);
    col *= ps_in.color;
    
    return col;
}

//float4 main() : SV_TARGET
//{
//    return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}
