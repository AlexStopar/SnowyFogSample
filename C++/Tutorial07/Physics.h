#include "btBulletDynamicsCommon.h"
#include <BulletCollision\Gimpact\btGImpactCollisionAlgorithm.h>

btDiscreteDynamicsWorld* dynamicsWorld;

class Physics
{
public:
	Physics::Physics()
	{
		broadphase = 0;
		collisionConfiguration = 0;
		dispatcher = 0;
		solver = 0;
		dynamicsWorld = 0;
	}
	Physics::Physics(const Physics&) {}
	~Physics() {}

	void SetupPhysics()
	{
		// Load the Physics
		btBroadphaseInterface* broadphase = new btDbvtBroadphase();
		btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
		btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
		//Setup collision algorithm and world
		btGImpactCollisionAlgorithm::registerAlgorithm(dispatcher);
		btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver();
		dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
		dynamicsWorld->setGravity(btVector3(0, -5, 0));
	}

	void ShutdownPhysics()
	{
		delete dynamicsWorld; 
		dynamicsWorld = 0;
		delete solver;
		solver = 0;
		delete dispatcher;
		dispatcher = 0;
		delete collisionConfiguration;
		collisionConfiguration = 0;
		delete broadphase;
		broadphase = 0;
	}
	btDiscreteDynamicsWorld* dynamicsWorld;
private:
	btBroadphaseInterface* broadphase;
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btSequentialImpulseConstraintSolver* solver;
	
};
