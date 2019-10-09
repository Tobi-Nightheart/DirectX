#pragma once

#include "pcH.h"
#include "GameTimer.h"
#include "SceneManager.h"
#include <string>

//Globals
HINSTANCE g_hInst = nullptr;
HWND g_hWnd = nullptr;
D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_1;
Microsoft::WRL::ComPtr<ID3D11Device> g_pDevice = nullptr;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> g_pContext = nullptr;
Microsoft::WRL::ComPtr<IDXGISwapChain> g_pSwapChain = nullptr;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> g_pRTView = nullptr;
Microsoft::WRL::ComPtr<ID3D11DepthStencilView> g_pZBuffer = nullptr;

SceneManager* g_pSceneManager = nullptr;

GameTimer g_GameTimer;

//Input* g_pInput = nullptr; //for input scan comments to implement input

#pragma region FowardDeclarations
HRESULT InitializeWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT InitializeD3D();
HRESULT InitializeGraphics(void);
void Render(void);
void CalculateFPS();
void ReportLiveObjects();
void ShutdownD3D();
#pragma endregion

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#if defined(DEBUG)|defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif


	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	g_GameTimer = GameTimer();

	if (FAILED(InitializeWindow(hInstance, nCmdShow)))
	{
		MessageBox(nullptr, L"Failed to create Window", L"HR FAILED", MB_OK);
		return 0;
	}

	/*g_pInput = new Input();
	if(FAILED(g_pInput->Initialise(hInstance, ghWnd)))
	{
		MessageBox(nullptr, L"Failed to create Input", L"HR FAILED", MB_OK);
		return 0;
	}
	*/

	if (FAILED(InitializeD3D()))
	{
		MessageBox(nullptr, L"Failed to create Device", L"HR FAILED", MB_OK);
		return 0;
	}

	if (FAILED(InitializeGraphics()))
	{
		MessageBox(nullptr, L"Failed to initialize Graphics", L"HR FAILED", MB_OK);
		return 0;
	}

	//Main Loop
	MSG msg = { 0 };
	g_GameTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			CalculateFPS();
			g_GameTimer.Tick();
			Render();
		}
	}
	ReportLiveObjects();
	ShutdownD3D();
	return (int)msg.wParam;
}

HRESULT InitializeWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.lpszClassName = L"Dynamic Snow Shader";

	if (!RegisterClassEx(&wcex)) return E_FAIL;

	g_hInst = hInstance;
	RECT rc = { 0, 0, 800, 450 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, 0);
	g_hWnd = CreateWindow(L"Dynamic Snow Shader", L"D3D11 Framework", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
	if (!g_hWnd)
		return E_FAIL;
	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	
	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	
	case WM_KEYDOWN:
		return 0;

	case WM_SIZE:
		if (g_pSwapChain)
		{
			RECT rc;
			GetClientRect(g_hWnd, &rc);
			UINT width = rc.right - rc.left;
			UINT height = rc.bottom - rc.top;

			g_pContext->OMSetRenderTargets(0, 0, 0);

			//Release all outstanding references to the swap chain's buffers.
			SAFE_RELEASE(g_pRTView);
			SAFE_RELEASE(g_pZBuffer);
			g_pContext->Flush();

			HRESULT hr = S_OK;
			hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

			ID3D11Texture2D* pBuffer;
			hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)& pBuffer);
			if (FAILED(hr))
			{
				MessageBox(nullptr, L"Resize GetBuffer Failed", L"HR FAILED", MB_OK);
				return hr;
			}

			hr = g_pDevice->CreateRenderTargetView(pBuffer, nullptr, &g_pRTView);
			if (FAILED(hr))
			{
				MessageBox(nullptr, L"Resize CreateRTV Failed", L"HR FAILED", MB_OK);
				return hr;
			}

			D3D11_TEXTURE2D_DESC tex2dDesc;
			ZeroMemory(&tex2dDesc, sizeof(D3D11_TEXTURE2D_DESC));

			D3D11_TEXTURE2D_DESC pBufferDesc;
			pBuffer->GetDesc(&pBufferDesc);

			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.ArraySize = 1;
			tex2dDesc.MipLevels = 1;
			tex2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			tex2dDesc.SampleDesc.Count = pBufferDesc.SampleDesc.Count;
			tex2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;

			ID3D11Texture2D* pZBufferTexture;
			hr = g_pDevice->CreateTexture2D(&tex2dDesc, nullptr, &pZBufferTexture);

			if(FAILED(hr)) return hr;

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			ZeroMemory(&dsvDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));

			dsvDesc.Format = tex2dDesc.Format;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

			hr = g_pDevice->CreateDepthStencilView(pZBufferTexture, &dsvDesc, &g_pZBuffer);
			if (FAILED(hr)) return hr;
			SAFE_RELEASE(pZBufferTexture);
			SAFE_RELEASE(pBuffer);

			g_pContext->OMSetRenderTargets(1, &g_pRTView, g_pZBuffer.Get());


			D3D11_VIEWPORT vp;
			vp.Width = (FLOAT) width;
			vp.Height = (FLOAT)height;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0.0f;
			vp.TopLeftY = 0.0f;
			g_pContext->RSSetViewports(1, &vp);
		}
		return 1;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HRESULT InitializeD3D()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;

#if defined (DEBUG) | defined (_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif


	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE, //comment out this line to test D3D 11.1 functionality on hardware that does not support it
		D3D_DRIVER_TYPE_WARP, //comment out also to use reference device
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC swapDesc;
	ZeroMemory(&swapDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapDesc.BufferCount = 1;
	swapDesc.BufferDesc.Width = width;
	swapDesc.BufferDesc.Height = height;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Numerator = 180;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = g_hWnd;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.Windowed = true;

	for (UINT driverTypeIndex = 0; driverTypeIndex<numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &swapDesc, &g_pSwapChain, &g_pDevice, &g_featureLevel, &g_pContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	ID3D11Texture2D* pBackBuffer;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRTView);
	if (FAILED(hr))
		return hr;

	D3D11_TEXTURE2D_DESC tex2dDesc;
	ZeroMemory(&tex2dDesc, sizeof(D3D11_TEXTURE2D_DESC));

	tex2dDesc.Width = width;
	tex2dDesc.Height = height;
	tex2dDesc.ArraySize = 1;
	tex2dDesc.MipLevels = 1;
	tex2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	tex2dDesc.SampleDesc.Count = swapDesc.SampleDesc.Count;
	tex2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	tex2dDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D* pZBufferTex;
	hr = g_pDevice->CreateTexture2D(&tex2dDesc, nullptr, &pZBufferTex);
	if (FAILED(hr))
		return hr;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));

	dsvDesc.Format = tex2dDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	hr = g_pDevice->CreateDepthStencilView(pZBufferTex, &dsvDesc, &g_pZBuffer);
	if (FAILED(hr))
		return hr;

	SAFE_RELEASE(pZBufferTex);

	g_pContext->OMSetRenderTargets(1, g_pRTView.GetAddressOf(), g_pZBuffer.Get());

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (FLOAT)width;
	viewport.Height = (FLOAT)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	g_pContext->RSSetViewports(1, &viewport);

	return S_OK;
}

HRESULT InitializeGraphics()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.left;
	D3D11_VIEWPORT viewport;

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (FLOAT)width;
	viewport.Height = (FLOAT)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	g_pSceneManager = new SceneManager(g_pDevice.Get(), g_pContext.Get(), &g_GameTimer);
	hr = g_pSceneManager->Initialize();
	if (FAILED(hr)) return hr;
	
	return S_OK;
}

void Render(void) 
{

	float rgba_clear_color[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
	g_pContext->ClearRenderTargetView(g_pRTView.Get(), rgba_clear_color);
	g_pContext->ClearDepthStencilView(g_pZBuffer.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, (UINT) 0.0f);
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.left;
	
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (FLOAT)width;
	viewport.Height = (FLOAT)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//g_pSceneManager->SetViewPort(&viewport);
	g_pSceneManager->Render();

	g_pSwapChain->Present(0, 0);
}


void CalculateFPS()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;
	if((g_GameTimer.TotalTime() - timeElapsed)>=1.0f)
	{
		float fps = (float) frameCnt;
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring text= L"Dynamic Snow Shader: FPS: " + fpsStr + L" FrameTime: "+ mspfStr+L" ms";
		SetWindowText(g_hWnd, text.c_str());

		frameCnt=0;
		timeElapsed += 1.0f;
	}
	
}

void ReportLiveObjects()
{
#if defined (DEBUG) | defined (_DEBUG)
	ID3D11Debug* DebugDevice = nullptr;
	HRESULT result = g_pDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&DebugDevice));
	if (FAILED(result))
		return;
	result = DebugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	if (FAILED(result)) 
		return;
	SAFE_RELEASE(DebugDevice);
#endif
}

void ShutdownD3D()
{
	g_pContext->ClearState();
	g_pContext->Flush();
	
	if(g_pSceneManager)
	{
		delete g_pSceneManager;
		g_pSceneManager = nullptr;
	}
	/*if(g_pInput)
	{
		delete g_pInput;
		g_pInput = nullptr;
	}
	SAFE_RELEASE(g_pCB0);
	SAFE_RELEASE(g_pVertexBuffer);
	SAFE_RELEASE(g_pVShader);
	SAFE_RELEASE(g_pPShader);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pZBuffer);
	SAFE_RELEASE(g_pSwapChain);
	SAFE_RELEASE(g_pRTView);

	SAFE_RELEASE(g_pContext);
	SAFE_RELEASE(g_pDevice);
	*/
}

