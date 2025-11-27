#pragma once
#include <d3d11.h>

void Game_Initialize(ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext);
void Game_Finalize(void);
void Game_Update();
void Game_Draw(void);

