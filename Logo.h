#pragma once
#include <d3d11.h>


void Logo_Initialize(ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext);
void Logo_Finalize(void);
void Logo_Update();
void LogoDraw(void);

