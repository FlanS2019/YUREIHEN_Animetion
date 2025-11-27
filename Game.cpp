#include "Game.h"
#include "Keyboard.h"
#include "Fade.h"
#include "SceneManager.h"
#include <DirectXMath.h>
using namespace DirectX;

void Game_Initialize(ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext)
{
	// シーン用初期化処理

}
void Game_Finalize(void)
{
	// シーン用終了処理

}

void Game_Update()
{
	// シーン用処理
	if (Keyboard_IsKeyDownTrigger(KK_ENTER) && (GetFadeState() == FADE_NONE))
	{
		XMFLOAT4 color = { 0,0,0,1 };
		SetFade(40, color, FADE_OUT, SCENE_RESULT);
	}

	if (Keyboard_IsKeyDownTrigger(KK_E))
	{
		SetScene(SCENE_RESULT);
	}

}
void Game_Draw(void)
{
	// シーン用描画処理

}