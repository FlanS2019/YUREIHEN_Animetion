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

// 各画像のアルファ管理（全て 500x500 で描く）
// index: 0 = yakata_jimen (館) — フェードしない（常時 1.0）
//        1 = yurei1     (幽霊) — フェードインして館付近で浮遊→必要時に館へ移動
//        2 = basuta1    (移動オブジェクト) — 右から来て館へ移動
static float alpha[3] = { 1.0f, 0.0f, 0.0f };

static int currentImage = 0;   // 未使用だが残す
static float imageTimer = 0.0f; // 未使用だが残す

// 幽霊アニメーション用（位置・回転管理）
static XMFLOAT2 g_ghostOffset = { 0.0f, 0.0f };
static float g_ghostBobRotation = 0.0f;    // bobbing 用の小さい回転
static float g_ghostAngle = 0.0f;          // 幽霊の基準角 (ラジアン)
static float g_ghostTargetAngle = 0.0f;    // 目標角 (ラジアン)

// 状態管理
enum GhostState { GHOST_IDLE = 0, GHOST_ALERT, GHOST_MOVE_TO_HOUSE };
static GhostState g_ghostState = GHOST_IDLE;

static const XMFLOAT2 g_imageSize = { 500.0f, 500.0f }; // 全画像 500x500 固定

// 屋敷　幽霊　ばすたの位置管理
static XMFLOAT2 g_yakataPos = { 0.0f, 0.0f };   // 屋敷は左端に固定（フェードなし）
static XMFLOAT2 g_ghostPos = { 0.0f, 0.0f };    // 幽霊の現在位置
static XMFLOAT2 g_basutaPos = { 0.0f, 0.0f };   // basuta の現在位置（右→館へ移動）
static XMFLOAT2 g_basutaTarget = { 0.0f, 0.0f };
static bool g_positionsInitialized = false;
static bool g_basutaMoving = false;

static const float PI = 3.14159265358979323846f;

// 角度差を -PI..PI に正規化
static float AngleDelta(float target, float current)
{
    float diff = target - current;
    while (diff > PI) diff -= 2.0f * PI;
    while (diff < -PI) diff += 2.0f * PI;
    return diff;
}

void Logo_Initialize(ID3D11Device* pDevice,
    ID3D11DeviceContext* pContext)
{
    // デバイスとデバイスコンテキストの保存
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

    // yakata は常に表示（フェード無し）
    alpha[0] = 1.0f;
    alpha[1] = 0.0f;
    alpha[2] = 0.0f;

    // 初期角度
    g_ghostAngle = 0.0f;
    g_ghostTargetAngle = 0.0f;
    g_ghostState = GHOST_IDLE;
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

    // 初期位置を最初のアップデートで決定（画面サイズ参照）
    if (!g_positionsInitialized)
    {
        const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
        const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();
        XMFLOAT2 center = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

        // yakata を画面左端に配置（中心座標）
        const float leftMargin = 20.0f; // 左端からのマージン
        g_yakataPos.x = (g_imageSize.x / 2.0f) + leftMargin;
        g_yakataPos.y = center.y;

        // 幽霊の初期位置は館の前方（館付近で浮遊させる）
        g_ghostPos.x = g_yakataPos.x + (g_imageSize.x * 0.35f);
        g_ghostPos.y = g_yakataPos.y - 40.0f;

        // basuta は画面右外から来る
        g_basutaPos.x = SCREEN_WIDTH + (g_imageSize.x / 2.0f) + 100.0f;
        g_basutaPos.y = center.y + 30.0f;

        // basuta の目標は館の前方（幽霊より少し下）
        g_basutaTarget.x = g_yakataPos.x + (g_imageSize.x * 0.25f);
        g_basutaTarget.y = g_yakataPos.y + 20.0f;

        g_positionsInitialized = true;
        g_basutaMoving = false; // タイミングで開始
    }

    // フェード timing / durations
    const float ghostStart = 0.5f;    // 幽霊フェード開始
    const float basutaStart = 2.0f;   // basuta フェード/移動開始
    const float fadeDuration = 0.8f;

    // 幽霊フェードイン（館近くに常駐、ふわふわは下で加算）
    if (timer > ghostStart)
    {
        alpha[1] += delta / fadeDuration;
        if (alpha[1] > 1.0f) alpha[1] = 1.0f;
    }

    // basuta フェードインと移動開始
    if (timer > basutaStart)
    {
        // フェードイン
        alpha[2] += delta / fadeDuration;
        if (alpha[2] > 1.0f) alpha[2] = 1.0f;
        // 移動開始
        g_basutaMoving = true;
    }

    // basuta の移動処理（右から館へ）
    if (g_basutaMoving)
    {
        const float moveSpeed = 220.0f; // px / sec
        float dx = g_basutaTarget.x - g_basutaPos.x;
        float dy = g_basutaTarget.y - g_basutaPos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 1.0f)
        {
            float nx = dx / dist;
            float ny = dy / dist;
            float step = moveSpeed * delta;
            if (step >= dist)
            {
                g_basutaPos.x = g_basutaTarget.x;
                g_basutaPos.y = g_basutaTarget.y;
                g_basutaMoving = false;
            }
            else
            {
                g_basutaPos.x += nx * step;
                g_basutaPos.y += ny * step;
            }
        }
        else
        {
            g_basutaPos.x = g_basutaTarget.x;
            g_basutaPos.y = g_basutaTarget.y;
            g_basutaMoving = false;
        }
    }

    // basuta が近づいたら幽霊が振り向き、その後館へ向かうロジック
    {
        // 発動距離
        const float triggerDist = 220.0f;

        // 距離計算
        float dx = g_basutaPos.x - g_ghostPos.x;
        float dy = g_basutaPos.y - g_ghostPos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (g_ghostState == GHOST_IDLE)
        {
            // basuta が近づき、basuta がフェードイン済みなら警戒状態へ
            if (dist <= triggerDist && alpha[2] > 0.3f)
            {
                g_ghostState = GHOST_ALERT;
                // basuta の方を向く
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
            }
        }
        else if (g_ghostState == GHOST_ALERT)
        {
            // 回転速度（ラジアン/秒）
            const float rotSpeed = 3.5f;
            float deltaAngle = AngleDelta(g_ghostTargetAngle, g_ghostAngle);
            float maxStep = rotSpeed * delta;
            if (fabsf(deltaAngle) <= maxStep)
            {
                // 回転完了 → 館へ向かって移動開始
                g_ghostAngle = g_ghostTargetAngle;
                g_ghostState = GHOST_MOVE_TO_HOUSE;
            }
            else
            {
                // スムーズに回転
                g_ghostAngle += (deltaAngle > 0.0f ? 1.0f : -1.0f) * maxStep;
            }
        }
        else if (g_ghostState == GHOST_MOVE_TO_HOUSE)
        {
            // 移動速度（幽霊） px/sec
            const float ghostMoveSpeed = 120.0f;
            // 目的地は館の正面（g_yakataPos）
            float tx = g_yakataPos.x + (g_imageSize.x * 0.25f); // 少し館寄りを狙う
            float ty = g_yakataPos.y - 10.0f;
            float dxh = tx - g_ghostPos.x;
            float dyh = ty - g_ghostPos.y;
            float distH = sqrtf(dxh * dxh + dyh * dyh);
            if (distH > 2.0f)
            {
                float nx = dxh / distH;
                float ny = dyh / distH;
                float step = ghostMoveSpeed * delta;
                if (step >= distH)
                {
                    g_ghostPos.x = tx;
                    g_ghostPos.y = ty;
                    // 到着したら停止（角度は変更しない）
                    g_ghostState = GHOST_IDLE;
                }
                else
                {
                    g_ghostPos.x += nx * step;
                    g_ghostPos.y += ny * step;
                    // 移動方向に角度を更新（スムーズな回転）
                    float targetMoveAngle = atan2f(ny, nx);
                    float moveDeltaAngle = AngleDelta(targetMoveAngle, g_ghostAngle);
                    float moveRotSpeed = 2.5f; // 移動中の回転速度は少し遅く
                    float moveMaxStep = moveRotSpeed * delta;
                    if (fabsf(moveDeltaAngle) <= moveMaxStep)
                    {
                        g_ghostAngle = targetMoveAngle;
                    }
                    else
                    {
                        g_ghostAngle += (moveDeltaAngle > 0.0f ? 1.0f : -1.0f) * moveMaxStep;
                    }
                }
            }
            else
            {
                // 既に到着
                g_ghostState = GHOST_IDLE;
            }
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

    // 幽霊のふわふわ（基準位置 g_ghostPos に対して揺らす）
    // bobbing は角度に微小変化を付与して自然に見せる（ただし移動中は控えめ）
    if (g_ghostState == GHOST_MOVE_TO_HOUSE)
    {
        g_ghostOffset.y = sinf(timer * 2.0f) * 6.0f;
        g_ghostOffset.x = sinf(timer * 0.7f) * 3.0f;
        g_ghostBobRotation = sinf(timer * 1.2f) * 0.03f;
    }
    else
    {
        g_ghostOffset.y = sinf(timer * 2.5f) * 12.0f;   // 上下ゆらぎ（振幅12px）
        g_ghostOffset.x = sinf(timer * 0.9f) * 6.0f;    // 左右わずかに揺れる
        g_ghostBobRotation = sinf(timer * 1.2f) * 0.06f;   // わずかな追加回転
    }

    if (Keyboard_IsKeyDownTrigger(KK_E))
    {
        // Eキーで即座にタイトルへ（デバッグ用）
        SetScene(SCENE_TITLE);
    }
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

    // 1) 屋敷 (index 0) を画面左端に固定（500x500） — フェード無し
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[0]);
        XMFLOAT4 col = { 1.0f, 1.0f, 1.0f, 1.0f }; // 常に 1.0
        DrawSpriteEx(g_yakataPos, g_imageSize, col, 0, 1, 1);
    }

    // 2) 幽霊 (index 1)
    if (alpha[1] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[1]);
        XMFLOAT4 ghostCol = { 1.0f, 1.0f, 1.0f, alpha[1] };
        // 描画回転 = 基準角 + bobbing 微小角度
        float drawRotation = g_ghostAngle + g_ghostBobRotation;
        XMFLOAT2 drawPos = { g_ghostPos.x + g_ghostOffset.x, g_ghostPos.y + g_ghostOffset.y };
        DrawSpriteExRotation(drawPos, g_imageSize, ghostCol, 0, 1, 1, drawRotation);
    }

    // 3) basuta (index 2) — 右から来て館の前に移動
    if (alpha[2] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[2]);
        XMFLOAT4 col2 = { 1.0f, 1.0f, 1.0f, alpha[2] };
        DrawSpriteEx(g_basutaPos, g_imageSize, col2, 0, 1, 1);
    }
}