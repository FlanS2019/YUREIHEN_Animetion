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
static ID3D11ShaderResourceView* g_SolidTex = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static bool fadeStarted = false;

static float alpha[3] = { 1.0f, 0.0f, 0.0f };

static int currentImage = 0;
static float imageTimer = 0.0f;

// 幽霊アニメーション用
static XMFLOAT2 g_ghostOffset = { 0.0f, 0.0f };
static float g_ghostBobRotation = 0.0f;
static float g_ghostAngle = 0.0f;
static float g_ghostTargetAngle = 0.0f;
static bool g_ghostFacingLeft = false; // 幽霊の向き（true = 左向き）
static float g_ghostScale = 1.0f;      // 幽霊のスケール（縮小用）

// basuta アニメーション用
static XMFLOAT2 g_basutaOffset = { 0.0f, 0.0f };

// 状態管理
enum GhostState { GHOST_IDLE = 0, GHOST_ALERT, GHOST_MOVE_TO_HOUSE };
static GhostState g_ghostState = GHOST_IDLE;

static const XMFLOAT2 g_imageSize = { 500.0f, 500.0f };

// 位置管理
static XMFLOAT2 g_yakataPos = { 0.0f, 0.0f };
static XMFLOAT2 g_ghostPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaTarget = { 0.0f, 0.0f };
static bool g_positionsInitialized = false;
static bool g_basutaMoving = false;

static const float PI = 3.14159265358979323846f;

static float AngleDelta(float target, float current)
{
    float diff = target - current;
    while (diff > PI) diff -= 2.0f * PI;
    while (diff < -PI) diff += 2.0f * PI;
    return diff;
}

void Logo_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;

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

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    UINT32 white = 0xFFFFFFFFu;
    D3D11_SUBRESOURCE_DATA srd = {};
    srd.pSysMem = &white;
    srd.SysMemPitch = 4;

    ID3D11Texture2D* solidTex = nullptr;
    if (SUCCEEDED(pDevice->CreateTexture2D(&td, &srd, &solidTex)) && solidTex)
    {
        pDevice->CreateShaderResourceView(solidTex, nullptr, &g_SolidTex);
        solidTex->Release();
    }

    alpha[0] = 1.0f;
    alpha[1] = 0.0f;
    alpha[2] = 0.0f;

    g_ghostAngle = 0.0f;
    g_ghostTargetAngle = 0.0f;
    g_ghostState = GHOST_IDLE;
    g_ghostFacingLeft = false;
    g_ghostScale = 1.0f;
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

    const float delta = 1.0f / 60.0f;
    timer += delta;
    imageTimer += delta;

    if (!g_positionsInitialized)
    {
        const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
        const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();
        XMFLOAT2 center = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

        const float leftMargin = 20.0f;
        g_yakataPos.x = (g_imageSize.x / 2.0f) + leftMargin;
        g_yakataPos.y = center.y;

        g_ghostPos.x = g_yakataPos.x + (g_imageSize.x * 0.35f);
        g_ghostPos.y = g_yakataPos.y - 40.0f;

        g_basutaPos.x = SCREEN_WIDTH + (g_imageSize.x / 2.0f) + 100.0f;
        g_basutaPos.y = center.y + 30.0f;

        g_basutaTarget.x = g_yakataPos.x + (g_imageSize.x * 0.25f);
        g_basutaTarget.y = g_yakataPos.y + 20.0f;

        g_positionsInitialized = true;
        g_basutaMoving = false;
    }

    const float ghostStart = 0.5f;
    const float basutaStart = 2.0f;
    const float fadeDuration = 0.8f;

    if (timer > ghostStart)
    {
        alpha[1] += delta / fadeDuration;
        if (alpha[1] > 1.0f) alpha[1] = 1.0f;
    }

    if (timer > basutaStart)
    {
        alpha[2] += delta / fadeDuration;
        if (alpha[2] > 1.0f) alpha[2] = 1.0f;
        g_basutaMoving = true;
    }

    // basuta の移動処理
    if (g_basutaMoving)
    {
        const float moveSpeed = 220.0f;
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

    // 幽霊の状態管理
    {
        const float triggerDist = 320.0f;  // 検知距離（より遠くから反応）
        const float fleeDist = 200.0f;     // 逃げ始める距離

        float dx = g_basutaPos.x - g_ghostPos.x;
        float dy = g_basutaPos.y - g_ghostPos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (g_ghostState == GHOST_IDLE)
        {
            // basutaが近づいてきたら警戒開始
            if (dist <= triggerDist && alpha[2] > 0.3f)
            {
                g_ghostState = GHOST_ALERT;
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
                // basutaの方を向く（手前で反転）
                g_ghostFacingLeft = (g_basutaPos.x < g_ghostPos.x);
            }
        }
        else if (g_ghostState == GHOST_ALERT)
        {
            // さらに近づいてきたら館に逃げる
            if (dist <= fleeDist)
            {
                g_ghostState = GHOST_MOVE_TO_HOUSE;
                // 館の方向を向くように反転を更新
                float tx = g_yakataPos.x - (g_imageSize.x * 0.15f);
                g_ghostFacingLeft = (tx < g_ghostPos.x);
            }
            else
            {
                // まだ距離があるので警戒しながらbasutaを見続ける
                const float rotSpeed = 3.5f;
                float deltaAngle = AngleDelta(g_ghostTargetAngle, g_ghostAngle);
                float maxStep = rotSpeed * delta;
                if (fabsf(deltaAngle) <= maxStep)
                {
                    g_ghostAngle = g_ghostTargetAngle;
                }
                else
                {
                    g_ghostAngle += (deltaAngle > 0.0f ? 1.0f : -1.0f) * maxStep;
                }
                // basutaの位置を追跡
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
            }
        }
        else if (g_ghostState == GHOST_MOVE_TO_HOUSE)
        {
            const float ghostMoveSpeed = 180.0f; // 逃げる時は速めに
            float tx = g_yakataPos.x - (g_imageSize.x * 0.15f); // 館の奥に入る
            float ty = g_yakataPos.y;
            float dxh = tx - g_ghostPos.x;
            float dyh = ty - g_ghostPos.y;
            float distH = sqrtf(dxh * dxh + dyh * dyh);

            // 館に近づくにつれて縮小（遠近感）
            const float startDist = 200.0f; // 縮小開始距離
            if (distH < startDist)
            {
                // 距離に応じて 1.0 → 0.3 まで縮小
                g_ghostScale = 0.3f + (distH / startDist) * 0.7f;
            }
            else
            {
                g_ghostScale = 1.0f;
            }

            if (distH > 2.0f)
            {
                float nx = dxh / distH;
                float ny = dyh / distH;
                float step = ghostMoveSpeed * delta;
                if (step >= distH)
                {
                    g_ghostPos.x = tx;
                    g_ghostPos.y = ty;
                    g_ghostState = GHOST_IDLE;
                    // 館の中に入ったので完全に消す
                    alpha[1] = 0.0f;
                }
                else
                {
                    g_ghostPos.x += nx * step;
                    g_ghostPos.y += ny * step;
                    float targetMoveAngle = atan2f(ny, nx);
                    float moveDeltaAngle = AngleDelta(targetMoveAngle, g_ghostAngle);
                    float moveRotSpeed = 2.5f;
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
                g_ghostState = GHOST_IDLE;
                // 館の中に入ったので完全に消す
                alpha[1] = 0.0f;
            }
        }
    }

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

    // 幽霊のふわふわ
    if (g_ghostState == GHOST_MOVE_TO_HOUSE)
    {
        g_ghostOffset.y = sinf(timer * 2.0f) * 6.0f;
        g_ghostOffset.x = sinf(timer * 0.7f) * 3.0f;
        g_ghostBobRotation = sinf(timer * 1.2f) * 0.03f;
    }
    else
    {
        g_ghostOffset.y = sinf(timer * 2.5f) * 12.0f;
        g_ghostOffset.x = sinf(timer * 0.9f) * 6.0f;
        g_ghostBobRotation = sinf(timer * 1.2f) * 0.06f;
    }

    // basuta のふわふわ（歩いているような揺れ）
    if (g_basutaMoving)
    {
        // 歩行中は上下に揺らす（より速いリズム）
        g_basutaOffset.y = sinf(timer * 5.0f) * 8.0f;
        g_basutaOffset.x = sinf(timer * 10.0f) * 3.0f; // 左右の微妙な揺れ
    }
    else
    {
        // 停止中は控えめな揺れ
        g_basutaOffset.y = sinf(timer * 2.0f) * 4.0f;
        g_basutaOffset.x = sinf(timer * 1.5f) * 2.0f;
    }

    if (Keyboard_IsKeyDownTrigger(KK_E))
    {
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

    // 背景を紫で塗る
    if (g_SolidTex)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_SolidTex);
        XMFLOAT4 purple = { 0.45f, 0.10f, 0.45f, 1.0f };
        XMFLOAT2 fullSize = { SCREEN_WIDTH, SCREEN_HEIGHT };
        DrawSprite_A(center, fullSize, purple);
    }

    // 1) 屋敷
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[0]);
        XMFLOAT4 col = { 1.0f, 1.0f, 1.0f, 1.0f };
        DrawSpriteEx(g_yakataPos, g_imageSize, col, 0, 1, 1);
    }

    // 2) 幽霊（反転 + スケール変化）
    if (alpha[1] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[1]);
        XMFLOAT4 ghostCol = { 1.0f, 1.0f, 1.0f, alpha[1] };

        XMFLOAT2 drawPos = { g_ghostPos.x + g_ghostOffset.x, g_ghostPos.y + g_ghostOffset.y };

        // スケールを適用したサイズ
        XMFLOAT2 scaledSize = { g_imageSize.x * g_ghostScale, g_imageSize.y * g_ghostScale };

        // 反転機能付きで描画
        DrawSpriteExFlip(drawPos, scaledSize, ghostCol, 0, 1, 1, g_ghostFacingLeft, false);
    }

    // 3) basuta（ふわふわ追加）
    if (alpha[2] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[2]);
        XMFLOAT4 col2 = { 1.0f, 1.0f, 1.0f, alpha[2] };
        XMFLOAT2 drawPos = { g_basutaPos.x + g_basutaOffset.x, g_basutaPos.y + g_basutaOffset.y };
        DrawSpriteEx(drawPos, g_imageSize, col2, 0, 1, 1);
    }
}