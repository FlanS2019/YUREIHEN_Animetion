//sprite.h

#pragma once

#include <d3d11.h>
#include "direct3d.h"
#include <DirectXMath.h>

using namespace DirectX;

void DrawSprite(XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 col);

void DrawSpriteEx(XMFLOAT2 pos, XMFLOAT2 size,
	XMFLOAT4 col, int bno, int wc, int hc);

void InitializeSprite(); //	スプライト初期化
void FinalizeSprite();   //スプライト終了
void DrawSpriteScroll(XMFLOAT2 pos, XMFLOAT2 size,
	XMFLOAT4 col, XMFLOAT2 texcoord);

void DrawSpriteEx_two(XMFLOAT2 size,
	XMFLOAT4 col, int bno, int wc, int hc);

void DrawSpriteExRotation(XMFLOAT2 pos, XMFLOAT2 size,
	XMFLOAT4 col, int bno, int wc, int hc, float radian);

void DrawSprite_A(XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 col);

void DrawSpriteExFlip(XMFLOAT2 pos, XMFLOAT2 size,
	XMFLOAT4 col, int bno, int wc, int hc, bool flipX, bool flipY);



// 頂点構造体
struct Vertex
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4 color; //頂点カラー(R.G.B.A)
	XMFLOAT2 texCoord; //テクスチャ座標
};

ID3D11ShaderResourceView* Sprite_LoadTexture(ID3D11Device* pDevice, const wchar_t* path);

