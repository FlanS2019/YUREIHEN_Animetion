#pragma once

#include <d3d11.h>

// タイトルシーンの更新と描画
void Title_Initialize(ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext);
void Title_Finalize(void);
void TitleDraw(void);
void Title_Update();
