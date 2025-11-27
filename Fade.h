//fade.h
#pragma once

#include "direct3d.h"
#include "Sprite.h"
#include "SceneManager.h"

enum FADE_STATE
{
	FADE_NONE = 0,		// フェードなし
	FADE_IN,		// フェードイン中
	FADE_OUT,		// フェードアウト中

	FADE_MAX
};

struct FadeObject
{
	FADE_STATE state;	// フェード状態
	float count;		// α値
	float frame;		// α値
	XMFLOAT4 fadecolor;		// フェードカラー
	SCENE scene;		// フェード後のシーン


};

void Fade_Initialize(ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext);
void Fade_Finalize(void);
void Fade_Update(void);
void Fade_Draw(void);

void SetFade(int fadeframe, XMFLOAT4 color,
	FADE_STATE state, SCENE scene);

FADE_STATE GetFadeState();
bool Fade_IsFinished();