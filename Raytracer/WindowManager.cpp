#include "stdafx.h"
#include "WindowManager.h"


bool g_Done = false;
bool g_Keys[256];
bool g_Started = false;
float g_DeltaTime = 0.0f;
SMouseInfo g_MouseInfo;


bool g_IsCursorShown = true;
bool g_IsDeviceLost = false;
SWindow g_Window;


ID3D11Device* g_pd3dDevice = 0;
IDXGISwapChain* g_pSwapChain = 0;
ID3D11RenderTargetView *g_pRenderTargetView = 0;
ID3D11DeviceContext* g_pImmediateContext = 0;

OnWindowResize g_pOnWindowResize = 0;

const DWORD WindowedExStyle = WS_EX_WINDOWEDGE;
const DWORD WindowedStyle = WS_OVERLAPPEDWINDOW; //WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU  | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
const DWORD FullscreenExStyle = 0;
const DWORD FullscreenStyle = WS_POPUP | WS_SYSMENU;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



void Demo_SetCursor(LPCSTR CursorName)
{
	SetClassLong(g_Window.hWnd, GCLP_HCURSOR, (LONG)LoadCursor(NULL, CursorName));
}

bool Demo_InitWindow(LPCSTR lpszTyt)
{
	WNDCLASS wc;
	wc.lpszClassName	= "WndClass";
	wc.hInstance		= g_Window.hInstance;
	wc.lpfnWndProc		= WndProc;
	wc.style			= 0;
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName		= NULL;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;

	if(!RegisterClass(&wc)) 
		return false;


	RECT WindowRect;
	WindowRect.left = (long)0;
	WindowRect.right = (long)g_Window.Width;
	WindowRect.top = (long)0;
	WindowRect.bottom = (long)g_Window.Height;


	if (g_Window.Fullscreen)
	{
		g_Window.hWnd = CreateWindowEx(FullscreenExStyle, "WndClass", lpszTyt,
						FullscreenStyle, 0, 0, g_Window.Width, g_Window.Height,
						NULL, NULL, g_Window.hInstance, NULL);
	}
	else
	{
		RECT WindowRect;
		WindowRect.left = (long)0;
		WindowRect.right = (long)g_Window.Width;
		WindowRect.top = (long)0;
		WindowRect.bottom = (long)g_Window.Height;
		AdjustWindowRectEx(&WindowRect, WindowedStyle, FALSE, WindowedExStyle);	

		g_Window.Left = -WindowRect.left;
		g_Window.Top = -WindowRect.top;

		g_Window.hWnd = CreateWindowEx(WindowedExStyle, 
										"WndClass", lpszTyt,
										WindowedStyle, 
										0, 0, 
										WindowRect.right-WindowRect.left,
										WindowRect.bottom-WindowRect.top,
										NULL, NULL, g_Window.hInstance, NULL);
	}


	ShowWindow(g_Window.hWnd, SW_SHOW);
	UpdateWindow(g_Window.hWnd);

	if(g_Window.hWnd == NULL) 
		return false;

	return true;
}


void Demo_DestroyWindow()
{
	UnregisterClass("UITesterWndClass", g_Window.hInstance);
	DestroyWindow(g_Window.hWnd);
}



void SetWindowedMode(int Width, int Height)
{
	RECT WindowRect;
	WindowRect.left = (long)0;
	WindowRect.right = (long)Width;
	WindowRect.top = (long)0;
	WindowRect.bottom = (long)Height;
	AdjustWindowRectEx(&WindowRect, WindowedStyle, FALSE, WindowedExStyle);	


	SetWindowLong(g_Window.hWnd, GWL_EXSTYLE, WindowedExStyle);
	SetWindowLong(g_Window.hWnd, GWL_STYLE, WindowedStyle);
	SetWindowPos(g_Window.hWnd, HWND_NOTOPMOST, 
				 0, 0, 
				 WindowRect.right-WindowRect.left,
				 WindowRect.bottom-WindowRect.top,
				 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void SetFullscreenMode(int Width, int Height)
{
	SetWindowLong(g_Window.hWnd, GWL_EXSTYLE, FullscreenExStyle);
	SetWindowLong(g_Window.hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP );
	SetWindowPos(g_Window.hWnd, NULL, 0, 0, Width, Height, SWP_NOZORDER );
}



bool Demo_InitDirect3D(int AdapterId)
{
	//Create the Direct3D Device and Swap Chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof(sd) );
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_Window.Width;
	sd.BufferDesc.Height = g_Window.Height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_Window.hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

#ifdef _DEBUG
	UINT Flags = D3D11_CREATE_DEVICE_DEBUG;
#else
	UINT Flags = 0;
#endif


	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};


	if( FAILED( D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, Flags, FeatureLevels, 3, D3D11_SDK_VERSION, 
												&sd, &g_pSwapChain, &g_pd3dDevice, 0, &g_pImmediateContext) ) )
	{
		MessageBoxA(g_Window.hWnd, "Failed to create Direct3D device!", "Error", 0);
		return false;
	}

	// Create a render target view(from Backbuffer)
	ID3D11Texture2D *pBackBuffer = NULL;
	if( FAILED(g_pSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer ) ) )
		return false;

	HRESULT hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if( FAILED( hr ) )
		return false;
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);


	return true;
}

void Demo_ReleaseDirect3D()
{
	SAFE_RELEASE(g_pImmediateContext);
	SAFE_RELEASE(g_pd3dDevice);
	SAFE_RELEASE(g_pSwapChain);
	SAFE_RELEASE(g_pRenderTargetView);
}


void Demo_ResetDevice()
{
	HRESULT HR;

	DXGI_SWAP_CHAIN_DESC newMode;
	g_pSwapChain->GetDesc(&newMode);

	SAFE_RELEASE(g_pRenderTargetView);

	HR = g_pSwapChain->ResizeBuffers(newMode.BufferCount, g_Window.Width, g_Window.Height, 
									newMode.BufferDesc.Format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	ID3D11Texture2D *pBackBuffer = NULL;
	if( FAILED(g_pSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer ) ) )
		return;

	HRESULT hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if( FAILED( hr ) )
		return;
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
}



void Demo_ApplyWindowSettings(SWindow *NewWindow)
{
	if (!(NewWindow->Width > 0 && NewWindow->Height > 0))
		return;

	if (g_Window.Width != NewWindow->Width ||
		g_Window.Height != NewWindow->Height ||
		g_Window.Fullscreen != NewWindow->Fullscreen ||
		g_Window.Vsync != NewWindow->Vsync)
	{
		//wylacz fullscreen
		bool bApplyWindowedMode = (g_Window.Fullscreen) && (!NewWindow->Fullscreen);
		bool bApplyFullscreenMode = (!g_Window.Fullscreen) && (NewWindow->Fullscreen);


		g_Window = *NewWindow;
		Demo_ResetDevice();

		//resize & enter windowed mode
		if (bApplyWindowedMode)
			SetWindowedMode(g_Window.Width, g_Window.Height);

		if (bApplyFullscreenMode)
			SetFullscreenMode(g_Window.Width, g_Window.Height);
	}
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!g_Started)
		return DefWindowProc(hWnd,uMsg,wParam,lParam);


	if (hWnd == g_Window.hWnd)
	switch (uMsg)
	{
		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
				return 0;						
			}
			break;	
		}

		case WM_CLOSE:
		{
			PostQuitMessage(0);	
			return 0;
		}


		case WM_CHAR:
		{
			return 0;
		}
		

		case WM_KEYDOWN:
		{
			g_Keys[wParam] = true;
			return 0;
		}

		case WM_KEYUP:
		{
			g_Keys[wParam] = false;
			return 0;
		}

		case WM_SIZE:
		{
			SWindow TmpWindow = g_Window;
			TmpWindow.Width = LOWORD(lParam);
			TmpWindow.Height = HIWORD(lParam);
			Demo_ApplyWindowSettings(&TmpWindow);

			if (g_pOnWindowResize)
				(*g_pOnWindowResize)();

			return 0;
		}


		case WM_LBUTTONDOWN:
		{
			g_MouseInfo.LeftButton = true;
            g_MouseInfo.PX = LOWORD(lParam);
            g_MouseInfo.PY = HIWORD(lParam);
            g_MouseInfo.X = g_MouseInfo.PX;
            g_MouseInfo.Y = g_MouseInfo.PY;

			SetCapture(hWnd);
            return 0;
		}

		case WM_LBUTTONUP:
		{
			g_MouseInfo.LeftButton = false;
			ReleaseCapture();
            return 0;
		}

		case WM_RBUTTONDOWN:
		{
			g_MouseInfo.RightButton = true;
            g_MouseInfo.PX = LOWORD(lParam);
            g_MouseInfo.PY = HIWORD(lParam);
            g_MouseInfo.X = g_MouseInfo.PX;
            g_MouseInfo.Y = g_MouseInfo.PY;
            return 0;
		}

		case WM_RBUTTONUP:
		{
			g_MouseInfo.RightButton = false;
            return 0;
		}

		case WM_MOUSEMOVE:
		{
			g_MouseInfo.X = MAKEPOINTS(lParam).x;
			g_MouseInfo.Y = MAKEPOINTS(lParam).y;

            return 0;
		}

		case WM_MOUSEWHEEL:
		{
            return 0;
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}