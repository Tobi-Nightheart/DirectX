#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>


#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "dxgi.lib")

#include <wrl.h>

#pragma region Macros
#ifndef ThrowIfFailed
#define ThrowIfFailed(x)											\
{																	\
	HRESULT hr__ = (x);												\
	wstring wfn = AnsiToWString(__FILE__);							\
	if(FAILED(hr__)) {throw DxException(hr__, L#x, wfn, __Line__);}	\
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x)	{if(x){x->Release(); x = 0;}}
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
	if(x!=nullptr)			\
	{					\
		x->Release();	\
		x=nullptr;			\
	}
#endif


#pragma endregion
