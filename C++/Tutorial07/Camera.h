#ifndef CAMERA_H
#define CAMERA_H

#include <windows.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
using namespace DirectX;

//For wherever the camera is facing
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values
const float YAW = 0.0f;
const float PITCH = 0.0f;
const float SPEED = 3.0f;
const float SENSITIVTY = 0.05f;
const float ZOOM = 45.0f;
const float COLLISION_CORRECTION = 0.1f; //How much to correct the camera when colliding with walls to reduce visual artifacts



// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// Camera Attributes
	XMFLOAT3 camReference; //Location of camera in reference to player
	XMVECTOR Eye; //Vectors used to make camera matrix
	XMVECTOR  At;
	XMVECTOR  Up;
	XMVECTOR  Right;
	XMVECTOR  WorldUp;
	// Eular Angles
	float Yaw;
	float Pitch;
	// Camera options
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;
	float Radius; //Radius of 3rd person camera
	float Sensitivity; //Sensitivity of rotation
	
	
	// Constructor with vectors
	Camera(XMVECTOR  position = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVECTOR up =
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMVECTOR at = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)) 
	{
		this->Eye = position;
		this->Up = up;
		this->At = at;
	}
	// Constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float atX, float atY, float atZ, float yaw, float pitch) 
	{
		this->Eye = XMVectorSet(posX, posY, posZ, 0.0f);
		this->Up = XMVectorSet(upX, upY, upZ, 0.0f);
		this->At = XMVectorSet(atX, atY, atZ, 0.0f);
		yaw = 0;
		pitch = 0;
	}

	// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
	XMMATRIX GetViewMatrix()
	{
		return XMMatrixLookAtLH(this->Eye, this->At, this->Up);
	}

	//Sets the camera reference to player location
	void SetCamReference(float x, float y, float z)
	{
		camReference = XMFLOAT3(x, y, z);
		Radius = z;
	}
	//Sets rotation sensitivity
	void SetSensitivity(float sensitivity)
	{
		this->Sensitivity = sensitivity;
	}

	void SetCameraRotation(float yoffset, float xoffset)
	{
		this->Pitch += yoffset * this->Sensitivity;
		this->Yaw += xoffset * this->Sensitivity;
		if (this->Pitch > 45.0f)
			this->Pitch = 45.0f;
		if (this->Pitch < -45.0f)
			this->Pitch = -45.0f;
		
		this->camReference.x = this->Radius * (float)sin((long double)XMConvertToRadians(this->Yaw));
		this->camReference.z = this->Radius * (float)cos((long double)XMConvertToRadians(this->Yaw));
		this->camReference.y = this->Radius * (float)sin((long double)XMConvertToRadians(this->Pitch - 20.0f));
	}

	//Sets camera location with regards to occlusion
	void ModifyCameraForCollision(btDiscreteDynamicsWorld* dynamicsWorld, XMFLOAT3 playerPos)
	{
		btVector3 btPlayerPos = btVector3(playerPos.x, playerPos.y, playerPos.z);
		this->Eye = XMVectorSet(playerPos.x + this->camReference.x,
			playerPos.y + this->camReference.y, playerPos.z + this->camReference.z, 0.0f);
		XMFLOAT3 cameraPos = XMFLOAT3(XMVectorGetX(this->Eye), XMVectorGetY(this->Eye), XMVectorGetZ(this->Eye));
		btVector3 btCameraPos = btVector3(cameraPos.x, cameraPos.y, cameraPos.z);
		//Create a ray between the player and usual camera position
		btCollisionWorld::ClosestRayResultCallback RayCallback(btPlayerPos, btCameraPos);
//		RayCallback.m_collisionFilterMask = 0xFFFF;
		dynamicsWorld->rayTest(btPlayerPos, btCameraPos, RayCallback);
		if (RayCallback.hasHit())
		{
			btVector3 direction = btCameraPos - btPlayerPos;
			direction.normalize();
			//Correct it slightly to place it in wall and reduce visual artifacts
			btVector3 hitPoint = RayCallback.m_hitPointWorld + (COLLISION_CORRECTION * direction);
			this->Eye = XMVectorSet(hitPoint.getX(), hitPoint.getY(), hitPoint.getZ(), 0.0f);
		}
	}
	
};

#endif