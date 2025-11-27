// Result.cpp
#include "Result.h"
#include "Keyboard.h"
#include "Fade.h"
#include "SceneManager.h"
#include "shader.h"

#include <DirectXMath.h>
using namespace DirectX;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11ShaderResourceView* g_Texture;//テクスチャ４枚分

void Result_Initialize(ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext)
{
	// 結果画面用初期化処理

	g_pDevice = pDevice;
	g_pContext = pContext;

	// テクスチャ読み込み
	TexMetadata metadata1;
	ScratchImage image1;

	LoadFromWICFile(L"asset\\owari.png",
		WIC_FLAGS_NONE, &metadata1, image1);
	CreateShaderResourceView(pDevice, image1.GetImages(),
		image1.GetImageCount(), metadata1, &g_Texture);
	assert(g_Texture);

}
void Result_Finalize(void)
{
	// 結果画面用終了処理
	SAFE_RELEASE(g_Texture);//安全に解放

}
void Result_Update()
{
	// 結果画面用処理
	if (Keyboard_IsKeyDownTrigger(KK_ENTER) && (GetFadeState() == FADE_NONE))
	{
		XMFLOAT4 color = { 0,0,0,1 };
		SetFade(40, color, FADE_OUT, SCENE_TITLE);
	}

	if (Keyboard_IsKeyDownTrigger(KK_E))
	{
		SetScene(SCENE_TITLE);
	}


}
void Result_Draw(void)
{
	// 結果画面用描画処理
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth(); // 画面の幅
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight(); // 画面の高さ

	//XMFLOAT2 pos = { 0, 0 };
	//XMFLOAT2 size = { SCREEN_WIDTH, SCREEN_HEIGHT };
	//XMFLOAT4 color = { 1, 1, 1, 1 };
	// 中心座標を指定（DrawSprite は中心基準）
		// 頂点シェーダーに変換行列を設定
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH
	(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	g_pContext->PSSetShaderResources(0, 1, &g_Texture);

	//画像サイズのスプライトを表示
	SetBlendState(BLENDSTATE_ALFA);

	XMFLOAT2 pos = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
	XMFLOAT2 size = { SCREEN_WIDTH, SCREEN_HEIGHT };
	XMFLOAT4 color = { 1, 1, 1, 1 };

	g_pContext->PSSetShaderResources(0, 1, &g_Texture);
	DrawSprite_A(pos, size, color);

}


