#ifndef SKYSHADER_H
#define SKYSHADER_H
#include "Shader.h"
using namespace DirectX;

class SkyShader : public Shader
{
public:
	SkyShader() : Shader()
	{

	}
	bool SkyShader::InitShader(ID3D11Device* device, HWND window, WCHAR* vs, WCHAR* ps)
	{
		ID3DBlob* pVSBlob = nullptr;
		HRESULT hr = S_OK;
		hr = CompileShaderFromFile(vs, "VS", "vs_4_0", &pVSBlob);
		if (FAILED(hr))
		{
			MessageBox(nullptr,
				L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
			return hr;
		}
		// Create the vertex shader
		hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &vertexShader);
		if (FAILED(hr))
		{
			pVSBlob->Release();
			return hr;
		}

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);

		// Create the input layout
		hr = device->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
			pVSBlob->GetBufferSize(), &vertexLayout);
		pVSBlob->Release();
		if (FAILED(hr))
			return hr;

		// Compile the pixel shader
		ID3DBlob* pPSBlob = nullptr;
		hr = CompileShaderFromFile(ps, "PS", "ps_4_0", &pPSBlob);
		if (FAILED(hr))
		{
			MessageBox(nullptr,
				L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
			return hr;
		}

		// Create the pixel shader
		hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pixelShader);
		pPSBlob->Release();
		if (FAILED(hr))
			return hr;

		//Set matrix buffer
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(MatrixBuffer);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		hr = device->CreateBuffer(&bd, nullptr, &matrixBuffer);
		if (FAILED(hr))
			return hr;

		//Set color buffer
		bd.ByteWidth = sizeof(ColorBuffer);
		hr = device->CreateBuffer(&bd, nullptr, &colorBuffer);
		if (FAILED(hr))
			return hr;

		// Create the sample state
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = device->CreateSamplerState(&sampDesc, &sampleState);
		if (FAILED(hr))
			return hr;
	}

private:

};
#endif