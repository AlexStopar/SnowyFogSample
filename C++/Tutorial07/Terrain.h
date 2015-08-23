#ifndef TERRAIN_H
#define TERRAIN_H

#include <map>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include "Model.h"
#include "Shader.h"
#include "DDSTextureLoader.h"
#include "BulletCollision\CollisionShapes\btHeightfieldTerrainShape.h"
const int TEXTURE_REPEAT = 4;
using namespace DirectX;
using namespace std;

class Terrain
{
private:

	struct HeightMapType
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

	struct VectorType
	{
		float x, y, z;
	};

public:
	Terrain()
	{
		terShader = Shader();
		texture = nullptr;
		heightMap = nullptr;
	}

	Shader terShader;
	bool Terrain::InitTerrain(ID3D11Device* device, char* heightMapFilename, btDiscreteDynamicsWorld* dynamicsWorld)
	{
		LoadTexture(device, L"snow.dds");
		LoadHeightMap(heightMapFilename);
		NormalizeHeightMap();
		CalculateNormals();
		CalculateTextureCoordinates();
		InitBuffers(device);
		
		btTriangleMesh* mesh = new btTriangleMesh();
		
		int i = 0;
		while (i < indices.size())
		{
			btVector3 p1 = btVector3(vertices[indices[i]].Pos.x, vertices[indices[i]].Pos.y, vertices[indices[i]].Pos.z);
			i++;
			btVector3 p2 = btVector3(vertices[indices[i]].Pos.x, vertices[indices[i]].Pos.y, vertices[indices[i]].Pos.z);
			i++;
			btVector3 p3 = btVector3(vertices[indices[i]].Pos.x, vertices[indices[i]].Pos.y, vertices[indices[i]].Pos.z);
			i++;
			mesh->addTriangle(p1, p2, p3);
		}
		
		shape = new btBvhTriangleMeshShape(mesh, false);
		//shape = new btHeightfieldTerrainShape(terrainWidth, terrainHeight, heights, 1.0f, 0.0f, 10.0f, 1, PHY_FLOAT, false);
		
		motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1)
			, btVector3(0, 0, 0)));
		btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, motionState, shape, btVector3(0, 0, 0));
		rigidBody = new btRigidBody(groundRigidBodyCI);
		rigidBody->setRestitution(0.5);
		dynamicsWorld->addRigidBody(rigidBody, (short)CO_WORLD, (short)worldCollidesWith);
		return S_OK;
	}
	void Terrain::Shutdown()
	{
		terShader.Shutdown();
		texture->Release();
	}
	void Terrain::Render(ID3D11DeviceContext* deviceContext, MatrixBuffer &matrices, ColorBuffer &color, LightBuffer &light, CameraBuffer &cam,
		FogBuffer &fog)
	{
		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		int indexCount = indices.size();
		terShader.Render(deviceContext, indexCount, matrices, color, light, cam, fog, texture);
	}
	ID3D11ShaderResourceView* Terrain::GetTexture()
	{
		return texture;
	}

private:

	btCollisionShape* shape; //Shape of physical object
	btDefaultMotionState* motionState; //Type of movement
	btRigidBody* rigidBody;
	bool Terrain::LoadHeightMap(char* filename)
	{
		FILE* filePtr;
		int error;
		unsigned int count;
		BITMAPFILEHEADER bitmapFileHeader;
		BITMAPINFOHEADER bitmapInfoHeader;
		int imageSize, i, j, k, index;
		unsigned char* bitmapImage;
		unsigned char height;


		// Open the height map file in binary.
		error = fopen_s(&filePtr, filename, "rb");
		if (error != 0) return false;
		
		// Read in the file header.
		count = fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);
		if (count != 1) return false;
		
		// Read in the bitmap info header.
		count = fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);
		if (count != 1) return false;
		
		// Save the dimensions of the terrain.
		terrainWidth = bitmapInfoHeader.biWidth;
		terrainHeight = bitmapInfoHeader.biHeight;

		// Calculate the size of the bitmap image data.
		imageSize = terrainWidth * terrainHeight * 3;

		// Allocate memory for the bitmap image data.
		bitmapImage = new unsigned char[imageSize];
		if (!bitmapImage) return false;
		
		// Move to the beginning of the bitmap data.
		fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

		// Read in the bitmap image data.
		count = fread(bitmapImage, 1, imageSize, filePtr);
		if (count != imageSize) return false;
		
		// Close the file.
		error = fclose(filePtr);
		if (error != 0) return false;
		
		// Create the structure to hold the height map data.
		heightMap = new HeightMapType[terrainWidth * terrainHeight];
		heights = new float[terrainWidth * terrainHeight];
		if (!heightMap) return false;
		if (!heights) return false;
	
		// Initialize the position in the image data buffer.
		k = 0;

		// Read the image data into the height map.
		for (j = 0; j<terrainHeight; j++)
		{
			for (i = 0; i<terrainWidth; i++)
			{
				height = bitmapImage[k];

				index = (terrainHeight * j) + i;

				heightMap[index].x = (float)i;
				heightMap[index].y = (float)height;
				heights[index] = (float)height;
				heightMap[index].z = (float)j;

				k += 3;
			}
		}

		// Release the bitmap image data.
		delete[] bitmapImage;
		bitmapImage = 0;
		return S_OK;
	}
	void Terrain::NormalizeHeightMap()
	{
		int i, j;

		for (j = 0; j<terrainHeight; j++)
		{
			for (i = 0; i<terrainWidth; i++)
			{
				heightMap[(terrainHeight * j) + i].y /= 10.0f;
				heights[(terrainHeight * j) + i] /= 10.0f;
			}
		}
	}
	bool Terrain::CalculateNormals()
	{
		int i, j, index1, index2, index3, index, count;
		float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
		VectorType* normals;


		// Create a temporary array to hold the un-normalized normal vectors.
		normals = new VectorType[(terrainHeight - 1) * (terrainWidth - 1)];
		if (!normals) return false;
		// Go through all the faces in the mesh and calculate their normals.
		for (j = 0; j<(terrainHeight - 1); j++)
		{
			for (i = 0; i<(terrainWidth - 1); i++)
			{
				index1 = (j * terrainHeight) + i;
				index2 = (j * terrainHeight) + (i + 1);
				index3 = ((j + 1) * terrainHeight) + i;

				// Get three vertices from the face.
				vertex1[0] = heightMap[index1].x;
				vertex1[1] = heightMap[index1].y;
				vertex1[2] = heightMap[index1].z;

				vertex2[0] = heightMap[index2].x;
				vertex2[1] = heightMap[index2].y;
				vertex2[2] = heightMap[index2].z;

				vertex3[0] = heightMap[index3].x;
				vertex3[1] = heightMap[index3].y;
				vertex3[2] = heightMap[index3].z;

				// Calculate the two vectors for this face.
				vector1[0] = vertex1[0] - vertex3[0];
				vector1[1] = vertex1[1] - vertex3[1];
				vector1[2] = vertex1[2] - vertex3[2];
				vector2[0] = vertex3[0] - vertex2[0];
				vector2[1] = vertex3[1] - vertex2[1];
				vector2[2] = vertex3[2] - vertex2[2];

				index = (j * (terrainHeight - 1)) + i;

				// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
				normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
				normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
				normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
			}
		}

		// Now go through all the vertices and take an average of each face normal 	
		// that the vertex touches to get the averaged normal for that vertex.
		for (j = 0; j<terrainHeight; j++)
		{
			for (i = 0; i<terrainWidth; i++)
			{
				// Initialize the sum.
				sum[0] = 0.0f;
				sum[1] = 0.0f;
				sum[2] = 0.0f;

				// Initialize the count.
				count = 0;

				// Bottom left face.
				if (((i - 1) >= 0) && ((j - 1) >= 0))
				{
					index = ((j - 1) * (terrainHeight - 1)) + (i - 1);

					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Bottom right face.
				if ((i < (terrainWidth - 1)) && ((j - 1) >= 0))
				{
					index = ((j - 1) * (terrainHeight - 1)) + i;

					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Upper left face.
				if (((i - 1) >= 0) && (j < (terrainHeight - 1)))
				{
					index = (j * (terrainHeight - 1)) + (i - 1);

					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Upper right face.
				if ((i < (terrainWidth - 1)) && (j < (terrainHeight - 1)))
				{
					index = (j * (terrainHeight - 1)) + i;

					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Take the average of the faces touching this vertex.
				sum[0] = (sum[0] / (float)count);
				sum[1] = (sum[1] / (float)count);
				sum[2] = (sum[2] / (float)count);

				// Calculate the length of this normal.
				length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));

				// Get an index to the vertex location in the height map array.
				index = (j * terrainHeight) + i;

				// Normalize the final shared normal for this vertex and store it in the height map array.
				heightMap[index].nx = (sum[0] / length);
				heightMap[index].ny = (sum[1] / length);
				heightMap[index].nz = (sum[2] / length);
			}
		}

		// Release the temporary normals.
		delete[] normals;
		normals = 0;

		return S_OK;
	}
	void Terrain::ShutdownHeightMap()
	{
		delete[] heightMap;
		heightMap = 0;
	}

	void Terrain::CalculateTextureCoordinates()
	{
		int incrementCount, i, j, tuCount, tvCount;
		float incrementValue, tuCoordinate, tvCoordinate;
		
		// Calculate how much to increment the texture coordinates by.
		incrementValue = (float)TEXTURE_REPEAT / (float)terrainWidth;

		// Calculate how many times to repeat the texture.
		incrementCount = terrainWidth / TEXTURE_REPEAT;

		// Initialize the tu and tv coordinate values.
		tuCoordinate = 0.0f;
		tvCoordinate = 1.0f;

		// Initialize the tu and tv coordinate indexes.
		tuCount = 0;
		tvCount = 0;

		// Loop through the entire height map and calculate the tu and tv texture coordinates for each vertex.
		for (j = 0; j<terrainHeight; j++)
		{
			for (i = 0; i<terrainWidth; i++)
			{
				// Store the texture coordinate in the height map.
				heightMap[(terrainHeight * j) + i].tu = tuCoordinate;
				heightMap[(terrainHeight * j) + i].tv = tvCoordinate;

				// Increment the tu texture coordinate by the increment value and increment the index by one.
				tuCoordinate += incrementValue;
				tuCount++;

				// Check if at the far right end of the texture and if so then start at the beginning again.
				if (tuCount == incrementCount)
				{
					tuCoordinate = 0.0f;
					tuCount = 0;
				}
			}

			// Increment the tv texture coordinate by the increment value and increment the index by one.
			tvCoordinate -= incrementValue;
			tvCount++;

			// Check if at the top of the texture and if so then start at the bottom again.
			if (tvCount == incrementCount)
			{
				tvCoordinate = 1.0f;
				tvCount = 0;
			}
		}
	}
	bool Terrain::LoadTexture(ID3D11Device* device, WCHAR* path)
	{
		HRESULT hr = CreateDDSTextureFromFile(device, path, nullptr, &texture);
		if (FAILED(hr)) return !!hr;
		return S_OK;
	}
	bool Terrain::InitBuffers(ID3D11Device* device)
	{
		int index, i, j;
		int index1, index2, index3, index4;
		float tu, tv;
		SimpleVertex vertex;
		index = 0;
		for (j = 0; j<(terrainHeight - 1); j++)
		{
			for (i = 0; i<(terrainWidth - 1); i++)
			{
				index1 = (terrainHeight * j) + i;          // Bottom left.
				index2 = (terrainHeight * j) + (i + 1);      // Bottom right.
				index3 = (terrainHeight * (j + 1)) + i;      // Upper left.
				index4 = (terrainHeight * (j + 1)) + (i + 1);  // Upper right.

				// Upper left.
				tv = heightMap[index3].tv;

				// Modify the texture coordinates to cover the top edge.
				if (tv == 1.0f) { tv = 0.0f; }
				vertex.Pos = XMFLOAT3(heightMap[index3].x, heightMap[index3].y, heightMap[index3].z);
				vertex.Tex = XMFLOAT2(heightMap[index3].tu, tv);
				vertex.Norm = XMFLOAT3(heightMap[index3].nx, heightMap[index3].ny, heightMap[index3].nz);
				vertices.push_back(vertex);
				indices.push_back(index);
				index++;

				// Upper right.
				tu = heightMap[index4].tu;
				tv = heightMap[index4].tv;

				// Modify the texture coordinates to cover the top and right edge.
				if (tu == 0.0f) { tu = 1.0f; }
				if (tv == 1.0f) { tv = 0.0f; }

				vertex.Pos = XMFLOAT3(heightMap[index4].x, heightMap[index4].y, heightMap[index4].z);
				vertex.Tex = XMFLOAT2(tu, tv);
				vertex.Norm = XMFLOAT3(heightMap[index4].nx, heightMap[index4].ny, heightMap[index4].nz);
				vertices.push_back(vertex);
				indices.push_back(index);
				index++;

				// Bottom left.
				vertex.Pos = XMFLOAT3(heightMap[index1].x, heightMap[index1].y, heightMap[index1].z);
				vertex.Tex = XMFLOAT2(heightMap[index1].tu, heightMap[index1].tv);
				vertex.Norm = XMFLOAT3(heightMap[index1].nx, heightMap[index1].ny, heightMap[index1].nz);
				vertices.push_back(vertex);
				indices.push_back(index);
				index++;

				// Bottom left.
				vertex.Pos = XMFLOAT3(heightMap[index1].x, heightMap[index1].y, heightMap[index1].z);
				vertex.Tex = XMFLOAT2(heightMap[index1].tu, heightMap[index1].tv);
				vertex.Norm = XMFLOAT3(heightMap[index1].nx, heightMap[index1].ny, heightMap[index1].nz);
				vertices.push_back(vertex);
				indices.push_back(index);
				index++;

				// Upper right.
				tu = heightMap[index4].tu;
				tv = heightMap[index4].tv;

				// Modify the texture coordinates to cover the top and right edge.
				if (tu == 0.0f) { tu = 1.0f; }
				if (tv == 1.0f) { tv = 0.0f; }

				vertex.Pos = XMFLOAT3(heightMap[index4].x, heightMap[index4].y, heightMap[index4].z);
				vertex.Tex = XMFLOAT2(tu, tv);
				vertex.Norm = XMFLOAT3(heightMap[index4].nx, heightMap[index4].ny, heightMap[index4].nz);
				vertices.push_back(vertex);
				indices.push_back(index);
				index++;

				// Bottom right.
				tu = heightMap[index2].tu;

				// Modify the texture coordinates to cover the right edge.
				if (tu == 0.0f) { tu = 1.0f; }

				vertex.Pos = XMFLOAT3(heightMap[index2].x, heightMap[index2].y, heightMap[index2].z);
				vertex.Tex = XMFLOAT2(tu, heightMap[index2].tv);
				vertex.Norm = XMFLOAT3(heightMap[index2].nx, heightMap[index2].ny, heightMap[index2].nz);
				vertices.push_back(vertex);
				indices.push_back(index);
				index++;
			}
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
		return S_OK;

	}

private:
	int terrainWidth, terrainHeight;
	int vertexCount, indexCount;
	vector<SimpleVertex> vertices;
	vector<UINT> indices;
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	HeightMapType* heightMap;
	float* heights;
	ID3D11ShaderResourceView* texture;
};
#endif