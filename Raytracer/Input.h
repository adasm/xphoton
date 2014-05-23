#pragma once

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxerr.lib")

//DirectInput
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h> 

class Input
{
private:
	LPDIRECTINPUT8 			lpdi;
	LPDIRECTINPUTDEVICE8	lpdikeyboard; 
	LPDIRECTINPUTDEVICE8	lpdimouse;
	HANDLE					hmouseevent;
	DIMOUSESTATE			mouse_state;
	u8						keystate[256];
	u8						oldKeystate[256];
	float2					MousePositionRel;
	float2					MousePositionAbs;
	f32						MousePosZRel;
	u32						MouseButton[4];
	u32						MouseButtonOld[4];
public:
	Input() : lpdi(0), lpdikeyboard(0), lpdimouse(0) { }
	u32		init(HINSTANCE hInstance, HWND hwnd);
	void	close();
	void	clear();
	void	update();
	u32		isKeyDown(u32 key);
	u32		isKeyUp(u32 key);
	u32		isKeyPressed(u32 key);
	u32		isMousePressed(u32 buttonNumber);
	float2*	getMouseAbs();
	float2*	getMouseRel();
	f32		getMouseZRel();
};