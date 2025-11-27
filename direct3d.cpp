#include <d3d11.h>
#include "direct3d.h"
#include "debug_ostream.h"

#pragma comment(lib, "d3d11.lib")

//各種インターフェイス
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;

//バックバッファ関連

static ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
static ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;
static ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
static D3D11_TEXTURE2D_DESC g_BackBufferDesc{};
static D3D11_VIEWPORT g_viewport{};

static bool configureBackBuffer();
static void releaseBackBuffer();

//グローバル変数追加
float bFactor[4] = { 0,0,0,0 };
ID3D11BlendState* bState[BLENDSTATE_MAX];
ID3D11DepthStencilState* g_DepthStateEnable;
ID3D11DepthStencilState* g_DepthStateDisable;


bool Direct3D_Initialize(HWND hWnd)
{
	////ブレンドステート設定
	//D3D11_BLEND_DESC blendDesc;

	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
	swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.BufferCount = 2;


	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swap_chain_desc.OutputWindow = hWnd;


	UINT device_flags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		device_flags,
		levels,
		ARRAYSIZE(levels),
		D3D11_SDK_VERSION,
		&swap_chain_desc,
		&g_pSwapChain,
		&g_pDevice,
		&feature_level,
		&g_pDeviceContext);

	if (FAILED(hr)) {
		MessageBox(hWnd, "Direct3Dの初期化に失敗しました。", "ERROR", MB_OK);
	}

	if (!configureBackBuffer())
	{
		MessageBox(hWnd, "バックバッファの設定に失敗しました", "ERROR", MB_OK);
		return false;
	}

	//サンプラーステートせって
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MaxLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	ID3D11SamplerState* samplerState = NULL;

	g_pDevice->CreateSamplerState(&samplerDesc, &samplerState);
	//サンプラーをシェーダーへセット
	g_pDeviceContext->PSSetSamplers(0, 1, &samplerState);

	//ブレンドステートせてい
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//ブレンド無効
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_NONE]);

	//αブレンド
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_ALFA]);

	//加算合成
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_ADD]);

	//減産合成
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_SUBTRACT;
	g_pDevice->CreateBlendState(&blendDesc, &bState[BLENDSTATE_SUB]);

	SetBlendState(BLENDSTATE_ALFA);

	//深度ステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	g_pDevice->CreateDepthStencilState(&depthStencilDesc, &g_DepthStateEnable);
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	g_pDevice->CreateDepthStencilState(&depthStencilDesc, &g_DepthStateDisable);

	g_pDeviceContext->OMSetDepthStencilState(g_DepthStateDisable, NULL);


	return true;
}

void Direct3D_Finalize()
{
	releaseBackBuffer();


	if (g_pSwapChain) {
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}

	if (g_pDeviceContext) {
		g_pDeviceContext->Release();
		//g_pDeviceContext = nullptr;
		g_pDevice = nullptr;
	}

}

void Direct3D_Clear()
{  //バックバッファのCLEAR
	float clear_color[4] = { 0.2f,0.4f,0.8f,1.0f };

	g_pDeviceContext->ClearRenderTargetView
	(g_pRenderTargetView, clear_color);

	g_pDeviceContext->ClearDepthStencilView
	(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pDeviceContext->OMSetRenderTargets
	(1, &g_pRenderTargetView, g_pDepthStencilView);
}

void Direct3D_Present()
{
	// スワップチェーンの表示

	g_pSwapChain->Present(2, 0);
}

void SetBlendState(BLENDSTATE blend)
{
	g_pDeviceContext->OMSetBlendState(bState[blend], bFactor, 0xffffffff);
}

ID3D11Device* Direct3D_GetDevice()
{
	return g_pDevice;
}

ID3D11DeviceContext* Direct3D_GetDeviceContext()
{
	return g_pDeviceContext;
}

unsigned int Direct3D_GetBackBufferWidth()
{
	return g_BackBufferDesc.Width;
}

unsigned int Direct3D_GetBackBufferHeight()
{
	return g_BackBufferDesc.Height;
}

bool configureBackBuffer()
{
	HRESULT hr;

	ID3D11Texture2D* back_buffer_pointer = nullptr;

	//　バックバッファの取得

	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer_pointer);

	if (FAILED(hr)) {
		hal::dout << "バックバッファの取得に失敗しました" << std::endl;
		return false;
	}

	// RenderTargetView の作成処理を追加
	hr = g_pDevice->CreateRenderTargetView(back_buffer_pointer, nullptr, &g_pRenderTargetView);
	if (FAILED(hr)) {
		hal::dout << "レンダーターゲットビューの作成に失敗しました" << std::endl;
		back_buffer_pointer->Release();
		return false;
	}
	//　バックバッファの状態
	back_buffer_pointer->GetDesc(&g_BackBufferDesc);

	back_buffer_pointer->Release(); //バックバッファのポインタは不要なので解散

	// デブスステンシルバッファの生成\

	D3D11_TEXTURE2D_DESC depth_stencil_desc{};
	depth_stencil_desc.Width = g_BackBufferDesc.Width;
	depth_stencil_desc.Height = g_BackBufferDesc.Height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_stencil_desc.SampleDesc.Count = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	hr = g_pDevice->CreateTexture2D
	(&depth_stencil_desc, nullptr, &g_pDepthStencilBuffer);

	if (FAILED(hr)) {
		hal::dout << "テンシャルバッファの生成に失敗した" << std::endl;
		return false;
	}

	//デプスス天心ビューの生成

	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
	depth_stencil_view_desc.Format = depth_stencil_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Flags = 0;
	depth_stencil_view_desc.Flags = 0;
	hr = g_pDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &depth_stencil_view_desc,
		&g_pDepthStencilView);

	if (FAILED(hr)) {
		hal::dout << "デプスステンシルビューの生成に失敗した" << std::endl;
	}

	//viewPort	
	g_viewport.TopLeftX = 0.0f;
	g_viewport.TopLeftY = 0.0f;
	g_viewport.Width = static_cast<FLOAT>(g_BackBufferDesc.Width);
	g_viewport.Height = static_cast<FLOAT>(g_BackBufferDesc.Height);
	g_viewport.MinDepth = 0.0f;
	g_viewport.MaxDepth = 1.0f;
	g_pDeviceContext->RSSetViewports(1, &g_viewport);


	return true;
}

void releaseBackBuffer()
{
	if (g_pRenderTargetView)
	{
		g_pRenderTargetView->Release();
		g_pRenderTargetView = nullptr;
	}

	if (g_pDepthStencilBuffer)
	{
		g_pDepthStencilBuffer->Release();
		g_pDepthStencilBuffer = nullptr;
	}
	if (g_pDepthStencilView)
	{
		g_pDepthStencilView->Release();
		g_pDepthStencilView = nullptr;
	}
}

