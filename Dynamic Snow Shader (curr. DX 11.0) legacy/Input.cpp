#pragma once
#include "Input.h"
#include "pch.h"
#include <minwinbase.h>


Input::Input()
{
	
}

HRESULT Input::Initialize(HINSTANCE inst, HWND wnd)
{
	g_iWnd = wnd;
	
	HRESULT hr;
	ZeroMemory(g_keyboard_state, sizeof(g_keyboard_state));
	//setting up the keyboard
	hr = DirectInput8Create(inst, DIRECTINPUT_VERSION, IID_IDirectInput8A, reinterpret_cast<void**>(&g_direct_input), nullptr);
	if (FAILED(hr)) return hr;
	hr = g_direct_input->CreateDevice(GUID_SysKeyboard, &g_keyboard_device, nullptr);
	if (FAILED(hr)) return hr;
	hr = g_keyboard_device->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr)) return hr;
	hr = g_keyboard_device->SetCooperativeLevel(wnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(hr)) return hr;
	hr = g_keyboard_device->Acquire();
	if (FAILED(hr)) return hr;
	//setting up the mouse --  is set to exclusive to get rid of the mouse pointer
	hr = g_direct_input->CreateDevice(GUID_SysMouse, &g_mouse_device, nullptr);
	if (FAILED(hr)) return hr;
	hr = g_mouse_device->SetDataFormat(&c_dfDIMouse);
	if (FAILED(hr)) return hr;
	hr = g_mouse_device->SetCooperativeLevel(wnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (FAILED(hr)) return hr;
	hr = g_mouse_device->Acquire();
	if (FAILED(hr)) return hr;

	return S_OK;
}

void Input::ReadInputStates()
{
	HRESULT hr;
	hr = g_keyboard_device->GetDeviceState(sizeof(g_keyboard_state), (LPVOID) &g_keyboard_state);

	if(FAILED(hr))
	{
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			g_keyboard_device->Acquire();
		}
	}

	hr = g_mouse_device->GetDeviceState(sizeof(g_mouse_device), (LPVOID) &g_mouse_device);
	if(FAILED(hr))
	{
		if((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			g_mouse_device->Acquire();
		}
	}
}

bool Input::IsKeyPressed(unsigned char DI_keycode)
{
	return g_keyboard_state[DI_keycode] & 0x80;
}

void Input::KeyboardInput(camera* cam, RainCompute* rain, SnowTexture* snow, scene_node* sn, float dt)
{
	if (IsKeyPressed(DIK_ESCAPE)) DestroyWindow(g_iWnd);
	if (IsKeyPressed(DIK_W)) cam->Forward(5.0f*dt);
	if (IsKeyPressed(DIK_A)) cam->Strafe(-5.0f*dt);
	if (IsKeyPressed(DIK_D)) cam->Strafe(5.0f*dt);
	if (IsKeyPressed(DIK_S)) cam->Forward(-5.0f*dt);
	if (IsKeyPressed(DIK_Q)) cam->Rotate(50.0f*dt, .0f*dt);
	if (IsKeyPressed(DIK_E)) cam->Rotate(-50.0f*dt, 0.f*dt);
	if (IsKeyPressed(DIK_Z)) cam->Rotate(0.f*dt, 50.0f*dt);
	if (IsKeyPressed(DIK_X)) cam->Rotate(0.f*dt, -50.0f*dt);
	if (IsKeyPressed(DIK_I)) cam->Jump(1.0f*dt);
	if (IsKeyPressed(DIK_K)) cam->Jump(-1.0f*dt);

	if (IsKeyPressed(DIK_1)) rain->SetDensity(0.0f);
	if (IsKeyPressed(DIK_2)) rain->SetDensity(0.5f);
	if (IsKeyPressed(DIK_3)) rain->SetDensity(1.0f);
	if (IsKeyPressed(DIK_4)) sn->ToggleDrawing();
	if (IsKeyPressed(DIK_5)) snow->SwitchRasterState();
}

void Input::MouseInput(camera* cam)
{

}


Input::~Input()
{
	if (g_iWnd) g_iWnd = nullptr;
	if(g_keyboard_device)
	{
		g_keyboard_device->Unacquire();
		g_keyboard_device->Release();
	}
	if(g_mouse_device)
	{
		g_mouse_device->Unacquire();
		g_mouse_device->Release();
	}
	if (g_direct_input) g_direct_input->Release();
}
