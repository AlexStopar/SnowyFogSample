#ifndef INPUT_H
#define INPUT_H

#define DIRECTINPUT_VERSION 0x0800
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#include <dinput.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include "Camera.h"
#include "Player.h"
using namespace DirectX;

class Input
{
public:
	Input();
	Input(const Input&) {}
	~Input() {}
	bool Initialize(HINSTANCE hinstance, HWND hwnd);
	void Shutdown();
	XMFLOAT3 Frame(Player &player, Camera &camera, HWND hwnd, float deltaTime);
	bool IsQuitting() //Quits the application
	{
		return !!(keyboardState[DIK_ESCAPE] & 0x80);
	}
	bool IsMakingCopy() //Makes a copy object
	{
		return !!(keyboardState[DIK_SPACE] & 0x80);
	}
private:
	float lastX, lastY; //Last known mouse location
	bool ReadKeyboard();
	bool ReadMouse(HWND hwnd);
	XMFLOAT3 ProcessInput(Player &player, Camera &camera, float deltaTime);
	IDirectInput8* directInput8; //DirectInput class
	IDirectInputDevice8* keyboard;
	POINT mouse; //Raw Input for mouse
	unsigned char keyboardState[256];
};

#endif