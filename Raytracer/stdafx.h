#pragma once

#include "targetver.h"

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

//DirectX
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx9.h>
#include <D3DCompiler.h>

#ifndef D3D_SAFE_RELEASE
#define D3D_SAFE_RELEASE(p) { if (p) { (p)->Release(); (p)=0; } }
#endif

//XNA Math library
#include <xnamath.h>

//Windows and common STL
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <hash_map>
#include <queue>
#include <time.h>
#include <tchar.h>
#include <malloc.h>
#include <memory.h>
#include <strsafe.h>
#include <process.h>
#include <tchar.h>
#include <stdio.h>

//Types
typedef HALF				half;
typedef XMHALF2				half2;
typedef XMHALF4				half4;
typedef XMFLOAT2			float2;
typedef XMFLOAT3			float3;
typedef XMFLOAT4			float4;
typedef XMFLOAT4X4			float4x4;
typedef XMVECTOR			vector;
typedef XMMATRIX			matrix;
typedef char				c8;
typedef	unsigned char		u8;
typedef signed short		i16;
typedef	unsigned short		u16;
typedef __int32				i32;
typedef unsigned __int32	u32;
typedef __int64				i64;
typedef unsigned __int64	u64;
typedef float				f32;