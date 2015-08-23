#include "Input.h"
#include <cmath>
using namespace std;

Input::Input()
{
	directInput8 = nullptr;
	keyboard = nullptr;
}

bool Input::Initialize(HINSTANCE hinstance, HWND hwnd)
{
	HRESULT result;

	//Create Direct Input device
	result = DirectInput8Create(hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&directInput8, NULL);
	if (FAILED(result))
		return !!result;

	//Create keyboard
	result = directInput8->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	if (FAILED(result))
		return !!result;
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(result))
		return !!result;

	//Set cooperative level
	result = keyboard->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
		return !!result;
	result = keyboard->Acquire();
	if (FAILED(result))
		return !!result;

	//Set mouse
	GetCursorPos(&mouse);
	ScreenToClient(hwnd, &mouse);
	//Set offsets for rotation change
	
	lastX = (float)mouse.x;
	lastY = (float)mouse.y;
	return true;
}

void Input::Shutdown()
{
	if (keyboard)
	{
		keyboard->Unacquire();
		keyboard->Release();
	}
	if (directInput8) directInput8->Release();
}

XMFLOAT3 Input::Frame(Player &player, Camera &camera, HWND hwnd, float deltaTime)
{
	ReadKeyboard();
	ReadMouse(hwnd);
	return ProcessInput(player, camera, deltaTime);
}

bool Input::ReadKeyboard()
{
	HRESULT result;

	result = keyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);
	if (FAILED(result))
	{
		if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED)) keyboard->Acquire();
		else return false;
	}
	return true;
}

bool Input::ReadMouse(HWND hwnd)
{
	GetCursorPos(&mouse);
	ScreenToClient(hwnd, &mouse);
	return true;
}

XMFLOAT3 Input::ProcessInput(Player &player, Camera &camera, float deltaTime)
{
	float xoffset = mouse.x - lastX;
	float yoffset = mouse.y - lastY;
	lastX = (float)mouse.x;
	lastY = (float)mouse.y;
	camera.SetCameraRotation(yoffset, xoffset);


	XMVECTOR direction = XMVectorZero();
	XMVECTOR addDirection = XMVectorZero(); //Direction to add to current direction
	XMVECTOR camDirection = XMVectorZero();
	if (keyboardState[DIK_W] & 0x80)
	{
		addDirection = XMVectorSet(XMVectorGetX(camera.At) - XMVectorGetX(camera.Eye),
			0.0f, XMVectorGetZ(camera.At) - XMVectorGetZ(camera.Eye), 0.0f);
		addDirection = XMVector3Normalize(addDirection);
		direction = XMVectorAdd(direction, addDirection);
	}
	if (keyboardState[DIK_S] & 0x80)
	{
		addDirection = XMVectorSet(XMVectorGetX(camera.At) - XMVectorGetX(camera.Eye),
			0.0f, XMVectorGetZ(camera.At) - XMVectorGetZ(camera.Eye), 0.0f);
		addDirection = XMVector3Normalize(addDirection);
		addDirection *= -1.0f;
		direction = XMVectorAdd(direction, addDirection);
	}
	if (keyboardState[DIK_A] & 0x80)
	{
		camDirection = XMVectorSet(XMVectorGetX(camera.At) - XMVectorGetX(camera.Eye),
			0.0f, XMVectorGetZ(camera.At) - XMVectorGetZ(camera.Eye), 0.0f);
		addDirection = camDirection;
		addDirection = XMVector3Cross(addDirection, camera.Up);
		addDirection = XMVector3Normalize(addDirection);
	   
		direction = XMVectorAdd(direction, addDirection);
	}
	if (keyboardState[DIK_D] & 0x80)
	{
		camDirection = XMVectorSet(XMVectorGetX(camera.At) - XMVectorGetX(camera.Eye),
			0.0f, XMVectorGetZ(camera.At) - XMVectorGetZ(camera.Eye), 0.0f);
		addDirection = camDirection;
		addDirection = XMVector3Cross(addDirection, camera.Up);
		addDirection *= -1.0f;
		addDirection = XMVector3Normalize(addDirection);
		direction = XMVectorAdd(direction, addDirection);
	}
	return XMFLOAT3((XMVectorGetX(direction) * player.Speed * deltaTime), 0, 
		(XMVectorGetZ(direction) * player.Speed * deltaTime));
	
}