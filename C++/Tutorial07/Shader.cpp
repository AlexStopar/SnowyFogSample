#include "Shader.h"

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
using namespace DirectX;


Shader::Shader()
{
	vertexShader = 0;
	pixelShader = 0;
	vertexLayout = 0;
	matrixBuffer = 0;
	colorBuffer = 0;
	lightBuffer = 0;
	cameraBuffer = 0;
	fogBuffer = 0;
	sampleState = 0;
}

bool Shader::InitShader(ID3D11Device* device, HWND window, WCHAR* vs, WCHAR* ps)
{
	//Time to compile vertex shader
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
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	if (FAILED(hr)) return hr;

		
	//Set other constant buffers
	bd.ByteWidth = sizeof(ColorBuffer);
	hr = device->CreateBuffer(&bd, nullptr, &colorBuffer);
	if (FAILED(hr)) return hr;
	
	bd.ByteWidth = sizeof(LightBuffer);
	hr = device->CreateBuffer(&bd, nullptr, &lightBuffer);
	if (FAILED(hr)) return hr;

	bd.ByteWidth = sizeof(CameraBuffer);
	hr = device->CreateBuffer(&bd, nullptr, &cameraBuffer);
	if (FAILED(hr)) return hr;

	bd.ByteWidth = sizeof(FogBuffer);
	hr = device->CreateBuffer(&bd, nullptr, &fogBuffer);
	if (FAILED(hr)) return hr;

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

void Shader::Shutdown()
{
	if (vertexShader) vertexShader->Release();
	if (pixelShader) pixelShader->Release();
	if (vertexLayout) vertexLayout->Release();
	if (matrixBuffer) matrixBuffer->Release();
	if (colorBuffer) colorBuffer->Release();
	if (lightBuffer) lightBuffer->Release();
	if (cameraBuffer) cameraBuffer->Release();
	if (fogBuffer) fogBuffer->Release();
	if (sampleState) sampleState->Release();
}

bool Shader::Render(ID3D11DeviceContext* deviceContext, int indexCount, MatrixBuffer &matrices, ColorBuffer &color, LightBuffer &light,
	CameraBuffer &cam, FogBuffer&fog, ID3D11ShaderResourceView* texture)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBuffer* matrixData;
	ColorBuffer* colorData;
	LightBuffer* lightData;
	CameraBuffer* cameraData;
	FogBuffer* fogData;

	//Map the matrix buffer first
	result = deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result)) return false;
	matrixData = (MatrixBuffer*)mappedResource.pData;
	matrixData->world = matrices.world;
	matrixData->view = matrices.view;
	matrixData->projection = matrices.projection;
	deviceContext->Unmap(matrixBuffer, 0);

	result = deviceContext->Map(colorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result)) return false;
	colorData = (ColorBuffer*)mappedResource.pData;
	colorData->vMeshColor = color.vMeshColor;
	deviceContext->Unmap(colorBuffer, 0);

	result = deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result)) return false;
	lightData = (LightBuffer*)mappedResource.pData;
	lightData->ambientColor = light.ambientColor;
	lightData->diffuseColor = light.diffuseColor;
	lightData->lightDirection = light.lightDirection;
	lightData->specularColor = light.specularColor;
	lightData->specularPower = light.specularPower;
	deviceContext->Unmap(lightBuffer, 0);

	result = deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result)) return false;
	cameraData = (CameraBuffer*)mappedResource.pData;
	cameraData->cameraPosition = cam.cameraPosition;
	cameraData->padding = cam.padding;
	deviceContext->Unmap(cameraBuffer, 0);

	result = deviceContext->Map(fogBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result)) return false;
	fogData = (FogBuffer*)mappedResource.pData;
	fogData->fogNear = fog.fogNear;
	fogData->fogFar = fog.fogFar;
	fogData->isFogOn = fog.isFogOn;
	fogData->fogPower = fog.fogPower;
	deviceContext->Unmap(fogBuffer, 0);

	//Setup rendering and render!
	deviceContext->IASetInputLayout(vertexLayout);
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);
	deviceContext->VSSetConstantBuffers(3, 1, &cameraBuffer);
	deviceContext->VSSetConstantBuffers(4, 1, &fogBuffer);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(1, 1, &colorBuffer);
	deviceContext->PSSetConstantBuffers(2, 1, &lightBuffer);
	deviceContext->PSSetShaderResources(0, 1, &texture);
	deviceContext->PSSetSamplers(0, 1, &sampleState);
	deviceContext->DrawIndexed(indexCount, 0, 0);
	return S_OK;
}

HRESULT Shader::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}