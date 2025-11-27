#ifndef DIRECT3D_H
#define DIRECT3D_H

#include <Windows.h>
#include <d3d11.h>
#include "DirectXTex.h"
#include "mmsystem.h"

#if _DEBUG
#pragma comment(lib,"DirectXTex_Debug.lib")
#else
#pragma comment(lib,"DirectXTex_Release.lib")
#endif


#define SAFE_RELEASE(o) if (o) { (o)->Release(); o = NULL; }



bool Direct3D_Initialize(HWND hWnd);
void Direct3D_Finalize();

void Direct3D_Clear();
void Direct3D_Present();

ID3D11Device* Direct3D_GetDevice();
ID3D11DeviceContext* Direct3D_GetDeviceContext();

unsigned int Direct3D_GetBackBufferWidth();
unsigned int Direct3D_GetBackBufferHeight();

enum BLENDSTATE
{
	BLENDSTATE_NONE = 0,
	BLENDSTATE_ALFA,
	BLENDSTATE_ADD,
	BLENDSTATE_SUB,

	BLENDSTATE_MAX
};

void SetBlendState(BLENDSTATE blend);

#define BLOCK_COLS (6)
#define BLOCK_ROWS (13)


//ブロックサイズ
#define BLOCK_WIDTH (50.0f) // ブロックの幅
#define BLOCK_HEIGHT (50.0f) // ブロックの高さ

#define POSITION_OFFESET_X (490.0f)
#define POSITION_OFFESET_Y (34.0f)


#endif /* !DIRECT3D_H*/
