#ifndef SKYBOX_H
#define SKYBOX_H
#include "Shader.h"
#include "Model.h"
#include "Mesh.h"

class Skybox
{
public:
	Skybox(){
		skyShader = Shader();
		smrv = nullptr;
		scale = XMFLOAT3(5.0f, 5.0f, 5.0f);
	}
	Shader skyShader;
	
	void Skybox::InitTexture(ID3D11Device* device)
	{
		HRESULT hr = CreateDDSTextureFromFile(device, L"skybox/skybox.dds", nullptr, &smrv);
		if (FAILED(hr)) return;
		D3D11_DEPTH_STENCIL_DESC dssDesc;
		ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		dssDesc.DepthEnable = true;
		dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		device->CreateDepthStencilState(&dssDesc, &DSLessEqual);
	}
	void Skybox::Shutdown()
	{
		skyShader.Shutdown();
		smrv->Release();
		DSLessEqual->Release();
		//RSCullNone->Release();
	}
	void Skybox::Render(ID3D11DeviceContext* deviceContext, MatrixBuffer &matrices, ColorBuffer &color, LightBuffer &light, CameraBuffer &cam,
		FogBuffer &fog)
	{
		deviceContext->OMSetDepthStencilState(DSLessEqual, 0);
		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		ID3D11Buffer* vertexBuffer = skyBoxModel.meshes[0].GetVertexBuffer();
		ID3D11Buffer* indexBuffer = skyBoxModel.meshes[0].GetIndexBuffer();
		deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		int indexCount = skyBoxModel.meshes[0].GetIndices().size();
		skyShader.Render(deviceContext, indexCount, matrices, color, light, cam, fog, smrv);
		deviceContext->OMSetDepthStencilState(NULL, 0);
	}
	void Skybox::CreateSkybox(ID3D11Device* device, const char* pathName, 
	 map<const char*, vector<Mesh>> &loadedModels, XMFLOAT3 scale = XMFLOAT3(1.0f, 1.0f, 1.0f))
	{
		skyBoxModel = Model(pathName, "Skybox", loadedModels);
		skyBoxModel.SetBuffers(device);
		skyBoxModel.SetPosition(0, 0, 0);
		skyBoxModel.SetRotation(0, 0, 0);
		skyBoxModel.SetScale(scale.x, scale.y, scale.z);
	}
	XMFLOAT3 scale;
private:
	Model skyBoxModel;
	ID3D11ShaderResourceView* smrv;
	ID3D11DepthStencilState* DSLessEqual;
	ID3D11RasterizerState* RSCullNone;

};
#endif