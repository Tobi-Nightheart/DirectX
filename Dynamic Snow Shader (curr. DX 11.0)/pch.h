#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <d3dx11.h>
#include <windows.h>
#include <dxerr.h>
#include "camera.h"
#define _XM_NO_INTRINSICS_
#define XM_NO_ALIGNMENT
//#include <DirectXMath.h>
//#include <DirectXPackedVector.h>
#include <xnamath.h>
#ifndef GOTO_EXIT_IF_FAILED
#define GOTO_EXIT_IF_FAILED(hr) if(FAILED(hr)) goto Exit;
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) \
	if(p)			\
	{				\
		delete (p);	 \
		(p)	= NULL;	\
	}				
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) \
   if(p)        \
   {                    \
      (p)->Release();     \
      (p) = NULL;         \
   }
#endif

#ifndef SAFE_ARRAY_DELETE
#define SAFE_ARRAY_DELETE(p) \
   if(p)             \
   {                         \
      delete[] (p);            \
      (p) = NULL;              \
   }
#endif