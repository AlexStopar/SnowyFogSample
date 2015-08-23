#ifndef SHADER_H
#define SHADER_H
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
using namespace DirectX;

struct MatrixBuffer
{
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
};

struct ColorBuffer
{ 
	XMFLOAT4 vMeshColor; //In case we modify color of mesh
};

struct LightBuffer
{
	XMFLOAT4 ambientColor;
	XMFLOAT4 diffuseColor;
	XMFLOAT3 lightDirection;
	float specularPower;
	XMFLOAT4 specularColor;
};

struct CameraBuffer
{
	XMFLOAT3 cameraPosition;
	float padding;
};

struct FogBuffer
{
	float fogNear;
	float fogFar;
	float isFogOn;
	float fogPower;
};
class Shader
{
public:

	Shader();
	Shader(const Shader&) {}
	~Shader() {}

	bool InitShader(ID3D11Device* device, HWND window, WCHAR* vs, WCHAR* ps);
	void Shutdown();
	bool Render(ID3D11DeviceContext* deviceContext, int indexCount, MatrixBuffer &matrices, ColorBuffer &color, LightBuffer &light,
		CameraBuffer &cam, FogBuffer &fog, ID3D11ShaderResourceView* texture);
protected:
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint,
		LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	ID3D11VertexShader* vertexShader; //Files to use for vertex and fragment shading
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* vertexLayout;
	ID3D11Buffer* matrixBuffer; //Constant buffers
	ID3D11Buffer* colorBuffer;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;
	ID3D11Buffer* fogBuffer;
	ID3D11SamplerState* sampleState;
};

#endif