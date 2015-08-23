#define BIT(x) (1<<(x)) //For bitwise masking

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <vector>
#include <map>
#include <thread>
#include <directxcolors.h>
#include "Skybox.h"
#include "resource.h"
#include "Input.h"
#include "Camera.h"
#include "Model.h"
#include "Light.h"
#include "Terrain.h"
#include "Physics.h"
#include "Shader.h"
#include "Player.h"
#include "Copy.h"
#include "Enemy.h"
#include "ParticleSystem.h"
#include "NavMesh.h"
using namespace std;
using namespace DirectX;

Physics gamePhysics;
map<string, Model> models;
map<const char*, vector<Mesh>> loadedModels;
map<string, Model> seeThroughModels;

vector<Copy> copies;
vector<Enemy> enemies;
XMMATRIX g_World;
Camera camera;
Shader shader;
Shader hazeShader; //Apply that faint transparent haze to the ball for future stuff
Input input;
NavMesh navMesh;
Skybox skybox; //Skybox for the world to have something to look at
Terrain terrain; //Terrain for something that's not a flat box
ParticleSystem pSystem; //Dem particles for snow
MatrixBuffer matrices;
ColorBuffer color;
LightBuffer light;
CameraBuffer cam;
FogBuffer fog;
Light directionLight;
int copyNumber = 0;
int enemyNumber = 0; //Keeps track of unique enemies
int copyCount = 0; //Number of copies to ensure unique copy count
int MAX_COPIES = 1;
const float NEAR_PLANE = 1.0f;
const float FAR_PLANE = 1000.0f;
const float AREA_WIDTH = 256.0f;
const float AREA_HEIGHT = 256.0f;
const float FALLOFF_POINT = -200.0f; //Point of falling to ultimate DOOM
string CURRENT_ENEMY_COPY_NAME; //For adding a new copy
ULONGLONG timePrev = 0;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4                            g_vMeshColor(160.0f / 255.0f, 32.0f / 255.0f, 240.0f / 255.0f, 1.0f);
ID3D11Buffer*                       g_pVertexBuffer = nullptr;
ID3D11Buffer*                       g_pIndexBuffer = nullptr;
ID3D11ShaderResourceView*           g_pTextureRV = nullptr;
Player player = { XMFLOAT3(10.0f, 20.0f, 110.0f), 0.5f, 2.0f };

class Device
{
public:
	Device::Device() {}
	Device::Device(const Device&) {}
	~Device() {}

	void Device::RunNavThread()
	{
		navMesh = NavMesh(AREA_WIDTH + 20.0f, -AREA_WIDTH - 20.0f, AREA_HEIGHT + 20.0f, -AREA_HEIGHT - 20.0f, 30.0f, -30.0f);
		//Call setup for models
		SetupModel("../Tutorial07/farm.obj", L"farm.dds", "Farm", 125.0f, 0.0f, 125.0f, false,
			XMFLOAT3(10.0f, 20.0f, 20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(2.0f, 1.0f, 2.0f));
		terrain.InitTerrain(g_pd3dDevice, "../Tutorial07/heightMap.bmp", gamePhysics.dynamicsWorld);
		
		//Setup NavMesh before player
		navMesh.GenerateNavMap();
	}

	HRESULT Device::InitDevice(HWND g_hWnd, HINSTANCE g_hInst)
	{
		HRESULT hr = S_OK;

		RECT rc;
		GetClientRect(g_hWnd, &rc);
		UINT width = rc.right - rc.left;
		UINT height = rc.bottom - rc.top;

		UINT createDeviceFlags = 0;
#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_DRIVER_TYPE driverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};
		UINT numDriverTypes = ARRAYSIZE(driverTypes);

		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};
		UINT numFeatureLevels = ARRAYSIZE(featureLevels);

		for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
		{
			g_driverType = driverTypes[driverTypeIndex];
			hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
				D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

			if (hr == E_INVALIDARG)
			{
				// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
				hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
					D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
			}

			if (SUCCEEDED(hr))
				break;
		}
		if (FAILED(hr)) return hr;	

		models = map<string, Model>();
		loadedModels = map<const char*, vector<Mesh>>();
		enemies = vector<Enemy>();
		gamePhysics = Physics();
		//Setup them physics
		gamePhysics.SetupPhysics();
		terrain = Terrain();
		terrain.terShader.InitShader(g_pd3dDevice, g_hWnd, L"Terrain.fx", L"Terrain.fx");
		thread navThread = std::thread(&Device::RunNavThread, this);

		// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
		IDXGIFactory1* dxgiFactory = nullptr;
		{
			IDXGIDevice* dxgiDevice = nullptr;
			hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
			if (SUCCEEDED(hr))
			{
				IDXGIAdapter* adapter = nullptr;
				hr = dxgiDevice->GetAdapter(&adapter);
				if (SUCCEEDED(hr))
				{
					hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
					adapter->Release();
				}
				dxgiDevice->Release();
			}
		}
		if (FAILED(hr))
			return hr;

		// Create swap chain
		IDXGIFactory2* dxgiFactory2 = nullptr;
		hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
		if (dxgiFactory2)
		{
			// DirectX 11.1 or later
			hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
			if (SUCCEEDED(hr)) (void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1),
				reinterpret_cast<void**>(&g_pImmediateContext1));

			DXGI_SWAP_CHAIN_DESC1 sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.Width = width;
			sd.Height = height;
			sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.BufferCount = 1;

			hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
			if (SUCCEEDED(hr)) hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
			dxgiFactory2->Release();
		}
		else
		{
			// DirectX 11.0 systems
			DXGI_SWAP_CHAIN_DESC sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.BufferCount = 1;
			sd.BufferDesc.Width = width;
			sd.BufferDesc.Height = height;
			sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.BufferDesc.RefreshRate.Numerator = 60;
			sd.BufferDesc.RefreshRate.Denominator = 1;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.OutputWindow = g_hWnd;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.Windowed = TRUE;

			hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
		}

		// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut

		dxgiFactory->Release();

		if (FAILED(hr)) return hr;
			
		// Create a render target view
		ID3D11Texture2D* pBackBuffer = nullptr;
		hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
		if (FAILED(hr)) return hr;
			
		hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
		pBackBuffer->Release();
		if (FAILED(hr)) return hr;
		
		D3D11_RASTERIZER_DESC cmdesc;

		ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));
		cmdesc.FillMode = D3D11_FILL_SOLID;
		cmdesc.CullMode = D3D11_CULL_BACK;
		cmdesc.FrontCounterClockwise = true;
		hr = g_pd3dDevice->CreateRasterizerState(&cmdesc, &CCWcullMode);

		cmdesc.FrontCounterClockwise = false;

		hr = g_pd3dDevice->CreateRasterizerState(&cmdesc, &CWcullMode);

		///////////////**************new**************////////////////////
		cmdesc.CullMode = D3D11_CULL_NONE;
		hr = g_pd3dDevice->CreateRasterizerState(&cmdesc, &RSCullNone);
		// Create depth stencil texture
		D3D11_TEXTURE2D_DESC descDepth;
		ZeroMemory(&descDepth, sizeof(descDepth));
		descDepth.Width = width;
		descDepth.Height = height;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;
		hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
		if (FAILED(hr)) return hr;
			
		// Create the depth stencil view
		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = descDepth.Format;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;
		hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
		if (FAILED(hr)) return hr;
			
		g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

		// Setup the viewport
		D3D11_VIEWPORT vp;
		vp.Width = (FLOAT)width;
		vp.Height = (FLOAT)height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		g_pImmediateContext->RSSetViewports(1, &vp);
		

		//Initialize Input
		input = Input();
		if (!input.Initialize(g_hInst, g_hWnd)) return false;

		//Initialize various shaders
		shader = Shader();
		shader.InitShader(g_pd3dDevice, g_hWnd, L"Tutorial07.fx", L"Tutorial07.fx");
		hazeShader = Shader();
		hazeShader.InitShader(g_pd3dDevice, g_hWnd, L"PurpleHaze.fx", L"PurpleHaze.fx");
		
		//Load Assimp model into vertices
		
		SetupModel("../Tutorial07/sphere.obj", L"seafloor.dds", "Player",
			player.Position.x, player.Position.y, player.Position.z, true);
		SetupModel("../Tutorial07/sphere.obj", L"chaos.dds", "Enemy" + to_string(enemyNumber),
			101.0f, 30.0f, 50.0f, true);

		//Setup ParticleSystem
		pSystem = ParticleSystem();
		pSystem.snowShader.InitShader(g_pd3dDevice, g_hWnd, L"Snow.fx", L"Snow.fx");
		pSystem.Initialize(g_pd3dDevice);
		//Setup a SkyBox
		skybox = Skybox();
		skybox.CreateSkybox(g_pd3dDevice, "../Tutorial07/sphere.obj", loadedModels, XMFLOAT3(10.0f, 10.0f, 10.0f));
		skybox.skyShader.InitShader(g_pd3dDevice, g_hWnd, L"Skybox.fx", L"Skybox.fx");
		skybox.InitTexture(g_pd3dDevice);
		// Set primitive topology
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Initialize the world matrices
		g_World = XMMatrixIdentity();

		// Initialize the view matrix
		XMVECTOR Eye = XMVectorSet(player.Position.x, player.Position.y + 3.0f, player.Position.z - 6.0f, 0.0f);
		XMVECTOR At = XMVectorSet(player.Position.x, player.Position.y, player.Position.z, 0.0f);
		XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		camera = Camera(Eye, Up, At);
		camera.SetCamReference(0.0f, 3.0f, -10.0f);
		camera.SetSensitivity(0.8f);

		g_View = camera.GetViewMatrix();
		//Initialize Projection Matrix
		g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, NEAR_PLANE, FAR_PLANE);
		matrices.view = XMMatrixTranspose(g_View);
		matrices.world = XMMatrixTranspose(g_World);
		matrices.projection = XMMatrixTranspose(g_Projection);

		//Setup the lighting
		directionLight = Light();
		directionLight.SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);
		directionLight.SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
		directionLight.SetDirection(0.0f, -1.0f, 0.0f);
		directionLight.SetSpecularColor(0.0f, 0.0f, 0.0f, 1.0f);
		directionLight.SetSpecularPower(64.0f);


		light.ambientColor = directionLight.GetAmbientColor();
		light.diffuseColor = directionLight.GetDiffuseColor();
		light.lightDirection = directionLight.GetDirection();
		light.specularColor = directionLight.GetSpecularColor();
		light.specularPower = directionLight.GetSpecularPower();

		fog.fogNear = NEAR_PLANE;
		fog.fogFar = FAR_PLANE;
		fog.isFogOn = 1.0f;
		fog.fogPower = 10.0f;
		
		//Oh yeah, and that camera buffer padding
		cam.padding = 0;

		D3D11_BLEND_DESC BlendState;
		ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
		BlendState.RenderTarget[0].BlendEnable = TRUE;
		BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		BlendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendState.RenderTarget[0].RenderTargetWriteMask = 0x0f;
		g_pd3dDevice->CreateBlendState(&BlendState, &g_pBlendStateYesBlend);
		float blendFactor[4];
		// Setup the blend factor.
		blendFactor[0] = 0.0f;
		blendFactor[1] = 0.0f;
		blendFactor[2] = 0.0f;
		blendFactor[3] = 0.0f;

		// Turn on the alpha blending.
		g_pImmediateContext->OMSetBlendState(g_pBlendStateYesBlend, blendFactor, 0xffffffff);
		navThread.join();
		return S_OK;
	}
	void Device::CleanupDevice()
	{
		g_pSwapChain->SetFullscreenState(false, NULL);
		shader.Shutdown();
		terrain.Shutdown();
		hazeShader.Shutdown();
		input.Shutdown();
		gamePhysics.ShutdownPhysics();
		navMesh.ShutdownNavMap();
		skybox.Shutdown();
		if (g_pImmediateContext) g_pImmediateContext->ClearState();
		if (g_pTextureRV) g_pTextureRV->Release();
		if (g_pBlendStateYesBlend) g_pBlendStateYesBlend->Release();
		if (g_pVertexBuffer) g_pVertexBuffer->Release();
		if (g_pIndexBuffer) g_pIndexBuffer->Release();
		if (g_pDepthStencil) g_pDepthStencil->Release();
		if (g_pDepthStencilView) g_pDepthStencilView->Release();
		if (g_pRenderTargetView) g_pRenderTargetView->Release();
		if (g_pSwapChain1) g_pSwapChain1->Release();
		if (g_pSwapChain) g_pSwapChain->Release();
		if (g_pImmediateContext1) g_pImmediateContext1->Release();
		if (g_pImmediateContext) g_pImmediateContext->Release();
		if (g_pd3dDevice1) g_pd3dDevice1->Release();
		if (g_pd3dDevice) g_pd3dDevice->Release();
	}
	void Device::Render(HWND g_hWnd)
	{
		//Setup deltaTime for framed movement
		static float t = 0.0f;
		static float deltaT = 0.0f;
		if (g_driverType == D3D_DRIVER_TYPE_REFERENCE) t += (float)XM_PI * 0.0125f;
		else
		{
			static ULONGLONG timeStart = 0;
			ULONGLONG timeCur = GetTickCount64();
			if (timeStart == 0)
				timeStart = timeCur;
			t = (timeCur - timeStart) / 1000.0f;
			deltaT = (float)(timeCur - timePrev);
			timePrev = timeCur;
		}
		//Get input for each render call
		XMFLOAT3 pushDirection = input.Frame(player, camera, g_hWnd, deltaT);
		if (input.IsMakingCopy() && copyCount < MAX_COPIES)
		{
			for (vector<Enemy>::iterator it = enemies.begin(); it != enemies.end(); ++it) {
				CURRENT_ENEMY_COPY_NAME = it->GetName();
				XMFLOAT3 enemyPos = models.find(it->GetName())->second.GetPosition();
				SetupModel("../Tutorial07/sphere.obj", L"chaos.dds", "Copy" + to_string(copyNumber),
					enemyPos.x, enemyPos.y, enemyPos.z, true);
				copyNumber++;
				copyCount++;
			}
			
		}
		//Do necessary frame calculations for particle system, then render
		
		//Get buffer setup for matrices to be called into shader
		MatrixBuffer matrixBuffer;
		XMMATRIX worldMatrix, viewMatrix;
		// Update our time
		
		//Kill our copies if it's their time
		for (vector<Copy>::iterator it = copies.begin(); it != copies.end(); ++it) {
			if (!it->IsDead)
			{
				it->TimeToDie += deltaT;
				it->Move(seeThroughModels.find(it->GetName())->second, player, deltaT, navMesh);
			}
			if (it->TimeToDie > MAX_COPY_LIFETIME && !it->IsDead)
			{
				seeThroughModels.erase(it->GetName());
				it->IsDead = true;
				copyCount--;
			}
		}
		//Move our enemies
		for (vector<Enemy>::iterator it = enemies.begin(); it != enemies.end(); ++it) {
			it->Move(models.find(it->GetName())->second, player, deltaT, navMesh);
		}
		//Initialize worldMatrix to identity
		worldMatrix = matrices.world;
		// Modify the color 
		color.vMeshColor = g_vMeshColor;
		//Set camera position
		cam.cameraPosition = XMFLOAT3(XMVectorGetX(camera.Eye), XMVectorGetY(camera.Eye), XMVectorGetZ(camera.Eye));
		// Clear the back buffer
		g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::Gray);

		// Clear the depth buffer to 1.0 (max depth)
		g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		//Simulate a step forward in gameplay
		gamePhysics.dynamicsWorld->stepSimulation(t);

		g_pImmediateContext->RSSetState(RSCullNone);
		XMMATRIX Scale = XMMatrixScaling(skybox.scale.x, skybox.scale.y, skybox.scale.z);
		//Make sure the sphere is always centered around camera
		XMMATRIX Translation = XMMatrixTranslation(XMVectorGetX(camera.Eye), XMVectorGetY(camera.Eye), XMVectorGetZ(camera.Eye));

		//Set sphereWorld's world space using the transformations
		worldMatrix = Scale * Translation;
		viewMatrix = camera.GetViewMatrix();
		matrixBuffer = { XMMatrixTranspose(worldMatrix), XMMatrixTranspose(viewMatrix), matrices.projection };
		skybox.Render(g_pImmediateContext, matrixBuffer, color, light, cam, fog);
		g_pImmediateContext->RSSetState(CWcullMode);

		worldMatrix = XMMatrixTranslation(player.Position.x, player.Position.y + 1.0f, player.Position.z);
		matrixBuffer = { XMMatrixTranspose(worldMatrix), XMMatrixTranspose(viewMatrix), matrices.projection }; 
		pSystem.Frame(deltaT, g_pImmediateContext);
		pSystem.Render(g_pImmediateContext, matrices, color, light, cam, fog);

		worldMatrix = XMMatrixIdentity();
		matrixBuffer = { XMMatrixTranspose(worldMatrix), XMMatrixTranspose(viewMatrix), matrices.projection };
		//Render all opaque, then all seethrough models
		terrain.Render(g_pImmediateContext, matrixBuffer, color, light, cam, fog);
		RenderModels(models, pushDirection, matrixBuffer, worldMatrix, viewMatrix);
		if(seeThroughModels.size() > 0) 
			RenderModels(seeThroughModels, pushDirection, matrixBuffer, worldMatrix, viewMatrix);

		//Set camera based on possible collision
		camera.ModifyCameraForCollision(gamePhysics.dynamicsWorld, player.Position);
		//Look at slightly above player
		camera.At = XMVectorSet(player.Position.x, player.Position.y + 1.0f, player.Position.z, 0.0f);

		// Present our back buffer to our front buffer
		g_pSwapChain->Present(0, 0);
	}
private: 
	
	void Device::RenderModels(map<string, Model> &renderModels, XMFLOAT3 &pushDirection, 
		MatrixBuffer &matrixBuffer, XMMATRIX &worldMatrix, XMMATRIX &viewMatrix)
	{
		for (map<string, Model>::iterator it = renderModels.begin(); it != renderModels.end(); ++it) {
			if (!it->second.isStatic && it->second.GetPosition().y < FALLOFF_POINT)
			{
				XMFLOAT3 initPos = it->second.GetInitPosition();
				it->second.SetPosition(initPos.x, initPos.y, initPos.z);
				it->second.GetRigidBody()->getWorldTransform().setOrigin(btVector3(initPos.x, initPos.y, initPos.z));
				it->second.GetRigidBody()->setLinearVelocity(btVector3(0, 0, 0));
				it->second.GetRigidBody()->setAngularVelocity(btVector3(0, 0, 0));
				if (it->second.Name == "Player") player.Position = initPos;
			}
			//Translate via applied rigidbody forces
			if (it->second.Name == "Player")
			{
				it->second.GetRigidBody()->activate();
				it->second.GetRigidBody()->applyCentralForce
					(btVector3((btScalar)pushDirection.x, (btScalar)pushDirection.y, (btScalar)pushDirection.z));
			}
			btVector3 rigidPos = it->second.GetRigidBody()->getWorldTransform().getOrigin();

			XMFLOAT3 rotation = it->second.GetRotation();
			if (it->second.isStatic)
			{
				//Modify rotation of static objects
				btTransform trans = it->second.GetRigidBody()->getWorldTransform();
				btQuaternion quat = btQuaternion(rotation.x, rotation.y, rotation.z);
				trans.setRotation(quat);
				it->second.GetRigidBody()->setWorldTransform(trans);
			}
			btQuaternion rigidRot = it->second.GetRigidBody()->getWorldTransform().getRotation();
			//Set Camera Position in relation to Player position
			if (it->second.Name == "Player")
			{
				player.Position = XMFLOAT3((float)rigidPos[0], (float)rigidPos[1], (float)rigidPos[2]);
				it->second.SetPosition(player.Position.x, player.Position.y, player.Position.z);
				//Account for camera not seeing past objects

			}
			else it->second.SetPosition((float)rigidPos[0], (float)rigidPos[1], (float)rigidPos[2]);
			XMFLOAT3 position = it->second.GetPosition();
			XMFLOAT3 scale = it->second.GetScale();
			//Set world Matrix for use in vertex shader
			worldMatrix = XMMatrixMultiply(XMMatrixScaling(scale.x, scale.y, scale.z),
				XMMatrixRotationQuaternion(XMVectorSet((float)rigidRot[0], (float)rigidRot[1], (float)rigidRot[2], (float)rigidRot[3])));
			worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixTranslation(position.x, position.y, position.z));
			viewMatrix = camera.GetViewMatrix();
			matrixBuffer = { XMMatrixTranspose(worldMatrix), XMMatrixTranspose(viewMatrix), matrices.projection };
			for (vector<Mesh>::iterator it2 = it->second.meshes.begin(); it2 != it->second.meshes.end(); ++it2) {
				//Set vertexBuffer and indexBuffer for each mesh object and draw
				UINT stride = sizeof(SimpleVertex);
				UINT offset = 0;
				ID3D11Buffer* vertexBuffer = it2->GetVertexBuffer();
				ID3D11Buffer* indexBuffer = it2->GetIndexBuffer();
				g_pImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
				g_pImmediateContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
				int indexCount = it2->GetIndices().size();
				ID3D11ShaderResourceView* texture = it2->GetTexture();
				if (it->second.Name.substr(0, 4) != "Copy") shader.Render(g_pImmediateContext, indexCount, 
					matrixBuffer, color, light, cam, fog, texture);
				else hazeShader.Render(g_pImmediateContext, indexCount, matrixBuffer, color, light, cam, fog, texture);
			}
		}
	}
	void Device::SetupModel(const char* pathName, WCHAR* textureName, string modelName,
		float x, float y, float z, bool isSphere, XMFLOAT3 extents = XMFLOAT3(0, 0, 0), 
		XMFLOAT3 rotation = XMFLOAT3(0, 0, 0), XMFLOAT3 scale = XMFLOAT3(1.0f, 1.0f, 1.0f))
	{
		Model model = Model(pathName, modelName, loadedModels);

		//Set buffers of each mesh in models

		model.SetBuffers(g_pd3dDevice);
		model.SetTexture(g_pd3dDevice, textureName);
		//Set transform settings of model in world
		model.SetPosition(x, y, z);
		model.SetInitPosition();
		model.SetRotation(rotation.x, rotation.y, rotation.z);
		model.SetScale(scale.x, scale.y, scale.z);
		//Create rigidbody based on sphere/plane shape
		if (!isSphere) {
			model.SetBoxShape(extents.x, extents.y, extents.z);
			model.CalculateBoxRigidBody(gamePhysics.dynamicsWorld);
			navMesh.LoadVerticesFromModel(model, scale.x, scale.y, scale.z);
			navMesh.LoadTrianglesFromModel(model);
			
		}
		else
		{
			model.isStatic = false;
			model.SetSphereShape(1.0f);
			model.CalculateFallRigidBody(gamePhysics.dynamicsWorld);
			if (model.Name.substr(0, 5) == "Enemy") enemies.push_back(Enemy(model.Name, 1.0f));
		}
		if (modelName.substr(0, 4) == "Copy")
		{		
			seeThroughModels.insert(pair<string, Model>(model.Name, model)); //Ghosts are see-through
			bool hasChanged = false;
			if(copies.size() < (unsigned int)MAX_COPIES) copies.push_back(Copy(model.Name, 0, false, 2.0f, CURRENT_ENEMY_COPY_NAME));
			else
			{
				for (vector<Copy>::iterator it = copies.begin(); it != copies.end(); ++it) {
					if (hasChanged) break;
					if (it->IsDead)
					{
						it->SetName(model.Name);
						it->TimeToDie = 0;
						it->IsDead = false;
						it->EnemyName = CURRENT_ENEMY_COPY_NAME;
						hasChanged = true;
					}
			}
		}
		}
		else models.insert(pair<string,Model>(model.Name, model));
	}
	D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
	ID3D11Device*                       g_pd3dDevice = nullptr;
	ID3D11Device1*                      g_pd3dDevice1 = nullptr;
	ID3D11DeviceContext*                g_pImmediateContext = nullptr;
	ID3D11DeviceContext1*               g_pImmediateContext1 = nullptr;
	IDXGISwapChain*                     g_pSwapChain = nullptr;
	IDXGISwapChain1*                    g_pSwapChain1 = nullptr;
	ID3D11RenderTargetView*             g_pRenderTargetView = nullptr;
	ID3D11Texture2D*                    g_pDepthStencil = nullptr;
	ID3D11DepthStencilView*             g_pDepthStencilView = nullptr;
	ID3D11BlendState* g_pBlendStateYesBlend = nullptr;
	ID3D11RasterizerState* CCWcullMode;
	ID3D11RasterizerState* CWcullMode;
	ID3D11RasterizerState* RSCullNone;
};