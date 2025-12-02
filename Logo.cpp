// Logo.cpp
#include "Fade.h"
#include "Logo.h"
#include "shader.h"
#include "Sprite.h"
#include "Keyboard.h"
#include "SceneManager.h"
#include <cmath>
#include <d3d11.h>

static ID3D11ShaderResourceView* g_Texture[5];
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
static bool g_prevGhostFacingLeft = false; // 前フレームの向き（bikkuri 同期用）
static float g_ghostScale = 1.0f;      // 幽霊のスケール（縮小用）
static bool g_forceFacingByTimer = true; // 向き制御をタイマー優先にするフラグ

// basuta アニメーション用
static XMFLOAT2 g_basutaOffset = { 0.0f, 0.0f };

// bikkuri 用（表示タイミング制御）
static bool g_bikkuriShown = false;
static float g_bikkuriTimer = 0.0f;
static const float g_bikkuriDuration = 0.9f; // 表示時間（秒）
static bool g_bikkuriFlip = false; // basuta の来る方向に合わせて反転する
static const float g_bikkuriLeadTime = 0.35f; // basuta 到来より早めに表示する時間（秒） <- 追加
static bool g_bikkuriShownOnce = false; // 一度だけ表示するフラグ

// 先読み（幽霊が早めに反応する）設定
static const float g_ghostLeadSeconds = 7.0f; // ここを変えると何秒早く反応するかを調整できます
static const float g_basutaSpeed = 220.0f;   // basuta の移動速度（移動処理と同期）

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

// 新規：basuta の出現元と画面内に入ったフラグ（幽霊の反転タイミング制御用）
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

void Logo_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;

    // 画像読み込みを共通関数へ置換（重複削減）
    g_Texture[0] = Sprite_LoadTexture(pDevice, L"asset\\yureihen\\yakata_jimen1.png");
    assert(g_Texture[0]);

    g_Texture[1] = Sprite_LoadTexture(pDevice, L"asset\\yureihen\\yurei1.png");
    assert(g_Texture[1]);

    g_Texture[2] = Sprite_LoadTexture(pDevice, L"asset\\yureihen\\basuta1.png");
    assert(g_Texture[2]);

    g_Texture[3] = Sprite_LoadTexture(pDevice, L"asset\\bikkuri.png");
    assert(g_Texture[3]);

    // 以下は変更なし（1x1 テクスチャ生成等）
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
    g_ghostFacingLeft = false; // 元画像は右向きなので初期は右（false）
    g_prevGhostFacingLeft = g_ghostFacingLeft;
    g_ghostScale = 1.0f;

    g_bikkuriShown = false;
    g_bikkuriTimer = 5.5f;
    g_bikkuriFlip = false;
    g_bikkuriShownOnce = false;

    // 新規フラグ初期化
    g_basutaStartFromRight = false;
    g_basutaEnteredScreen = false;

    // タイマー優先で向きを制御する（要求どおり）
    g_forceFacingByTimer = true;
}

void Logo_Finalize()
{
    for (int i = 0; i < 4; i++)
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

    // 強制タイマー制御：最初は右（false）、3秒後に左(true)、さらに1秒後に右(false) に戻す
    if (g_forceFacingByTimer)
    {
        if (timer < 3.0f)
        {
            g_ghostFacingLeft = false; // 右向き（元画像の向き）
        }
        else if (timer < 4.0f)
        {
            g_ghostFacingLeft = true; // 3秒経過後→左向き
        }
        else
        {
            g_ghostFacingLeft = false; // さらに1秒後→右向きに戻す
        }
    }

    // --- ここで向き変化を検知して bikkuri を同時に出す（ただし一度だけ） ---
    if (!g_bikkuriShownOnce && g_ghostFacingLeft != g_prevGhostFacingLeft)
    {
        // 向きが変わった瞬間に bikkuri 表示を開始（1回だけ）
        g_bikkuriShown = true;
        g_bikkuriTimer = 0.0f;
        g_bikkuriFlip = g_ghostFacingLeft;
        // g_bikkuriShownOnce は実際に描画した後で true にする（描画を確実にするため）
        // 前回向きを更新
        g_prevGhostFacingLeft = g_ghostFacingLeft;

        // デバッグ出力
        char dbg[128];
        sprintf_s(dbg, "Bikkuri triggered by facing-change: facingLeft=%d timer=%.2f\n", g_ghostFacingLeft, timer);
        OutputDebugStringA(dbg);
    }
    // ------------------------------------------------------

    // 画面幅を参照する（basuta の画面入場判定で使用）
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

        g_positionsInitialized = true;
        g_basutaMoving = false;

        // 新規：basuta の出現元が右か左かを記録
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

    // ここで早めに bikkuri を表示する（basutaStart より g_bikkuriLeadTime 秒前）
    if (!g_bikkuriShownOnce && !g_bikkuriShown && timer > (basutaStart - g_bikkuriLeadTime) && timer <= basutaStart)
    {
        // basuta のスタート位置情報から反転方向を決定
        g_bikkuriFlip = g_basutaStartFromRight; // 右から来るなら反転（true）
        g_bikkuriShown = true;
        g_bikkuriTimer = 0.0f;

        // デバッグ出力
        char dbg[128];
        sprintf_s(dbg, "Bikkuri triggered by basuta lead: basutaStart=%.2f timer=%.2f flip=%d\n",
            basutaStart, timer, g_bikkuriFlip);
        OutputDebugStringA(dbg);
    }

    if (timer > basutaStart)
    {
        // basuta 表示開始（既存ロジックはそのまま）
        alpha[2] += delta / fadeDuration;
        if (alpha[2] > 1.0f) alpha[2] = 1.0f;

        // basuta が動き始めた瞬間に一度だけ bikkuri を表示する（ただし既に一度表示済みなら飛ばす）
        if (!g_basutaMoving && !g_bikkuriShown && !g_bikkuriShownOnce)
        {
            // basuta は右端から来る想定 → basutaPos.x > basutaTarget.x の場合は左向きに来る
            g_bikkuriFlip = (g_basutaTarget.x < g_basutaPos.x);
            g_bikkuriShown = true;
            g_bikkuriTimer = 0.0f;

            // デバッグ出力
            char dbg[128];
            sprintf_s(dbg, "Bikkuri triggered when basuta starts moving: flip=%d\n", g_bikkuriFlip);
            OutputDebugStringA(dbg);
        }

        g_basutaMoving = true;
    }

    // basuta の移動処理
    if (g_basutaMoving)
    {
        const float moveSpeed = g_basutaSpeed;
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

        // 新規：basuta が「画面に見え始めた」最初のフレームで幽霊を振り向かせる
        if (!g_basutaEnteredScreen)
        {
            const float enterThresholdRight = SCREEN_WIDTH - (g_imageSize.x * 0.5f);
            const float enterThresholdLeft = (g_imageSize.x * 0.5f);

            if (g_basutaStartFromRight)
            {
                if (g_basutaPos.x <= enterThresholdRight)
                {
                    g_basutaEnteredScreen = true;
                    if (!g_forceFacingByTimer) g_ghostFacingLeft = false;  // ★ 右を見る = 反転なし（元の向き）

                    if (g_ghostState == GHOST_IDLE)
                    {
                        g_ghostState = GHOST_ALERT;
                        g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y,
                            g_basutaPos.x - g_ghostPos.x);
                    }
                }
            }
            else
            {
                if (g_basutaPos.x >= enterThresholdLeft)
                {
                    g_basutaEnteredScreen = true;
                    if (!g_forceFacingByTimer) g_ghostFacingLeft = true;  // ★ 左を見る = 反転する

                    if (g_ghostState == GHOST_IDLE)
                    {
                        g_ghostState = GHOST_ALERT;
                        g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y,
                            g_basutaPos.x - g_ghostPos.x);
                    }
                }
            }
        }
    }
    // bikkuri 表示タイマー更新（タイミングのみ制御）
    if (g_bikkuriShown)
    {
        g_bikkuriTimer += 1.0f / 60.0f;
        if (g_bikkuriTimer >= g_bikkuriDuration)
        {
            g_bikkuriShown = false;
            g_bikkuriTimer = 0.0f;
            // g_bikkuriShownOnce は LogoDraw() で描画した直後に true にする
        }
    }

    // 幽霊の状態管理（既存ロジック）
    {
        const float baseTriggerDist = 320.0f;  // 元の検知距離
        const float triggerDist = baseTriggerDist + (g_basutaSpeed * g_ghostLeadSeconds);
        const float fleeDist = 200.0f;     // 逃げ始める距離

        float dx = g_basutaPos.x - g_ghostPos.x;
        float dy = g_basutaPos.y - g_ghostPos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (g_ghostState == GHOST_IDLE)
        {
            if (dist <= triggerDist && alpha[2] > 0.3f)
            {
                g_ghostState = GHOST_ALERT;
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y,
                    g_basutaPos.x - g_ghostPos.x);
                if (!g_basutaEnteredScreen)
                {
                    if (!g_forceFacingByTimer)
                    {
                        g_ghostFacingLeft = (g_basutaPos.x < g_ghostPos.x);
                    }
                }
            }
        }
        else if (g_ghostState == GHOST_ALERT)
        {
            if (dist <= fleeDist)
            {
                g_ghostState = GHOST_MOVE_TO_HOUSE;
                float tx = g_yakataPos.x - (g_imageSize.x * 0.15f);
                if (!g_forceFacingByTimer)
                {
                    g_ghostFacingLeft = (tx < g_ghostPos.x);
                }
            }
            else
            {
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

            const float startDist = 200.0f; // 縮小開始距離
            if (distH < startDist)
            {
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
        g_basutaOffset.y = sinf(timer * 5.0f) * 8.0f;
        g_basutaOffset.x = sinf(timer * 10.0f) * 3.0f;
    }
    else
    {
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

        // 描画の反転は g_ghostFacingLeft を直接使う（元画像は右向き）
        bool shouldFlip = g_ghostFacingLeft;
        DrawSpriteExFlip(drawPos, scaledSize, ghostCol, 0, 1, 1, shouldFlip, false);
    }

    // 3) basuta（ふわふわ追加）
    if (alpha[2] > 0.0f)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[2]);
        XMFLOAT4 col2 = { 1.0f, 1.0f, 1.0f, alpha[2] };
        XMFLOAT2 drawPos = { g_basutaPos.x + g_basutaOffset.x, g_basutaPos.y + g_basutaOffset.y };
        DrawSpriteEx(drawPos, g_imageSize, col2, 0, 1, 1);
    }

    // 4) bikkuri（幽霊の向き変化と同期して表示） — 画面内に収まるよう Y を下げ、1 回のみ表示
    if (g_bikkuriShown && g_Texture[3])
    {
        // bikkuri のサイズと位置を調整（やや小さく、幽霊の下に表示）
        XMFLOAT2 bsize = { 180.0f, 180.0f }; // 少し小さくしてはみ出しを防ぐ
        XMFLOAT2 bpos;
        bpos.x = g_ghostPos.x + g_ghostOffset.x;
        // 以前より下に配置（画面内に収めやすく）
        bpos.y = g_ghostPos.y + g_ghostOffset.y + (g_imageSize.y * 0.45f);

        // 画面外にはみ出さないようにクランプ（中心座標基準）
        float halfH = bsize.y * 0.5f;
        const float margin = 8.0f;
        if (bpos.y + halfH + margin > SCREEN_HEIGHT)
        {
            bpos.y = SCREEN_HEIGHT - halfH - margin;
        }
        if (bpos.y - halfH - margin < 0.0f)
        {
            bpos.y = halfH + margin;
        }

        // 背景のソリッドを軽く描画して見えやすくする（デバッグ用、透過）
        if (g_SolidTex)
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SolidTex);
            XMFLOAT4 dbgBg = { 0.0f, 0.0f, 0.0f, 0.45f }; // 半透明の黒
            XMFLOAT2 bgSize = { bsize.x + 20.0f, bsize.y + 20.0f };
            DrawSpriteEx({ bpos.x, bpos.y }, bgSize, dbgBg, 0, 1, 1);
        }

        // bikkuri 本体を描画（幽霊の向きに合わせて反転）
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[3]);
        XMFLOAT4 bcol = { 1.0f, 1.0f, 1.0f, 1.0f };
        DrawSpriteExFlip(bpos, bsize, bcol, 0, 1, 1, g_bikkuriFlip, false);

        // 描画を確認したら「一度だけ表示」フラグを立てる
        if (!g_bikkuriShownOnce)
        {
            g_bikkuriShownOnce = true;

            // デバッグ出力
            char dbg[128];
            sprintf_s(dbg, "Bikkuri drawn on screen at (%.1f, %.1f), flip=%d\n", bpos.x, bpos.y, g_bikkuriFlip);
            OutputDebugStringA(dbg);
        }
    }
}