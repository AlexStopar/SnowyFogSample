#ifndef PLAYER_H
#define PLAYER_H
#include <d3d11_1.h>
#include <directxmath.h>
using namespace DirectX;


struct Player
{
	XMFLOAT3 Position;
	float Speed;
	float MAX_PLAYER_SPEED;
};

#endif