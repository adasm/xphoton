#include "stdafx.h"
#include "Input.h"

#define LOG_ERROR(s) MessageBox(0, s, TEXT(""), 0);

u32 Input::init(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT  hr; 
	hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&lpdi, NULL); 
	if FAILED(hr) { LOG_ERROR(TEXT("Initializing DXInput8 failed on DirectInput8Create")); close(); return FALSE;}
	hr = lpdi->CreateDevice(GUID_SysKeyboard, &lpdikeyboard, NULL);     
	if FAILED(hr) { LOG_ERROR(TEXT("Initializing DXInput8 failed on CreateDevice(GUID_SysKeyboard)")); close(); return FALSE;}
	hr = lpdi->CreateDevice(GUID_SysMouse, &lpdimouse, NULL); 
	if FAILED(hr) { LOG_ERROR(TEXT("Initializing DXInput8 failed on CreateDevice(GUID_SysMouse)")); close(); return FALSE;}
	hr = lpdikeyboard->SetDataFormat(&c_dfDIKeyboard); 
	if FAILED(hr) { LOG_ERROR(TEXT("Initializing DXInput8 failed on SetDataFormat(&c_dfDIKeyboard)")); close(); return FALSE;}
	hr = lpdimouse->SetDataFormat(&c_dfDIMouse); 
	if FAILED(hr) { LOG_ERROR(TEXT("Initializing DXInput8 failed on SetDataFormat(&c_dfDIMouse)")); close(); return FALSE;}
	hr = lpdikeyboard->SetCooperativeLevel(hwnd,DISCL_BACKGROUND  | DISCL_NONEXCLUSIVE); 
	if FAILED(hr) { LOG_ERROR(TEXT("Initializing DXInput8 failed on lpdikeyboard->SetCooperativeLevel")); close();  return FALSE;}
	hr = lpdimouse->SetCooperativeLevel(hwnd,DISCL_BACKGROUND  | DISCL_NONEXCLUSIVE); 
	if FAILED(hr) { LOG_ERROR(TEXT("Initializing DXInput8 failed on lpdimouse->SetCooperativeLevel")); close(); return FALSE;}
	hmouseevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hmouseevent == NULL) { LOG_ERROR(TEXT("Initializing DXInput8 failed on CreateEvent")); close(); return FALSE;}
	hr = lpdimouse->SetEventNotification(hmouseevent);
	if (FAILED(hr)) { LOG_ERROR(TEXT("Initializing DXInput8 failed on lpdimouse->SetEventNotification")); close(); return FALSE;}
	DIPROPDWORD dipdw;
	dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj        = 0;
	dipdw.diph.dwHow        = DIPH_DEVICE;
	dipdw.dwData            = 16;
	hr = lpdimouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);
	if (FAILED(hr)) { LOG_ERROR(TEXT("Initializing DXInput8 failed on lpdimouse->SetProperty")); close(); return FALSE;}
	hr = lpdikeyboard->Acquire(); 
	if FAILED(hr) { LOG_ERROR(TEXT("Initializing DXInput8 failed on lpdikeyboard->Acquire")); close(); return FALSE;}
	hr = lpdimouse->Acquire(); 
	if FAILED(hr) { LOG_ERROR(TEXT("Initializing DXInput8 failed on lpdimouse->Acquire")); close();return FALSE;}
	clear();
	return TRUE; 
}

void Input::clear()
{
	MousePositionRel = MousePositionAbs = float2(0,0);
	MousePosZRel = 0;
	memset(oldKeystate, 0, sizeof(u8)*256);
	memset(keystate, 0, sizeof(u8)*256);
}

void Input::close()
{
	if (lpdi) 
	{ 
		if (lpdikeyboard) 
		{ 
			lpdikeyboard->Unacquire(); 
			lpdikeyboard->Release();
			lpdikeyboard = NULL; 
		} 

		if (lpdimouse) 
		{ 
			lpdimouse->Unacquire(); 
			lpdimouse->Release();
			lpdimouse = NULL; 
		}
		
		if (hmouseevent) 
		{CloseHandle(hmouseevent);hmouseevent = NULL;}
		lpdi->Release();
		lpdi = NULL; 
	}
}


void Input::update()
{
	bool mousemove = false;
	lpdimouse->GetDeviceState(sizeof(mouse_state), (LPVOID)&mouse_state);
	MousePositionRel.x = (float)mouse_state.lX;
	MousePositionRel.y = (float)mouse_state.lY; 
	POINT absp;
	GetCursorPos(&absp);
	MousePositionAbs.x = absp.x;
	MousePositionAbs.y = absp.y;

	if(MousePositionRel.x != 0 || MousePositionRel.y != 0) mousemove = true;

/*	if(mousemove)
	{
		Graph::GUI::desktop.postMessage(WM_MOUSEMOVE, MousePositionAbs.x, MousePositionAbs.y, NULL);
	}*/
	
	MousePosZRel = (float)mouse_state.lZ;

	for (int i = 0; i < 4; i++) 
	{
		MouseButtonOld[i] = MouseButton[i];
		MouseButton[i] = (mouse_state.rgbButtons[i] & 0x80)?1:0;
	}

	/*if((MouseButtonOld[0] != MouseButton[0]) && MouseButton[0])
		Graph::GUI::desktop.postMessage(WM_LBUTTONDOWN, MousePositionAbs.x, MousePositionAbs.y, NULL);
	if((MouseButtonOld[0] != MouseButton[0]) && !MouseButton[0])
		Graph::GUI::desktop.postMessage(WM_LBUTTONUP, MousePositionAbs.x, MousePositionAbs.y, NULL);*/

	memcpy(oldKeystate, keystate, sizeof(u8)*256);
	lpdikeyboard->GetDeviceState(sizeof(u8[256]), (LPVOID)keystate);
}

u32 Input::isKeyDown(u32 key)
{
	return ((keystate[key] & 0x80) ? 1 : 0);
}

u32 Input::isKeyUp(u32 key)
{
	return !((keystate[key] & 0x80) ? 1 : 0);
}

u32 Input::isKeyPressed(u32 key)
{
	return ((oldKeystate[key] & 0x80) ? 1 : 0)  && (!((keystate[key] & 0x80) ? 1 : 0));
}

u32 Input::isMousePressed(u32 buttonNumber)
{
	if(buttonNumber >= 0 && buttonNumber <= 3)
	{
		return MouseButton[buttonNumber];
	}
	else return 0;
}
/*	
bool Input::isMousePressedOld(int buttonNumber)
{
	if(buttonNumber >= 0 && buttonNumber <= 3)
	{
		return MouseButtonOld[buttonNumber];
	}
	else return false;
}
*/
float2* Input::getMouseAbs()
{
	return &MousePositionAbs;
}

float2* Input::getMouseRel()
{
	return &MousePositionRel;
}

f32 Input::getMouseZRel()
{
	return MousePosZRel;
}