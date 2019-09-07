#pragma once
#include <dinput.h>
#include "RainCompute.h"
#include "SnowTexture.h"
#include "scene_node.h"

class Input
{
private:
	IDirectInput8A* g_direct_input;
	IDirectInputDevice8A* g_keyboard_device;
	IDirectInputDevice8A* g_mouse_device;
	DIMOUSESTATE g_mouse_state;
	unsigned char g_keyboard_state[256];
	HWND g_iWnd;
public:
	Input();
	HRESULT Initialize(HINSTANCE inst, HWND wnd);
	void ReadInputStates();
	bool IsKeyPressed(unsigned char DI_keycode);
	void KeyboardInput(camera* cam, RainCompute* rain, SnowTexture* snow, scene_node* sn, float dt);
	void MouseInput(camera* cam);
	~Input();
};

