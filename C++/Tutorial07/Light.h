#ifndef LIGHT_H
#define LIGHT_H
#include <d3d11_1.h>
#include <directxmath.h>
using namespace DirectX;
class Light
{
public:
	Light() {}
	Light(const Light&) {}
	~Light() {}

	void SetAmbientColor(float red, float blue, float green, float alpha)
	{
		this->ambientColor = XMFLOAT4(red, blue, green, alpha);
	}
	void SetDiffuseColor(float red, float blue, float green, float alpha)
	{
		this->diffuseColor = XMFLOAT4(red, blue, green, alpha);
	}
	void SetDirection(float x, float y, float z)
	{
		this->direction = XMFLOAT3(x, y, z);
	}
	void SetSpecularColor(float red, float blue, float green, float alpha)
	{
		this->specularColor = XMFLOAT4(red, blue, green, alpha);
	}
	void SetSpecularPower(float power)
	{
		this->specularPower = power;
	}

	XMFLOAT4 GetAmbientColor() { return this->ambientColor; }
	XMFLOAT4 GetDiffuseColor() { return this->diffuseColor; }
	XMFLOAT3 GetDirection() { return this->direction; }
	XMFLOAT4 GetSpecularColor() { return this->specularColor; }
	float GetSpecularPower() { return this->specularPower; }

private:
	XMFLOAT4 ambientColor;
	XMFLOAT4 diffuseColor;
	XMFLOAT3 direction;
	XMFLOAT4 specularColor;
	float specularPower;
};
#endif