//fade.cpp
#include "fade.h"
#include "shader.h"
#include "SceneManager.h"

FadeObject g_Fade;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ID3D11ShaderResourceView* g_Texture;//テクスチャ


void Fade_Initialize(ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	// テクスチャ読み込み
	TexMetadata metadata1;
	ScratchImage image1;

	LoadFromWICFile(L"asset\\fade.bmp",
		WIC_FLAGS_NONE, &metadata1, image1);
	CreateShaderResourceView(pDevice, image1.GetImages(),
		image1.GetImageCount(), metadata1, &g_Texture);
	assert(g_Texture);

	g_Fade.fadecolor.x = 0.0f;
	g_Fade.fadecolor.y = 0.0f;
	g_Fade.fadecolor.z = 0.0f;
	g_Fade.fadecolor.w = 1.0f;

	g_Fade.frame = 60;

	g_Fade.state = FADE_STATE::FADE_NONE;
}
void Fade_Finalize(void)
{
	if (g_Texture != NULL) // テクスチャの解放
	{
		g_Texture->Release();
		g_Texture = NULL;
	}

}
void Fade_Update(void)
{

}

void Fade_Draw(void)
{
	switch (g_Fade.state)
	{
	case FADE_STATE::FADE_NONE:
		return;
	case FADE_STATE::FADE_IN:
		if (g_Fade.fadecolor.w < 0)
		{
			g_Fade.fadecolor.w = 0.0f;
			g_Fade.state = FADE_STATE::FADE_NONE;
		}
		break;

	case FADE_STATE::FADE_OUT:
		if (g_Fade.fadecolor.w > 1.0f)
		{
			g_Fade.fadecolor.w = 1.0f;

			SetFade(g_Fade.frame, g_Fade.fadecolor,
				FADE_STATE::FADE_IN, g_Fade.scene);

			SetScene(g_Fade.scene);

		}
		break;

	default:
		break;
	}

	// 画面の幅と高さを取得
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth(); // 画面の幅
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight(); // 画面の高さ

	// シェーダーの開始
	Shader_Begin();

	// 画面の幅と高さを使って正射影行列を設定
	XMMATRIX Projection = XMMatrixOrthographicOffCenterLH
	(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f); // 左、右、上、下、近、遠の順

	g_pContext->PSSetShaderResources(0, 1, &g_Texture); // シェーダーにテクスチャを設定
	// スプライトの位置、サイズ、色を設定
	SetBlendState(BLENDSTATE_ALFA); // アルファブレンドを有効にする
	XMFLOAT2 pos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }; // 画面全体の中心
	XMFLOAT2 size = { SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2 }; // 画面全体
	DrawSprite(pos, size, g_Fade.fadecolor); // スプライトを描画

	switch (g_Fade.state)
	{
	case FADE_STATE::FADE_IN:
		g_Fade.fadecolor.w -= (0.3 / g_Fade.frame);
		break;
	case FADE_STATE::FADE_OUT:
		g_Fade.fadecolor.w += (1.0f / g_Fade.frame);
		break;

	default:
		break;
	}
}

void SetFade(int fadeframe, XMFLOAT4 color,
	FADE_STATE state, SCENE scene)
{
	g_Fade.frame = fadeframe;
	g_Fade.fadecolor = color;
	g_Fade.state = state;
	g_Fade.scene = scene;
	if (g_Fade.state == FADE_STATE::FADE_IN)
	{
		g_Fade.fadecolor.w = 1.0f;
	}
	else
	{
		g_Fade.fadecolor.w = 0.0f;
	}

}


FADE_STATE GetFadeState()
{
	return FADE_NONE;
	//return g_Fade.state;
}

bool Fade_IsFinished()
{
	return g_Fade.state == FADE_NONE;
}