#pragma once
#include <d3d11.h>


void Result_Initialize(ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext);
void Result_Finalize(void);
void Result_Update();
void Result_Draw(void);
