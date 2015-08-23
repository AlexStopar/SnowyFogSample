#ifndef ENEMY_H
#define ENEMY_H
#include "Model.h"
#include "Pathfinder.h"
#include <Recast.h>

class Enemy
{
public:
	Enemy() {}
	Enemy(string name, float speed)
	{
		this->Name = name;
		this->Speed = speed;
		pFinder = 0;
		pFinder = new Pathfinder();
	}
	

	void Move(Model& model, Player& player, float deltaTime, NavMesh& navMesh)
	{
		//Setup a path for the enemy to follow
		pFinder->SetStartPath(model.GetPosition());
		pFinder->SetEndPath(player.Position);
		pFinder->FindStraightPath(navMesh);
		XMFLOAT3 nextPosition = pFinder->getNextPoint();
		//Determine direction to player and normalize, then multiply by speed and deltaTime
		XMFLOAT3 stalkDirection = XMFLOAT3(nextPosition.x - model.GetPosition().x,
			nextPosition.y - model.GetPosition().y, nextPosition.z - model.GetPosition().z);
		XMVECTOR stalkVector = XMVectorSet(stalkDirection.x, stalkDirection.y, stalkDirection.z, 0);
		stalkVector = XMVector3Normalize(stalkVector);
		btVector3 pushDirection = btVector3(XMVectorGetX(stalkVector) * this->Speed * deltaTime, XMVectorGetY(stalkVector) * this->Speed * deltaTime,
			XMVectorGetZ(stalkVector) * this->Speed * deltaTime);
		//Apply force to current Enemy rigidbody
		model.GetRigidBody()->applyCentralForce(pushDirection);
	}
	string GetName() { return this->Name; }
	void SetName(string name) { this->Name = name; }
	void Shutdown()
	{
		delete pFinder;
		pFinder = 0;
	}
private:
	string Name;
	float Speed;
	XMFLOAT3 Velocity;
	Pathfinder* pFinder;
};
#endif