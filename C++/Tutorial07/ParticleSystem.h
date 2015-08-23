#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include "DDSTextureLoader.h"
#include "Mesh.h"
#include "Shader.h"
using namespace DirectX;
using namespace std;

struct ParticleType
{
	XMFLOAT3 Pos;
	float Velocity;
	bool IsActive;
};
class ParticleSystem
{
public:
	ParticleSystem() 
	{
		snowTex = nullptr;
		particleList = 0;
		vertices = vector<SimpleVertex>();
		indices = vector<UINT>();
		indexBuffer = 0;
		vertexBuffer = 0;
	}
	ParticleSystem(const ParticleSystem&) {}
	~ParticleSystem() {}
	bool ParticleSystem::Initialize(ID3D11Device* device)
	{
		LoadTexture(device, L"snowflake.dds");
		InitParticleSystem();
		InitBuffers(device);
		return S_OK;
	}
	void ParticleSystem::Shutdown()
	{
		snowShader.Shutdown();
		snowTex->Release();
		ShutdownBuffers();
	}
	bool ParticleSystem::Frame(float deltaT, ID3D11DeviceContext* deviceContext)
	{
		KillParticles();
		// Emit new particles.
		EmitParticles(deltaT);
		// Update the position of the particles.
		UpdateParticles(deltaT);
		// Update the dynamic vertex buffer with the new position of each particle.
		UpdateBuffers(deviceContext);
		return S_OK;
	}
	void ParticleSystem::Render(ID3D11DeviceContext* deviceContext, MatrixBuffer &matrices, ColorBuffer &color, LightBuffer &light,
		CameraBuffer &cam, FogBuffer &fog)
	{
		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		int indexCount = indices.size();
		snowShader.Render(deviceContext, indices.size(), matrices, color, light, cam, fog, snowTex);
	}

	ID3D11ShaderResourceView* ParticleSystem::GetTexture() { return snowTex; }
	int ParticleSystem::GetIndexCount() { return indices.size(); }

	Shader snowShader;
private:
	bool ParticleSystem::LoadTexture(ID3D11Device* device, WCHAR* path)
	{
		HRESULT hr = CreateDDSTextureFromFile(device, path, nullptr, &snowTex);
		if (FAILED(hr)) return !!hr;
		return true;
	}

	bool ParticleSystem::InitParticleSystem()
	{
		int i;
		// Set the random deviation of where the particles can be located when emitted.
		particleDeviation = XMFLOAT3(0.5f, 0.1f, 2.0f);
		// Set the speed and speed variation of particles.
		particleVelocity = 1.0f;
		particleVelocityVariation = 0.2f;

		// Set the physical size of the particles.
		particleSize = 0.2f;

		// Set the number of particles to emit per second.
		particlesPerSecond = 2.0f;

		// Set the maximum number of particles allowed in the particle system.
		maxParticles = 50;

		particleList = new ParticleType[maxParticles];
		if (!particleList) return false;

		for (i = 0; i< maxParticles; i++)
		{
			particleList[i].IsActive = false;
		}
		// Initialize the current particle count to zero since none are emitted yet.
		currentParticleCount = 0;
		// Clear the initial accumulated time for the particle per second emission rate.
		accumulatedTime = 0.0f;
		return S_OK;
	}
	void ParticleSystem::ShutdownParticleSystem()
	{
		if (particleList)
		{
			delete[] particleList;
			particleList = 0;
		}
	}

	bool ParticleSystem::InitBuffers(ID3D11Device* device)
	{
		int i;
		D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
		D3D11_SUBRESOURCE_DATA vertexData, indexData;
		HRESULT result;

		vertexCount = maxParticles * 6;
		// Set the maximum number of indices in the index array.
		indexCount = vertexCount;

		for (i = 0; i<vertexCount; i++)
		{
			vertices.push_back({ XMFLOAT3(0, 0, 0), XMFLOAT2(0, 0), XMFLOAT3(0, 0, 0) });
		}
		// Initialize the index array.
		for (i = 0; i<indexCount; i++)
		{
			indices.push_back(i);
		}

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
		bd.ByteWidth = sizeof(UINT)* this->indices.size();
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = &this->indices[0];
		hr = device->CreateBuffer(&bd, &InitData, &this->indexBuffer);
		if (FAILED(hr))
			return !!hr;

		return true;
	}
	void ParticleSystem::ShutdownBuffers()
	{
		if (indexBuffer)
		{
			indexBuffer->Release();
			indexBuffer = 0;
		}

		// Release the vertex buffer.
		if (vertexBuffer)
		{
			vertexBuffer->Release();
			vertexBuffer = 0;
		}
	}

	void ParticleSystem::EmitParticles(float deltaT)
	{
		bool emitParticle, found;
		float positionX, positionY, positionZ, velocity, red, green, blue;
		int index, i, j;

		// Increment the frame time.
		accumulatedTime += deltaT;

		// Set emit particle to false for now.
		emitParticle = false;

		// Check if it is time to emit a new particle or not.
		if (accumulatedTime > (1000.0f / particlesPerSecond))
		{
			accumulatedTime = 0.0f;
			emitParticle = true;
		}

		// If there are particles to emit then emit one per frame.
		if ((emitParticle == true) && (currentParticleCount < (maxParticles - 1)))
		{
			currentParticleCount++;

			// Now generate the randomized particle properties.
			positionX = (((float)rand() - (float)rand()) / RAND_MAX) * particleDeviation.x;
			positionY = (((float)rand() - (float)rand()) / RAND_MAX) * particleDeviation.y;
			positionZ = (((float)rand() - (float)rand()) / RAND_MAX) * particleDeviation.z;

			velocity = particleVelocity + (((float)rand() - (float)rand()) / RAND_MAX) * particleVelocityVariation;

			// Now since the particles need to be rendered from back to front for blending we have to sort the particle array.
			// We will sort using Z depth so we need to find where in the list the particle should be inserted.
			index = 0;
			found = false;
			while (!found)
			{
				if ((particleList[index].IsActive == false) || (particleList[index].Pos.z < positionZ))
					found = true;
				else index++;
			}

			// Now that we know the location to insert into we need to copy the array over by one position from the index to make room for the new particle.
			i = currentParticleCount;
			j = i - 1;

			while (i != index)
			{
				particleList[i].Pos.x = particleList[j].Pos.x;
				particleList[i].Pos.y = particleList[j].Pos.y;
				particleList[i].Pos.z = particleList[j].Pos.z;
				
				particleList[i].Velocity = particleList[j].Velocity;
				particleList[i].IsActive = particleList[j].IsActive;
				i--;
				j--;
			}

			// Now insert it into the particle array in the correct depth order.
			particleList[index].Pos.x = positionX;
			particleList[index].Pos.y = positionY;
			particleList[index].Pos.z = positionZ;
			particleList[index].Velocity = velocity;
			particleList[index].IsActive = true;
		}

	}
	void ParticleSystem::UpdateParticles(float deltaT)
	{
		int i;
		// Each frame we update all the particles by making them move downwards using their position, velocity, and the frame time.
		for (i = 0; i< currentParticleCount; i++)
		{
			particleList[i].Pos.y -= (particleList[i].Velocity * deltaT * 0.001f);
		}
	}
	void ParticleSystem::KillParticles()
	{
		int i, j;


		// Kill all the particles that have gone below a certain height range.
		for (i = 0; i<maxParticles; i++)
		{
			if ((particleList[i].IsActive == true) && (particleList[i].Pos.y < -3.0f))
			{
				particleList[i].IsActive = false;
				currentParticleCount--;

				// Now shift all the live particles back up the array to erase the destroyed particle and keep the array sorted correctly.
				for (j = i; j< maxParticles - 1; j++)
				{
					particleList[j].Pos = particleList[j + 1].Pos;					
					particleList[j].Velocity = particleList[j + 1].Velocity;
					particleList[j].IsActive = particleList[j + 1].IsActive;
				}
			}
		}
	}

	bool ParticleSystem::UpdateBuffers(ID3D11DeviceContext* deviceContext)
	{
		int index, i;
		HRESULT result;
		D3D11_MAPPED_SUBRESOURCE mappedResource;	
		SimpleVertex* verticesPtr;

		// Now build the vertex array from the particle list array.  Each particle is a quad made out of two triangles.
		index = 0;
		vertices.clear();
		for (i = 0; i<vertexCount; i++)
		{
			vertices.push_back({ XMFLOAT3(0, 0, 0), XMFLOAT2(0, 0), XMFLOAT3(0, 0, 0) });
		}
		for (i = 0; i< currentParticleCount; i++)
		{
			// Bottom left.
			vertices[index].Pos = XMFLOAT3(particleList[i].Pos.x - particleSize, 
				particleList[i].Pos.y - particleSize, particleList[i].Pos.z);
			vertices[index].Tex = XMFLOAT2(0.0f, 1.0f);
			index++;

			// Top left.
			vertices[index].Pos = XMFLOAT3(particleList[i].Pos.x - particleSize,
				particleList[i].Pos.y + particleSize, particleList[i].Pos.z);
			vertices[index].Tex = XMFLOAT2(0.0f, 0.0f);
			index++;

			// Bottom right.
			vertices[index].Pos = XMFLOAT3(particleList[i].Pos.x + particleSize,
				particleList[i].Pos.y - particleSize, particleList[i].Pos.z);
			vertices[index].Tex = XMFLOAT2(1.0f, 1.0f);
			index++;

			// Bottom right.
			vertices[index].Pos = XMFLOAT3(particleList[i].Pos.x + particleSize,
				particleList[i].Pos.y - particleSize, particleList[i].Pos.z);
			vertices[index].Tex = XMFLOAT2(1.0f, 1.0f);
			index++;

			// Top left.
			vertices[index].Pos = XMFLOAT3(particleList[i].Pos.x - particleSize,
				particleList[i].Pos.y + particleSize, particleList[i].Pos.z);
			vertices[index].Tex = XMFLOAT2(0.0f, 0.0f);
			index++;

			// Top right.
			vertices[index].Pos = XMFLOAT3(particleList[i].Pos.x + particleSize,
				particleList[i].Pos.y + particleSize, particleList[i].Pos.z);
			vertices[index].Tex = XMFLOAT2(1.0f, 0.0f);
			
			index++;
		}

		// Lock the vertex buffer.
		result = deviceContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result)) return false;
		
		// Get a pointer to the data in the vertex buffer.
		verticesPtr = (SimpleVertex*)mappedResource.pData;

		// Copy the data into the vertex buffer.
		memcpy(verticesPtr, (void*)&vertices[0], (sizeof(SimpleVertex)* vertexCount));

		// Unlock the vertex buffer.
		deviceContext->Unmap(vertexBuffer, 0);
	}

	XMFLOAT3 particleDeviation;
	float particleVelocity, particleVelocityVariation;
	float particleSize, particlesPerSecond;
	int maxParticles;
	int vertexCount, indexCount;
	int currentParticleCount;
	float accumulatedTime;
	ID3D11ShaderResourceView* snowTex;
	ParticleType* particleList;
	vector<SimpleVertex> vertices;
	vector<UINT> indices;
	ID3D11Buffer *vertexBuffer, *indexBuffer;
};
#endif