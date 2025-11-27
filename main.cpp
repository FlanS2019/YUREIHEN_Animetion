//main.cpp

/*
   main.cpp

   板倉大和

   2025/5/09
*/

// ウィンドウの表示
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "debug_ostream.h"
#include <algorithm>


#include "direct3d.h"
#include "shader.h"
#include "sprite.h"
#include "keyboard.h"
#include "SceneManager.h"

//==================================================
// マクロ定義
//==================================================
#define CLASS_NAME      "DX21 Window"
#define WINDOW_CAPTION  "AT12D187_05_板倉大和"

#define SCREEN_HEIGHT (720)
#define SCREEN_WIDTH (1280)

//==================================================
// グローバル変数
//==================================================
#ifdef _DEBUG //デブバッグのときのみ使用される
int g_CountFPS;
char g_DebugStr[2048];

#endif
#pragma comment(lib,"winmm.lib")

//==================================================
// プロトタイプ宣言
//==================================================

// ウィンドウプロシージャ
// コールバック関数 => 他人が呼び出してくれる関数
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance, LPSTR lpCmd, int nCmdShow)
{
    //FreamRate計測
    DWORD dwExecLastTime;
    DWORD dwFPSLastTime;
    DWORD dwCurrentTime;
    DWORD dwFrameCount;

    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

    // ウィンドウクラスの登録（ウィンドウの仕様的な物を決めてWindowsへセットする）
    WNDCLASS wc; // 構造体を準備
    ZeroMemory(&wc, sizeof(WNDCLASS)); // 内容を0で初期化
    wc.lpfnWndProc = WndProc;     // コールバック関数のポインタ
    wc.lpszClassName = CLASS_NAME;  // この仕様の名前
    wc.hInstance = hInstance;   // このアプリケーションのこと
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); // カーソルの種類
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1); // ウィンドウの背景色
    RegisterClass(&wc); // 構造体をWindowsへセット


    // ウィンドウサイズの調整
    //クライアント領域（描画領域）のサイズを表す短径
    RECT window_rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    //ウィンドウスタイルの設定
    DWORD window_style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);
    //指定のクライアント領域+ウィンドウスタイルでの全体のサイズを計算
    AdjustWindowRect(&window_rect, window_style, FALSE);
    //短形の横と縦のサイズを計算
    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;

    // ウィンドウの作成
    HWND hWnd = CreateWindowA(
        CLASS_NAME,           // 作りたいウィンドウ
        WINDOW_CAPTION,  // ウィンドウに表示するタイトル
        window_style,
        CW_USEDEFAULT,        // デフォルト設定でおまかせ
        CW_USEDEFAULT,
        window_width,
        window_height,
        NULL,
        NULL,
        hInstance,            // アプリケーションのハンドル
        NULL
    );

    // 作成したウィンドウを表示する
    ShowWindow(hWnd, nCmdShow); // 引数に従って表示、または非表示

    //ウィンドウ内部の更新要求
    UpdateWindow(hWnd);

    Direct3D_Initialize(hWnd);// Direct3Dの初期化
    Keyboard_Initialize(); // キーボードの初期化

	Shader_Initialize(Direct3D_GetDevice(),
		Direct3D_GetDeviceContext());//シェーダー初期化

    InitializeSprite();//これより下なし

    SceneManager_Initialize();
    // メッセージループ
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG)); // メッセージ構造体を作成して初期化

    //hal::dout << "start\n" << std::endl;

    //フレームレート計測初期化
    timeBeginPeriod(1); //timerの制度を設定

    dwExecLastTime = dwFPSLastTime = timeGetTime();
    dwCurrentTime = dwFrameCount = 0;


    // 終了メッセージが来るまでループする
    do
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);  // WndProcが呼び出される
        }
        else
        {//game処理
            dwCurrentTime = timeGetTime();
            if ((dwCurrentTime - dwFPSLastTime) >= 1000)
            {
#ifdef _DEBUG
                g_CountFPS = dwFrameCount;
#endif 
                dwFPSLastTime = dwCurrentTime;
                dwFrameCount = 0;
            }

            if ((dwCurrentTime - dwExecLastTime) >= (float)1000 / 60)
            {
                dwExecLastTime = dwCurrentTime;
#ifdef _DEBUG
                wsprintf(g_DebugStr, "DX21 Project");
                wsprintf(&g_DebugStr[strlen(g_DebugStr)],
                    "FPS : %d", g_CountFPS);
                SetWindowText(hWnd, g_DebugStr);
#endif
                //更新処理
				//_Updateとかの
				SceneManager_Update();
                

                //描画処理
                //_Drawとかの
                Direct3D_Clear();
				SceneManager_Draw();
                Direct3D_Present();

                keycopy(); //キーボード状態のコピー

                dwFrameCount++; //処理回数更新
            }
        }

    } while (msg.message != WM_QUIT);

	SceneManager_Finalize();
    Shader_Finalize();
    FinalizeSprite();

    //hal::dout << "終了\n";
    // 終了する


    Direct3D_Finalize();  //direct3d 終了処理

    return (int)msg.wParam;

}

//==================================================
// ウィンドウプロシージャ
// メッセージループ中で呼び出される
//==================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HGDIOBJ hbrWhite, hbrGray;

    HDC hdc;
    PAINTSTRUCT ps;

    switch (uMsg)
    {
    case WM_ACTIVATEAPP:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        Keyboard_ProcessMessage(uMsg, wParam, lParam);
        break;

    case WM_KEYDOWN: // キーが押された
        Keyboard_ProcessMessage(uMsg, wParam, lParam);
        if (wParam == VK_ESCAPE) // 押されたのはESCキー
        {
            // ウィンドウを閉じたいリクエストをWindowsに送る
            SendMessage(hWnd, WM_CLOSE, 0, 0);
        }
        break;

    case WM_CLOSE: // ウィンドウを閉じたい命令
        if (
            MessageBox(hWnd, "本当に終了してよろしいですか？",
                "確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK
            )
        { // OKが押されたとき
            DestroyWindow(hWnd); // 終了する手続きをWindowsへリクエスト
        }
        else
        {
            return 0; // やっぱり終わらない
        }
        break;

    case WM_DESTROY:  // 終了してOKですよ
        PostQuitMessage(0); // 自分のメッセージに0を送る
        break;
    }
    // 必用の無いメッセージは適当に処理させて終了
    return DefWindowProc(hWnd, uMsg, wParam, lParam);

}