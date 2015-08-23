#ifndef MESH_H
#define MESH_H
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <iostream>
#include <vector>
#include <directxcolors.h>
#include "DDSTextureLoader.h"
using namespace DirectX;
using namespace std;
struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	XMFLOAT3 Norm;
};

class Mesh {
public:
	/*  Mesh Data  */
	
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	/*  Functions  */
	// Constructor
	Mesh(vector<SimpleVertex> vertices, vector<UINT> indices)
	{
		this->vertices = vertices;
		this->indices = indices;
		texture = nullptr;
	}

	// Render the mesh
	void Draw()
	{
		// Bind appropriate textures
		
	}

	vector<SimpleVertex> GetVertices()
	{
		return vertices;
	}

	vector<UINT> GetIndices()
	{
		return indices;
	}

	ID3D11Buffer* GetVertexBuffer()
	{
		return vertexBuffer;
	}

	ID3D11Buffer* GetIndexBuffer()
	{
		return indexBuffer;
	}

	ID3D11ShaderResourceView* GetTexture()
	{
		return texture;
	}

	bool SetBuffers(ID3D11Device* device)
	{
		HRESULT hr = S_OK;
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(SimpleVertex)* this->vertices.size();
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = &this->vertices[0];
		hr = device->CreateBuffer(&bd, &InitData, &this->vertexBuffer);
		if (FAILED(hr))
			return !!hr;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(int)* this->indices.size();
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = &this->indices[0];
		hr = device->CreateBuffer(&bd, &InitData, &this->indexBuffer);
		if (FAILED(hr))
			return !!hr;
		return S_OK;

	}
	void SetTexture(ID3D11Device* device, WCHAR* texFileName)
	{
		HRESULT hr = CreateDDSTextureFromFile(device, texFileName, nullptr, &this->texture);
		if (FAILED(hr))
		{
			cout << "Texture loading failed!" << endl;
		}
	}
private:
	vector<SimpleVertex> vertices;
	vector<UINT> indices;
	ID3D11ShaderResourceView* texture;
};

#endif