// Logo.cpp
#include "Fade.h"
#include "Logo.h"
#include "shader.h"
#include "Sprite.h"
#include "Keyboard.h"
#include "SceneManager.h"
#include <DirectXMath.h>
#include <cmath>
#include <d3d11.h>

using namespace DirectX;

static ID3D11ShaderResourceView* g_Texture[4];
static ID3D11ShaderResourceView* g_SolidTex = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static bool fadeStarted = false;
static float alpha[3] = { 1.0f, 0.0f, 0.0f };

// 幽霊アニメーション
static XMFLOAT2 g_ghostOffset = { 0.0f, 0.0f };
static float g_ghostBobRotation = 0.0f;
static float g_ghostAngle = 0.0f;
static float g_ghostTargetAngle = 0.0f;
static bool g_ghostFacingLeft = false;
static bool g_prevGhostFacingLeft = false;
static float g_ghostScale = 1.0f;
static bool g_forceFacingByTimer = true;

// 右振り向きでの縮小アニメ
static bool g_shrinkTriggered = false;
static bool g_shrinkAppliedOnce = false;
static float g_shrinkTimer = 0.0f;
static const float g_shrinkDuration = 0.45f;
static const float g_shrinkTargetScale = 0.45f;

// basuta（移動するオブジェクト）
static XMFLOAT2 g_basutaOffset = { 0.0f, 0.0f };

// bikkuri（1 回だけ表示）
static bool g_bikkuriShown = false;
static float g_bikkuriTimer = 0.0f;
static const float g_bikkuriDuration = 0.9f;
static bool g_bikkuriFlip = false;
static const float g_bikkuriLeadTime = 0.35f;
static bool g_bikkuriShownOnce = false;

// 先読み設定・状態
static const float g_ghostLeadSeconds = 7.0f;
static const float g_basutaSpeed = 220.0f;
enum GhostState { GHOST_IDLE = 0, GHOST_ALERT, GHOST_MOVE_TO_HOUSE };
static GhostState g_ghostState = GHOST_IDLE;

static const XMFLOAT2 g_imageSize = { 500.0f, 500.0f };

// 位置
static XMFLOAT2 g_yakataPos = { 0.0f, 0.0f };
static XMFLOAT2 g_ghostPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaTarget = { 0.0f, 0.0f };
static bool g_positionsInitialized = false;
static bool g_basutaMoving = false;
static bool g_basutaStartFromRight = false;
static bool g_basutaEnteredScreen = false;

static const float PI = 3.14159265358979323846f;

static float AngleDelta(float target, float current)
{
    float diff = target - current;
    while (diff > PI) diff -= 2.0f * PI;
    while (diff < -PI) diff += 2.0f * PI;
    return diff;
}

static float EaseOutCubic(float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

void Logo_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;

    g_Texture[0] = Sprite_LoadTexture(pDevice, L"asset\\yureihen\\yakata_jimen1.png"); assert(g_Texture[0]);
    g_Texture[1] = Sprite_LoadTexture(pDevice, L"asset\\yureihen\\yurei1.png"); assert(g_Texture[1]);
    g_Texture[2] = Sprite_LoadTexture(pDevice, L"asset\\yureihen\\basuta1.png"); assert(g_Texture[2]);
    g_Texture[3] = Sprite_LoadTexture(pDevice, L"asset\\bikkuri.png"); assert(g_Texture[3]);

    // 1x1 ソリッドテクスチャ作成
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1; td.Height = 1; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    UINT32 white = 0xFFFFFFFFu;
    D3D11_SUBRESOURCE_DATA srd = {}; srd.pSysMem = &white; srd.SysMemPitch = 4;
    ID3D11Texture2D* solidTex = nullptr;
    if (SUCCEEDED(pDevice->CreateTexture2D(&td, &srd, &solidTex)) && solidTex)
    {
        pDevice->CreateShaderResourceView(solidTex, nullptr, &g_SolidTex);
        solidTex->Release();
    }

    alpha[0] = 1.0f; alpha[1] = 0.0f; alpha[2] = 0.0f;
    g_ghostAngle = g_ghostTargetAngle = 0.0f;
    g_ghostState = GHOST_IDLE;
    g_ghostFacingLeft = false;
    g_prevGhostFacingLeft = g_ghostFacingLeft;
    g_ghostScale = 1.0f;

    g_shrinkTriggered = false; g_shrinkAppliedOnce = false; g_shrinkTimer = 0.0f;

    g_bikkuriShown = false; g_bikkuriTimer = 5.5f; g_bikkuriFlip = false; g_bikkuriShownOnce = false;

    g_basutaStartFromRight = false; g_basutaEnteredScreen = false;
    g_forceFacingByTimer = true;
}

void Logo_Finalize()
{
    for (int i = 0; i < 4; ++i)
    {
        if (g_Texture[i]) { g_Texture[i]->Release(); g_Texture[i] = nullptr; }
    }
    if (g_SolidTex) { g_SolidTex->Release(); g_SolidTex = nullptr; }
}

void Logo_Update()
{
    static float timer = 0.0f;
    static bool waitStarted = false;
    static float waitTimer = 0.0f;

    const float delta = 1.0f / 60.0f;
    timer += delta;

    // 強制向き制御（シーケンス制御）
    if (g_forceFacingByTimer)
    {
        if (timer < 3.0f) g_ghostFacingLeft = false;
        else if (timer < 4.0f) g_ghostFacingLeft = true;
        else g_ghostFacingLeft = false;
    }

    // 右を向いた瞬間で縮小アニメを一度だけ開始
    if (!g_shrinkTriggered && !g_shrinkAppliedOnce &&
        g_ghostFacingLeft != g_prevGhostFacingLeft && g_ghostFacingLeft == false)
    {
        g_shrinkTriggered = true; g_shrinkTimer = 0.0f;
    }

    // 向き変化で bikkuri を一度表示（表示確定は Draw 内で g_bikkuriShownOnce をセット）
    if (!g_bikkuriShownOnce && g_ghostFacingLeft != g_prevGhostFacingLeft)
    {
        if (!g_bikkuriShown)
        {
            g_bikkuriShown = true;
            g_bikkuriTimer = 0.0f;
            g_bikkuriFlip = g_ghostFacingLeft;
        }
        g_prevGhostFacingLeft = g_ghostFacingLeft;
    }

    const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
    if (!g_positionsInitialized)
    {
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
        g_positionsInitialized = true; g_basutaMoving = false;
        g_basutaStartFromRight = (g_basutaPos.x > SCREEN_WIDTH);
        g_basutaEnteredScreen = false;
    }

    const float ghostStart = 1.5f;
    const float basutaStart = 0.8f;
    const float fadeDuration = 2.4f;

    if (timer > ghostStart)
    {
        alpha[1] += delta / fadeDuration;
        if (alpha[1] > 1.0f) alpha[1] = 1.0f;
    }

    // basuta 到来前リードで bikkuri 表示
    if (!g_bikkuriShownOnce && !g_bikkuriShown &&
        timer > (basutaStart - g_bikkuriLeadTime) && timer <= basutaStart)
    {
        g_bikkuriFlip = g_basutaStartFromRight;
        g_bikkuriShown = true; g_bikkuriTimer = 0.0f;
    }

    if (timer > basutaStart)
    {
        alpha[2] += delta / fadeDuration;
        if (alpha[2] > 1.0f) alpha[2] = 1.0f;

        if (!g_basutaMoving && !g_bikkuriShown && !g_bikkuriShownOnce)
        {
            g_bikkuriFlip = (g_basutaTarget.x < g_basutaPos.x);
            g_bikkuriShown = true; g_bikkuriTimer = 0.0f;
        }
        g_basutaMoving = true;
    }

    // basuta 移動
    if (g_basutaMoving)
    {
        const float moveSpeed = g_basutaSpeed;
        float dx = g_basutaTarget.x - g_basutaPos.x;
        float dy = g_basutaTarget.y - g_basutaPos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 1.0f)
        {
            float nx = dx / dist, ny = dy / dist;
            float step = moveSpeed * delta;
            if (step >= dist) { g_basutaPos = g_basutaTarget; g_basutaMoving = false; }
            else { g_basutaPos.x += nx * step; g_basutaPos.y += ny * step; }
        }
        else { g_basutaPos = g_basutaTarget; g_basutaMoving = false; }

        // 画面入場判定で幽霊のアラート向き判定
        if (!g_basutaEnteredScreen)
        {
            const float enterThresholdRight = SCREEN_WIDTH - (g_imageSize.x * 0.5f);
            const float enterThresholdLeft = (g_imageSize.x * 0.5f);
            if (g_basutaStartFromRight)
            {
                if (g_basutaPos.x <= enterThresholdRight)
                {
                    g_basutaEnteredScreen = true;
                    if (!g_forceFacingByTimer) g_ghostFacingLeft = false;
                    if (g_ghostState == GHOST_IDLE)
                    {
                        g_ghostState = GHOST_ALERT;
                        g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
                    }
                }
            }
            else
            {
                if (g_basutaPos.x >= enterThresholdLeft)
                {
                    g_basutaEnteredScreen = true;
                    if (!g_forceFacingByTimer) g_ghostFacingLeft = true;
                    if (g_ghostState == GHOST_IDLE)
                    {
                        g_ghostState = GHOST_ALERT;
                        g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
                    }
                }
            }
        }
    }

    // bikkuri タイマー
    if (g_bikkuriShown)
    {
        g_bikkuriTimer += 1.0f / 60.0f;
        if (g_bikkuriTimer >= g_bikkuriDuration)
        {
            g_bikkuriShown = false;
            g_bikkuriTimer = 0.0f;
            // g_bikkuriShownOnce は LogoDraw で描画後に立てる
        }
    }

    // 幽霊状態管理
    {
        const float baseTriggerDist = 320.0f;
        const float triggerDist = baseTriggerDist + (g_basutaSpeed * g_ghostLeadSeconds);
        const float fleeDist = 200.0f;
        float dx = g_basutaPos.x - g_ghostPos.x, dy = g_basutaPos.y - g_ghostPos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (g_ghostState == GHOST_IDLE)
        {
            if (dist <= triggerDist && alpha[2] > 0.3f)
            {
                g_ghostState = GHOST_ALERT;
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
                if (!g_basutaEnteredScreen && !g_forceFacingByTimer)
                {
                    g_ghostFacingLeft = (g_basutaPos.x < g_ghostPos.x);
                }
            }
        }
        else if (g_ghostState == GHOST_ALERT)
        {
            if (dist <= fleeDist)
            {
                g_ghostState = GHOST_MOVE_TO_HOUSE;
                float tx = g_yakataPos.x - (g_imageSize.x * 0.15f);
                if (!g_forceFacingByTimer) g_ghostFacingLeft = (tx < g_ghostPos.x);
            }
            else
            {
                const float rotSpeed = 3.5f;
                float deltaAngle = AngleDelta(g_ghostTargetAngle, g_ghostAngle);
                float maxStep = rotSpeed * delta;
                if (fabsf(deltaAngle) <= maxStep) g_ghostAngle = g_ghostTargetAngle;
                else g_ghostAngle += (deltaAngle > 0.0f ? 1.0f : -1.0f) * maxStep;
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
            }
        }
        else if (g_ghostState == GHOST_MOVE_TO_HOUSE)
        {
            const float ghostMoveSpeed = 180.0f;
            float tx = g_yakataPos.x - (g_imageSize.x * 0.15f);
            float ty = g_yakataPos.y;
            float dxh = tx - g_ghostPos.x, dyh = ty - g_ghostPos.y;
            float distH = sqrtf(dxh * dxh + dyh * dyh);
            const float startDist = 200.0f;

            if (!g_shrinkAppliedOnce)
            {
                if (distH < startDist) g_ghostScale = 0.3f + (distH / startDist) * 0.7f;
                else g_ghostScale = 1.0f;
            }
            else
            {
                g_ghostScale = g_shrinkTargetScale;
            }

            if (distH > 2.0f)
            {
                float nx = dxh / distH, ny = dyh / distH;
                float step = ghostMoveSpeed * delta;
                if (step >= distH) { g_ghostPos.x = tx; g_ghostPos.y = ty; g_ghostState = GHOST_IDLE; alpha[1] = 0.0f; }
                else
                {
                    g_ghostPos.x += nx * step; g_ghostPos.y += ny * step;
                    float targetMoveAngle = atan2f(ny, nx);
                    float moveDeltaAngle = AngleDelta(targetMoveAngle, g_ghostAngle);
                    float moveRotSpeed = 2.5f;
                    float moveMaxStep = moveRotSpeed * delta;
                    if (fabsf(moveDeltaAngle) <= moveMaxStep) g_ghostAngle = targetMoveAngle;
                    else g_ghostAngle += (moveDeltaAngle > 0.0f ? 1.0f : -1.0f) * moveMaxStep;
                }
            }
            else { g_ghostState = GHOST_IDLE; alpha[1] = 0.0f; }
        }
    }

    // 右向き縮小アニメ更新（移動縮小と排他）
    if (g_shrinkTriggered && !g_shrinkAppliedOnce && g_ghostState != GHOST_MOVE_TO_HOUSE)
    {
        g_shrinkTimer += delta;
        float t = g_shrinkTimer / g_shrinkDuration; if (t > 1.0f) t = 1.0f;
        float e = EaseOutCubic(t);
        g_ghostScale = 1.0f + (g_shrinkTargetScale - 1.0f) * e;
        if (t >= 1.0f) { g_shrinkTriggered = false; g_shrinkAppliedOnce = true; g_ghostScale = g_shrinkTargetScale; }
    }

    if (!fadeStarted && timer > 7.0f && GetFadeState() == FADE_NONE)
    {
        fadeStarted = true;
        XMFLOAT4 color = { 0,0,0,1 };
        SetFade(90, color, FADE_OUT, SCENE_TITLE);
    }

    if (fadeStarted && !waitStarted && GetFadeState() == FADE_NONE) { waitStarted = true; waitTimer = 0.0f; }
    if (waitStarted) { waitTimer += delta; if (waitTimer >= 1.5f) SetScene(SCENE_TITLE); }

    // ふわふわ
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

    if (g_basutaMoving)
    {
        g_basutaOffset.y = sinf(timer * 5.0f) * 8.0f;
        g_basutaOffset.x = sinf(timer * 10.0f) * 3.0f;
    }
    else
    {
        g_basutaOffset.y = sinf(timer * 2.0f) * 4.0f;
        g_basutaOffset.x = sinf(timer * 1.5f) * 2.0f;
    }

    if (Keyboard_IsKeyDownTrigger(KK_E)) SetScene(SCENE_TITLE);

    g_prevGhostFacingLeft = g_ghostFacingLeft;
}

void LogoDraw(void)
{
    const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
    const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

    Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
    SetBlendState(BLENDSTATE_ALFA);

    XMFLOAT2 center = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

    if (g_SolidTex)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_SolidTex);
        XMFLOAT4 purple = { 0.45f, 0.10f, 0.45f, 1.0f };
        XMFLOAT2 fullSize = { SCREEN_WIDTH, SCREEN_HEIGHT };
        DrawSprite_A(center, fullSize, purple);
    }

    // 屋敷
    g_pContext->PSSetShaderResources(0, 1, &g_Texture[0]);
    DrawSpriteEx(g_yakataPos, g_imageSize, XMFLOAT4{ 1,1,1,1 }, 0, 1, 1);

    // 幽霊
    if (alpha[1] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[1]);
        XMFLOAT4 ghostCol = { 1.0f,1.0f,1.0f,alpha[1] };
        XMFLOAT2 drawPos = { g_ghostPos.x + g_ghostOffset.x, g_ghostPos.y + g_ghostOffset.y };
        XMFLOAT2 scaledSize = { g_imageSize.x * g_ghostScale, g_imageSize.y * g_ghostScale };
        bool shouldFlip = g_ghostFacingLeft;
        DrawSpriteExFlip(drawPos, scaledSize, ghostCol, 0, 1, 1, shouldFlip, false);
    }

    // basuta
    if (alpha[2] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[2]);
        XMFLOAT4 col2 = { 1,1,1,alpha[2] };
        XMFLOAT2 drawPos = { g_basutaPos.x + g_basutaOffset.x, g_basutaPos.y + g_basutaOffset.y };
        DrawSpriteEx(drawPos, g_imageSize, col2, 0, 1, 1);
    }

    // bikkuri（描画確定後に一度だけフラグを立てる）
    if (g_bikkuriShown && g_Texture[3])
    {
        XMFLOAT2 bsize = { 180.0f, 180.0f };
        XMFLOAT2 bpos;
        bpos.x = g_ghostPos.x + g_ghostOffset.x;
        bpos.y = g_ghostPos.y + g_ghostOffset.y + (g_imageSize.y * 0.45f);

        float halfH = bsize.y * 0.5f;
        const float margin = 8.0f;
        if (bpos.y + halfH + margin > SCREEN_HEIGHT) bpos.y = SCREEN_HEIGHT - halfH - margin;
        if (bpos.y - halfH - margin < 0.0f) bpos.y = halfH + margin;

        if (g_SolidTex)
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SolidTex);
            XMFLOAT4 dbgBg = { 0.0f,0.0f,0.0f,0.45f };
            XMFLOAT2 bgSize = { bsize.x + 20.0f, bsize.y + 20.0f };
            DrawSpriteEx({ bpos.x, bpos.y }, bgSize, dbgBg, 0, 1, 1);
        }

        g_pContext->PSSetShaderResources(0, 1, &g_Texture[3]);
        DrawSpriteExFlip(bpos, bsize, XMFLOAT4{ 1,1,1,1 }, 0, 1, 1, g_bikkuriFlip, false);

        if (!g_bikkuriShownOnce) g_bikkuriShownOnce = true;
    }
}