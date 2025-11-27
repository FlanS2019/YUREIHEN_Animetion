// Title.cpp
#include "Fade.h"
#include "Title.h"
#include "shader.h"
#include "Sprite.h"
#include "Keyboard.h"
#include "SceneManager.h"

static ID3D11ShaderResourceView* g_Texture = NULL;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

void Title_Initialize(ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	//テクスチャ読み込み
	TexMetadata metadata1;
	ScratchImage image1;
	LoadFromWICFile(L"asset\\Title.png",
		WIC_FLAGS_NONE, &metadata1, image1);
	CreateShaderResourceView(pDevice, image1.GetImages(),
		image1.GetImageCount(), metadata1, &g_Texture);
	assert(g_Texture);
}

void Title_Finalize()
{
	if (g_Texture)
	{
		g_Texture->Release();
		g_Texture = NULL;
	}
}
void Title_Update()
{
	if (Keyboard_IsKeyDownTrigger(KK_ENTER) && (GetFadeState() == FADE_NONE))
	{
		XMFLOAT4 color = { 0,0,0,1 };
		SetFade(40, color, FADE_OUT, SCENE_GAME);
	}

	if (Keyboard_IsKeyDownTrigger(KK_E))
	{
		SetScene(SCENE_GAME);
	}
}

void TitleDraw(void)
{

	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth(); // 画面の幅
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight(); // 画面の高さ
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
