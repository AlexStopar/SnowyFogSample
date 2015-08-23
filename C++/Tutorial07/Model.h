#ifndef MODEL_H
#define MODEL_H
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <vector>
#include <iostream>
#include <directxcolors.h>
#include "Mesh.h"
#include "btBulletDynamicsCommon.h"
using namespace std;

enum CollisionTypes{ NOTHING = 0, CO_GHOST = BIT(0), CO_PLAYER = BIT(1), CO_WORLD = BIT(2) };
int playerCollidesWith = CO_PLAYER | CO_WORLD;
int worldCollidesWith = CO_PLAYER | CO_GHOST;
class Model
{
public:
	string Name;
	bool isStatic = true;
	/*  Functions   */
	// Constructor, expects a filepath to a 3D model.
	Model(){}
	Model(const char* path, string name, map<const char*, vector<Mesh>> &loadedModels)
	{
		Name = name;
		if (loadedModels.count(path) <= 0)
		{
			this->loadModel(path);
			loadedModels.insert(pair<const char*,vector<Mesh>>(path, this->meshes));
		}
		else this->meshes = loadedModels.find(path)->second;
		
	}
	
	// Draws the model, and thus all its meshes

	vector<Mesh> meshes;
	void SetBuffers(ID3D11Device* device)
	{
		for (vector<Mesh>::iterator it = this->meshes.begin(); it != this->meshes.end(); ++it) {
			it->SetBuffers(device);
		}
	}
	void SetTexture(ID3D11Device* device, WCHAR* texFileName)
	{
		for (vector<Mesh>::iterator it = this->meshes.begin(); it != this->meshes.end(); ++it) {
			it->SetTexture(device, texFileName);
		}
	}
	//Each model has a position, rotation, and scale to initialize in the program
	void SetPosition(float x, float y, float z)
	{
		position = XMFLOAT3(x, y, z);
	}
	//Sets intialPosition to current starting position in time
	void SetInitPosition() { initialPosition = position; }
	void OffsetRigidBody(float x, float y, float z)
	{ 
		btVector3 origin = this->rigidBody->getWorldTransform().getOrigin();
		btVector3 newOrigin = btVector3(origin.getX() + x, origin.getY() + y, origin.getZ() + z);
		this->rigidBody->getWorldTransform().setOrigin(newOrigin);
	}
	void SetRotation(float x, float y, float z)
	{
		rotation = XMFLOAT3(x, y, z);
	}
	void SetScale(float x, float y, float z)
	{
		scale = XMFLOAT3(x, y, z);
	}
	//Set a shape for static boxes or falling spheres
	void SetBoxShape(float x, float y, float z)
	{
		btTriangleMesh* mesh = new btTriangleMesh();
		for (vector<Mesh>::iterator it = meshes.begin(); it != meshes.end(); ++it) {
			vector<SimpleVertex> vertices = it->GetVertices();
			vector<UINT> indices = it->GetIndices();
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
		}
		shape = new btBvhTriangleMeshShape(mesh, false);
		shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
	}
	void SetSphereShape(float radius)
	{
		shape = new btSphereShape((btScalar)radius);
	}
	//Calculate physical bodies based on shape
	void CalculateBoxRigidBody(btDiscreteDynamicsWorld* dynamicsWorld)
	{
		motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1)
			, btVector3((btScalar)position.x, (btScalar)position.y, (btScalar)position.z)));
		btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, motionState, shape, btVector3(0, 0, 0));
		rigidBody = new btRigidBody(groundRigidBodyCI);
		rigidBody->setRestitution(0.5);
		dynamicsWorld->addRigidBody(rigidBody, (short)CO_WORLD, (short)worldCollidesWith);
	}
	void CalculateFallRigidBody(btDiscreteDynamicsWorld* dynamicsWorld)
	{
		motionState = new btDefaultMotionState(
			btTransform(btQuaternion(0, 0, 0, 1), btVector3((btScalar)position.x, (btScalar)position.y, (btScalar)position.z)));
		btScalar mass = 1.0;
		btVector3 fallInertia(0, 1, 0);
		shape->calculateLocalInertia(mass, fallInertia);
		btRigidBody::btRigidBodyConstructionInfo
			fallRigidBodyCI(mass, motionState, shape, fallInertia);
		rigidBody = new btRigidBody(fallRigidBodyCI);
		//Necessary for rolling and bouncing
		rigidBody->setFriction(0.5);
		rigidBody->setDamping(0.04, 0.04);
		rigidBody->setRestitution(1.0);
		if (this->Name.substr(0, 4) != "Copy") dynamicsWorld->addRigidBody(rigidBody,
			(short)CO_PLAYER, (short)playerCollidesWith);
		else dynamicsWorld->addRigidBody(rigidBody,
			(short)CO_GHOST, (short)playerCollidesWith);
		
	}
	XMFLOAT3 GetPosition()
	{
		return position;
	}
	XMFLOAT3 GetRotation()
	{
		return rotation;
	}
	XMFLOAT3 GetScale()
	{
		return scale;
	}
	XMFLOAT3 GetInitPosition()
	{
		return initialPosition;
	}
	btRigidBody* GetRigidBody()
	{
		return rigidBody;
	}
	void SetRigidBody(btRigidBody* body)
	{
		rigidBody = body;
	}
private:
	/*  Model Data  */
	XMFLOAT3 position;
	XMFLOAT3 initialPosition; //This will be for restarting objects
	XMFLOAT3 rotation;
	XMFLOAT3 scale;
	string directory;
	btCollisionShape* shape; //Shape of physical object
	btDefaultMotionState* motionState; //Type of movement
	btRigidBody* rigidBody; //Physical body for movement
		// Stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.

	/*  Functions   */
	// Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
	void loadModel(string path)
	{
		// Read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
		// Check for errors
		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}
		// Retrieve the directory path of the filepath
		this->directory = path.substr(0, path.find_last_of('/'));

		// Process ASSIMP's root node recursively
		this->processNode(scene->mRootNode, scene);
	}

	// Processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode* node, const aiScene* scene)
	{
		// Process each mesh located at the current node
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			// The node object only contains indices to index the actual objects in the scene. 
			// The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			
			this->meshes.push_back(this->processMesh(mesh));
		}
		// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			this->processNode(node->mChildren[i], scene);
		}

	}

	Mesh processMesh(aiMesh* mesh)
	{
		// Data to fill
		vector<SimpleVertex> vertices;
		vector<UINT> indices;
		// Walk through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			SimpleVertex vertex;
			XMFLOAT3 vector; // We declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			// Positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Pos = vector;
			//Do the same for the Normals
			XMFLOAT3 normVector;
			normVector.x = mesh->mNormals[i].x;
			normVector.y = mesh->mNormals[i].y;
			normVector.z = mesh->mNormals[i].z;
			vertex.Norm = normVector;
			if (mesh->mTextureCoords[0]) // Does the mesh contain texture coordinates?
			{
				XMFLOAT2 tex = XMFLOAT2(mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y);
				// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vertex.Tex = tex;
			}
			else vertex.Tex = XMFLOAT2(0.0f, 0.0f);
			vertices.push_back(vertex);
		}
		// Now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// Retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back((unsigned int)face.mIndices[j]);
		}
		return Mesh(vertices, indices);
		
	}
	
};

#endif




