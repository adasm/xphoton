#pragma once
#include "main.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p)=0; } }
#endif

typedef void (*OnWindowResize)();

struct SWindow
{
	bool Fullscreen;
	bool Vsync;

	HWND hWnd;
	HINSTANCE hInstance;

	int Width;
	int Height;

	int Top;
	int Left;
};


struct SMouseInfo
{
	int X, Y, PX, PY;
	bool LeftButton;
	bool RightButton;
};


extern ID3D11Device* g_pd3dDevice;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView *g_pRenderTargetView;
extern ID3D11DeviceContext* g_pImmediateContext;

extern SWindow g_Window;

extern bool g_IsDeviceLost;

extern bool g_Done;
extern bool g_Keys[256];
extern bool g_Started;

extern float g_DeltaTime;

extern SMouseInfo g_MouseInfo;



bool Demo_InitDirect3D(int AdapterId);
void Demo_ReleaseDirect3D();


extern OnWindowResize g_pOnWindowResize;

void Demo_SetCursor(LPCSTR CursorName);
void Demo_ResizeWindow(int Width, int Height);
bool Demo_InitWindow(LPCSTR lpszTyt);
void Demo_DestroyWindow();

void Demo_Render();

void Demo_DestroyWindow();
void Demo_ApplyWindowSettings(SWindow *NewWindow);