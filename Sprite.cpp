
#include "sprite.h"
#include "shader.h"
#include "direct3d.h"
#include "keyboard.h"

//グローバル変数
static constexpr int NUM_VERTEX = 4; // 使用できる最大頂点数
static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

void InitializeSprite()
{

	g_pDevice = Direct3D_GetDevice();

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

}
void FinalizeSprite()
{
	g_pVertexBuffer->Release();

}
//スプライト描画
void DrawSprite(XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 col)
{
	Shader_Begin();

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);


	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

#define YOKO (10)
#define TEX_W (1.0f / YOKO )


	v[0].position = { pos.x - (size.x / 2), pos.y - (size.y / 2) , 0.0f };
	v[0].color = col;
	v[0].texCoord = { TEX_W * 10 , 0 };

	//v[0].color = { 1,1,1,1 }; //SIRO

	v[1].position = { pos.x + (size.x / 2), pos.y - (size.y / 2) , 0.0f };
	v[1].color = col;
	v[1].texCoord = { TEX_W * 4 + TEX_W, 0 };

	//v[1].color = { 1,0,1,1 };


	v[2].position = { pos.x - (size.x / 2), pos.y + (size.y / 2) , 0.0f };
	v[2].color = col;
	v[2].texCoord = { TEX_W * 4, 1.0 };

	//v[2].color = { 1,2,0,1 }; 

	v[3].position = { pos.x + (size.x / 2), pos.y + (size.y / 2) , 0.0f };
	v[3].color = col;
	v[3].texCoord = { TEX_W * 4 + TEX_W, 1.0 };

	//v[3].color = { 1,1,0,1 };


	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// プリミティブトポロジ設定
	//g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴン描画命令発行
	g_pContext->Draw(4, 0);

	//g_pContext->Draw(NUM_VERTEX, 0);
}

void DrawSpriteEx_two(XMFLOAT2 size,
	XMFLOAT4 col, int bno, int wc, int hc)
{
	Shader_Begin();

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);


	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	//bno の左上のテクスチャ座標を計算
	float w = 1.0f / wc;
	float h = 1.0f / hc;

	//ブロックの縦横サイズを計算
	float x = (bno % wc) * w;
	float y = (bno / wc) * h;

	v[0].position = { -(size.x / 2), -(size.y / 2) , 0.0f };
	v[0].color = col;
	v[0].texCoord = { x, y };

	v[1].position = { (size.x / 2), -(size.y / 2), 0.0f };
	v[1].color = col;
	v[1].texCoord = { x + w, y };


	v[2].position = { -(size.x / 2), (size.y / 2), 0.0f };
	v[2].color = col;
	v[2].texCoord = { x, y + h };


	v[3].position = { (size.x / 2), (size.y / 2), 0.0f };
	v[3].color = col;
	v[3].texCoord = { x + w,y + h };


	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	//g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴン描画命令発行
	//g_pContext->Draw(4, 0);

	g_pContext->Draw(NUM_VERTEX, 0);

}

void DrawSpriteEx(XMFLOAT2 pos, XMFLOAT2 size,
	XMFLOAT4 col, int bno, int wc, int hc)
{
	Shader_Begin();

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);


	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	//bno の左上のテクスチャ座標を計算
	float w = 1.0f / wc;
	float h = 1.0f / hc;

	//ブロックの縦横サイズを計算
	float x = (bno % wc) * w;
	float y = (bno / wc) * h;


	v[0].position = { pos.x - (size.x / 2), (pos.y - size.y / 2) , 0.0f };
	v[0].color = col;
	v[0].texCoord = { x, y };

	v[1].position = { pos.x + (size.x / 2), (pos.y - size.y / 2), 0.0f };
	v[1].color = col;
	v[1].texCoord = { x + w, y };


	v[2].position = { pos.x - (size.x / 2), pos.y + (size.y / 2), 0.0f };
	v[2].color = col;
	v[2].texCoord = { x, y + h };


	v[3].position = { pos.x + (size.x / 2), (pos.y + size.y / 2), 0.0f };
	v[3].color = col;
	v[3].texCoord = { x + w,y + h };

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	//g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴン描画命令発行
	//g_pContext->Draw(4, 0);

	g_pContext->Draw(NUM_VERTEX, 0);

}

//画像をスクロールさせる
void DrawSpriteScroll(XMFLOAT2 pos, XMFLOAT2 size,
	XMFLOAT4 col, XMFLOAT2 texcoord)

{
	Shader_Begin();

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);


	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	v[0].position = { pos.x - (size.x / 2), pos.y - (size.y / 2) , 0.0f };
	v[0].color = col;
	v[0].texCoord = { texcoord.x, texcoord.y };


	v[1].position = { pos.x + (size.x / 2), pos.y - (size.y / 2) , 0.0f };
	v[1].color = col;
	v[1].texCoord = { 1 + texcoord.x, texcoord.y };


	v[2].position = { pos.x - (size.x / 2), pos.y + (size.y / 2) , 0.0f };
	v[2].color = col;
	v[2].texCoord = { texcoord.x, 1 + texcoord.y };


	v[3].position = { pos.x + (size.x / 2), pos.y + (size.y / 2) , 0.0f };
	v[3].color = col;
	v[3].texCoord = { 1 + texcoord.x, 1 + texcoord.y };


	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴン描画命令発行
	g_pContext->Draw(4, 0);
}

void DrawSpriteExRotation(XMFLOAT2 pos, XMFLOAT2 size,
	XMFLOAT4 col, int bno, int wc, int hc, float radian)
{
	Shader_Begin();

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);


	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	//bno の左上のテクスチャ座標を計算
	float w = 1.0f / wc;
	float h = 1.0f / hc;

	//ブロックの縦横サイズを計算
	float x = (bno % wc) * w;
	float y = (bno / wc) * h;


	v[0].position = { -(size.x / 2), -(size.y / 2) , 0.0f };
	v[0].color = col;
	v[0].texCoord = { x, y };

	v[1].position = { (size.x / 2), -(size.y / 2), 0.0f };
	v[1].color = col;
	v[1].texCoord = { x + w, y };


	v[2].position = { -(size.x / 2), (size.y / 2), 0.0f };
	v[2].color = col;
	v[2].texCoord = { x, y + h };


	v[3].position = { (size.x / 2), (size.y / 2), 0.0f };
	v[3].color = col;
	v[3].texCoord = { x + w,y + h };

	float co = cosf(radian);
	float si = sinf(radian);

	for (int i = 0; i < 4; i++)
	{
		float x = v[i].position.x;
		float y = v[i].position.y;
		v[i].position.x = (x * co - y * si) + pos.x;
		v[i].position.y = (x * si + y * co) + pos.y;
	}

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	//g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴン描画命令発行
	g_pContext->Draw(4, 0);

	//g_pContext->Draw(NUM_VERTEX, 0);

}

void DrawSprite_A(XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 col)
{
	Shader_Begin();

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	// フルテクスチャ表示用の素直なUV
	v[0].position = { pos.x - (size.x / 2), pos.y - (size.y / 2) , 0.0f };
	v[0].color = col;
	v[0].texCoord = { 0.0f, 0.0f };

	v[1].position = { pos.x + (size.x / 2), pos.y - (size.y / 2) , 0.0f };
	v[1].color = col;
	v[1].texCoord = { 1.0f, 0.0f };

	v[2].position = { pos.x - (size.x / 2), pos.y + (size.y / 2) , 0.0f };
	v[2].color = col;
	v[2].texCoord = { 0.0f, 1.0f };

	v[3].position = { pos.x + (size.x / 2), pos.y + (size.y / 2) , 0.0f };
	v[3].color = col;
	v[3].texCoord = { 1.0f, 1.0f };

	// ロック解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// 頂点バッファを描画パイプラインに設定（← これが抜けていた）
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// トポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// 描画
	g_pContext->Draw(NUM_VERTEX, 0);
}
// 反転機能付きのスプライト描画関数
void DrawSpriteExFlip(XMFLOAT2 pos, XMFLOAT2 size,
	XMFLOAT4 col, int bno, int wc, int hc, bool flipX, bool flipY)
{
	Shader_Begin();

	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	//bno の左上のテクスチャ座標を計算
	float w = 1.0f / wc;
	float h = 1.0f / hc;

	//ブロックの縦横サイズを計算
	float x = (bno % wc) * w;
	float y = (bno / wc) * h;

	// テクスチャ座標（反転対応）
	float u0 = flipX ? (x + w) : x;
	float u1 = flipX ? x : (x + w);
	float v0 = flipY ? (y + h) : y;
	float v1 = flipY ? y : (y + h);

	v[0].position = { pos.x - (size.x / 2), (pos.y - size.y / 2) , 0.0f };
	v[0].color = col;
	v[0].texCoord = { u0, v0 };

	v[1].position = { pos.x + (size.x / 2), (pos.y - size.y / 2), 0.0f };
	v[1].color = col;
	v[1].texCoord = { u1, v0 };

	v[2].position = { pos.x - (size.x / 2), pos.y + (size.y / 2), 0.0f };
	v[2].color = col;
	v[2].texCoord = { u0, v1 };

	v[3].position = { pos.x + (size.x / 2), (pos.y + size.y / 2), 0.0f };
	v[3].color = col;
	v[3].texCoord = { u1, v1 };

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴン描画命令発行
	g_pContext->Draw(NUM_VERTEX, 0);
}