// Logo.cpp
#include "Fade.h"
#include "Logo.h"
#include "shader.h"
#include "Sprite.h"
#include "Keyboard.h"
#include "SceneManager.h"
#include <cmath>
#include <d3d11.h>

static ID3D11ShaderResourceView* g_Texture[4];
static ID3D11ShaderResourceView* g_SolidTex = nullptr; // 1x1 白テクスチャ（背景用に色を乗算）
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static bool fadeStarted = false;

static float ghostAlpha = 0.0f;
static bool ghostFadeStarted = false;

static float ghostAlpha2 = 0.0f;
static bool ghostFadeStarted2 = false;

// 各画像のアルファ管理（全て 500x500 で描く）
static float alpha[3] = { 0.0f, 0.0f, 0.0f };

static int currentImage = 0;   // 現在表示中の画像番号
static float imageTimer = 0.0f; // 画像切替タイマー

// 幽霊アニメーション用
static XMFLOAT2 g_ghostOffset = { 0.0f, 0.0f };
static XMFLOAT2 g_ghostOffset2 = { 0.0f, 0.0f };
static float g_ghostRotation = 0.0f;
static float g_ghostRotation2 = 0.0f;
static const XMFLOAT2 g_imageSize = { 500.0f, 500.0f }; // 全画像 500x500 固定

// 左端の屋敷（yakata_jimen）の中心位置、幽霊の移動位置管理
static XMFLOAT2 g_yakataPos = { 0.0f, 0.0f };
static XMFLOAT2 g_ghostPos = { 0.0f, 0.0f };
static XMFLOAT2 g_ghostTarget = { 0.0f, 0.0f };
static bool g_positionsInitialized = false;
static bool g_ghostMoving = true;

void Logo_Initialize(ID3D11Device* pDevice,
    ID3D11DeviceContext* pContext)
{
    // デバイスとデバイスコンテキストの保存aaaaaaaa
    g_pDevice = pDevice;
    g_pContext = pContext;

    //テクスチャ読み込み
    TexMetadata metadata1;
    ScratchImage image1;
    LoadFromWICFile(L"asset\\yureihen\\yakata_jimen1.png",
        WIC_FLAGS_NONE, &metadata1, image1);
    CreateShaderResourceView(pDevice, image1.GetImages(),
        image1.GetImageCount(), metadata1, &g_Texture[0]);
    assert(g_Texture[0]);

    LoadFromWICFile(L"asset\\yureihen\\yurei1.png",
        WIC_FLAGS_NONE, &metadata1, image1);
    CreateShaderResourceView(pDevice, image1.GetImages(),
        image1.GetImageCount(), metadata1, &g_Texture[1]);
    assert(g_Texture[1]);

    LoadFromWICFile(L"asset\\yureihen\\basuta1.png",
        WIC_FLAGS_NONE, &metadata1, image1);
    CreateShaderResourceView(pDevice, image1.GetImages(),
        image1.GetImageCount(), metadata1, &g_Texture[2]);
    assert(g_Texture[2]);

    // 背景用の 1x1 白テクスチャを作成（色を乗算して紫にする）
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    UINT32 white = 0xFFFFFFFFu; // RGBA = 255,255,255,255
    D3D11_SUBRESOURCE_DATA srd = {};
    srd.pSysMem = &white;
    srd.SysMemPitch = 4;

    ID3D11Texture2D* solidTex = nullptr;
    if (SUCCEEDED(pDevice->CreateTexture2D(&td, &srd, &solidTex)) && solidTex)
    {
        pDevice->CreateShaderResourceView(solidTex, nullptr, &g_SolidTex);
        solidTex->Release();
    }
}

void Logo_Finalize()
{
    for (int i = 0; i < 3; i++)
    {
        if (g_Texture[i])
        {
            g_Texture[i]->Release();
            g_Texture[i] = nullptr;
        }
    }

    if (g_SolidTex)
    {
        g_SolidTex->Release();
        g_SolidTex = nullptr;
    }
}

void Logo_Update()
{
    static float timer = 0.0f;
    static bool waitStarted = false;
    static float waitTimer = 0.0f;

    // 時間差（60FPS固定想定）
    const float delta = 1.0f / 60.0f;
    timer += delta;
    imageTimer += delta;

    // 最初のフレームで位置を決める（画面サイズが必要なためここで初期化）
    if (!g_positionsInitialized)
    {
        const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
        const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();
        XMFLOAT2 center = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

        // yakata を画面左端に配置（中心座標をセット）
        const float leftMargin = 20.0f; // 左端からのマージン
        g_yakataPos.x = (g_imageSize.x / 2.0f) + leftMargin; // 500幅の左側中心座標
        g_yakataPos.y = center.y;

        // 幽霊は画面中央からスタート
        g_ghostPos = center;

        // 幽霊の到着目標は屋敷の中心より少し右（屋敷の正面に来るイメージ）
        g_ghostTarget.x = g_yakataPos.x + (g_imageSize.x * 0.35f); // 屋敷中心より右寄せ
        g_ghostTarget.y = g_yakataPos.y - 30.0f; // 少し上に止める
        g_positionsInitialized = true;
        g_ghostMoving = true;
    }

    // 画像表示/切替パラメータ
    const float displayDuration = 3.0f; // 1枚あたり表示時間
    const float fadeDuration = 0.8f;    // フェード時間

    // 画像自動切替処理
    if (imageTimer > displayDuration)
    {
        currentImage = (currentImage + 1) % 3;
        imageTimer = 0.0f;
    }

    // 各画像のフェードイン / フェードアウトを管理（確実に0になるようにクランプ）
    for (int i = 0; i < 3; ++i)
    {
        if (i == currentImage)
        {
            // フェードイン
            alpha[i] += delta / fadeDuration;
            if (alpha[i] > 1.0f) alpha[i] = 1.0f;
        }
        else
        {
            // フェードアウト
            alpha[i] -= delta / fadeDuration;
            if (alpha[i] < 0.0f) alpha[i] = 0.0f;
        }
    }

    // 全体フェード開始（遷移）
    if (!fadeStarted && timer > 7.0f && GetFadeState() == FADE_NONE)
    {
        fadeStarted = true;
        XMFLOAT4 color = { 0, 0, 0, 1 };
        SetFade(90, color, FADE_OUT, SCENE_TITLE);
    }

    if (fadeStarted && !waitStarted && GetFadeState() == FADE_NONE)
    {
        waitStarted = true;
        waitTimer = 0.0f;
    }

    if (waitStarted)
    {
        waitTimer += delta;
        if (waitTimer >= 1.5f)
        {
            SetScene(SCENE_TITLE);
        }
    }

    // 幽霊の移動（ターゲットへ線形移動、到達すると停止してふわふわのみ継続）
    if (g_ghostMoving)
    {
        // 移動速度（ピクセル / 秒）
        const float moveSpeed = 140.0f;
        float dx = g_ghostTarget.x - g_ghostPos.x;
        float dy = g_ghostTarget.y - g_ghostPos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 1.0f)
        {
            float nx = dx / dist;
            float ny = dy / dist;
            float step = moveSpeed * delta;
            if (step >= dist)
            {
                g_ghostPos.x = g_ghostTarget.x;
                g_ghostPos.y = g_ghostTarget.y;
                g_ghostMoving = false;
            }
            else
            {
                g_ghostPos.x += nx * step;
                g_ghostPos.y += ny * step;
            }
        }
        else
        {
            g_ghostPos.x = g_ghostTarget.x;
            g_ghostPos.y = g_ghostTarget.y;
            g_ghostMoving = false;
        }
    }

    // ---- 幽霊のふわふわ（到達後も小さく揺れる） ----
    // timer は上でインクリメント済み
    g_ghostOffset.y = sinf(timer * 2.5f) * 16.0f;   // 上下ゆらぎ（振幅16px）
    g_ghostOffset.x = sinf(timer * 0.9f) * 6.0f;    // 左右わずかに揺れる
    g_ghostRotation = sinf(timer * 1.2f) * 0.10f;   // わずかな回転（ラジアン）

    // 2番幽霊用（未使用でも保持）
    g_ghostOffset2.y = sinf((timer + 1.0f) * 1.8f) * 14.0f;
    g_ghostOffset2.x = cosf(timer * 0.7f) * 6.0f;
    g_ghostRotation2 = sinf((timer + 0.5f) * 1.1f) * 0.08f;
}

void LogoDraw(void)
{
    const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
    const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

    Shader_SetMatrix(XMMatrixOrthographicOffCenterLH
    (0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

    SetBlendState(BLENDSTATE_ALFA);

    XMFLOAT2 center = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

    // 背景を紫で塗る（1x1 白テクスチャに色を乗算）
    if (g_SolidTex)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_SolidTex);
        XMFLOAT4 purple = { 0.45f, 0.10f, 0.45f, 1.0f };
        XMFLOAT2 fullSize = { SCREEN_WIDTH, SCREEN_HEIGHT };
        DrawSprite_A(center, fullSize, purple);
    }

    // 1) 屋敷 (index 0) を画面左端に固定（500x500）
    if (alpha[0] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[0]);
        XMFLOAT4 col = { 1.0f, 1.0f, 1.0f, alpha[0] };
        DrawSpriteEx(g_yakataPos, g_imageSize, col, 0, 1, 1);
    }

    // 2) 幽霊 (index 1) を屋敷の前方へ移動させて止め、ふわふわと回転させる
    if (alpha[1] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[1]);
        XMFLOAT4 ghostCol = { 1.0f, 1.0f, 1.0f, alpha[1] };
        XMFLOAT2 drawPos = { g_ghostPos.x + g_ghostOffset.x, g_ghostPos.y + g_ghostOffset.y };
        DrawSpriteExRotation(drawPos, g_imageSize, ghostCol, 0, 1, 1, g_ghostRotation);
    }

    // 3) その他 (index 2) は右寄せ・フェードで描画（例として右下へ）
    if (alpha[2] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[2]);
        XMFLOAT4 col2 = { 1.0f, 1.0f, 1.0f, alpha[2] };
        XMFLOAT2 pos2 = { center.x + 120.0f, center.y + 30.0f };
        DrawSpriteEx(pos2, g_imageSize, col2, 0, 1, 1);
    }
}