#include "SceneManager.h"
#include "direct3d.h"
#include "Keyboard.h" 
#include "Title.h"
#include "Fade.h"
#include "Result.h"
#include "Game.h"
#include "Logo.h"
#include <DirectXMath.h>

using namespace DirectX;

static SCENE g_Scene = SCENE_NONE;

void SceneManager_Initialize()
{
	Fade_Initialize(Direct3D_GetDevice(),
	Direct3D_GetDeviceContext());

	XMFLOAT4 color = XMFLOAT4{ 0,0,0,1 };
	SetFade(60, color, FADE_STATE::FADE_IN, SCENE_GAME);
	SetScene(SCENE_LOGO);
}

void SceneManager_Finalize()
{
	Fade_Finalize();
	SetScene(SCENE_NONE);
}

void SceneManager_Update()
{
	switch (g_Scene)
	{
	case SCENE_NONE:
		break;
	case SCENE_LOGO:
		Logo_Update();
		break;
	case SCENE_TITLE:
		Title_Update();
		break;
	case SCENE_GAME:
		Game_Update();
		break;
	case SCENE_RESULT:
		Result_Update();
		break;
	default:
		break;
	}
	Fade_Update();
}

void SceneManager_Draw()
{
	switch (g_Scene)
	{
	case SCENE_NONE:
		break;
	case SCENE_LOGO:
		LogoDraw();
		break;
	case SCENE_TITLE:
		TitleDraw();
		break;
	case SCENE_GAME:
		Game_Draw();
		break;
	case SCENE_RESULT:
		Result_Draw();
		break;
	default:
		break;
	}
	Fade_Draw();
}

void SetScene(SCENE scene)
{
	switch (g_Scene)
	{
	case SCENE_NONE:
		break;
	case SCENE_LOGO:
		Logo_Finalize();
		break;
	case SCENE_TITLE:
		Title_Finalize();
		break;
	case SCENE_GAME:
		Game_Finalize();
		break;
	case SCENE_RESULT:
		Result_Finalize();
		break;
	default:
		break;
	}
	g_Scene = scene;
	XMFLOAT4 color = XMFLOAT4{ 0,0,0,1 };
	SetFade(60, color, FADE_STATE::FADE_IN, scene);
	
	switch (g_Scene)
	{
	case SCENE_NONE:
		break;
	case SCENE_LOGO:
		Logo_Initialize(Direct3D_GetDevice(),
			Direct3D_GetDeviceContext());
		break;
	case SCENE_TITLE:
		Title_Initialize(Direct3D_GetDevice(),
			Direct3D_GetDeviceContext());
		break;
	case SCENE_GAME:
		Game_Initialize(Direct3D_GetDevice(),
			Direct3D_GetDeviceContext());
		break;
	case SCENE_RESULT:
		Result_Initialize(Direct3D_GetDevice(),
			Direct3D_GetDeviceContext());
		break;
	default:
		break;
	}
}
