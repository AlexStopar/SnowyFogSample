#ifndef PATHFINDER_H
#define PATHFINDER_H
#include "NavMesh.h"

int MAX_STEPS = 500;
int MAX_STRAIGHT_PATH = 3;
class Pathfinder
{
public:
	Pathfinder(){
		Path = new XMFLOAT3[500];
		Start = XMFLOAT3(0, 0, 0);
		End = XMFLOAT3(0, 0, 0);
		PathSize = 0;
	}

	Pathfinder(const Pathfinder&){}

	~Pathfinder(){}

	void SetStartPath(XMFLOAT3 start) { Start = start; }
	
	void SetEndPath(XMFLOAT3 end) { End = end; }
	
	XMFLOAT3 getNextPoint() {
		return Path[0];
	}
	void FindStraightPath(NavMesh& navMesh)
	{
		float spos[3];
		spos[0] = -1.0f * Start.y;
		spos[1] = Start.z;
		spos[2] = -1.0f * Start.x;

		//
		float epos[3];
		epos[0] = -1.0f * End.y;
		epos[1] = End.z;
		epos[2] = -1.0f * End.x;

		dtQueryFilter filter;
		filter.setIncludeFlags(0xffff);
		filter.setExcludeFlags(0);

		float polyPickExt[3];
		polyPickExt[0] = 2;
		polyPickExt[1] = 4;
		polyPickExt[2] = 2;

		dtPolyRef startRef;
		dtPolyRef endRef;
		dtNavMeshQuery* dNavQuery = navMesh.GetNavQuery();

		float nfs[3] = { 0, 0, 0 };
		float nfe[3] = { 0, 0, 0 };

		startRef = dNavQuery->findNearestPoly(spos, polyPickExt, &filter, &startRef, nfs);
		endRef = dNavQuery->findNearestPoly(epos, polyPickExt, &filter, &endRef, nfe);

		static const int MAX_POLYS = 256;
		dtPolyRef polys[MAX_POLYS];
		int* npolys = new int[MAX_POLYS];
		int npoly;
		float straightPath[MAX_POLYS * 3];
		unsigned char straightPathFlags[MAX_POLYS];
		dtPolyRef straightPathPolys[MAX_POLYS];
		int nstraightPath;
		int* nstraightPathCount = new int[MAX_POLYS];
		//
		int pos = 0;

		npoly = dNavQuery->findPath(startRef, endRef, spos, epos, &filter, polys, npolys, MAX_POLYS);
		nstraightPath = 0;
		if (npoly)
		{
			nstraightPath = dNavQuery->findStraightPath(spos, epos, polys, npoly, 
				straightPath, straightPathFlags, straightPathPolys, nstraightPathCount, MAX_POLYS);
			for (int i = 0; i < nstraightPath * 3;)
			{
				Path[pos].y = -1.0f * straightPath[i++];
				Path[pos].z = straightPath[i++];
				Path[pos].x = -1.0f * straightPath[i++];
				pos++;
			}
			// append the end point
			Path[pos].x = End.x;
			Path[pos].y = End.y;
			Path[pos].z = End.z;
			pos++;
		}
			
	}
private:
	XMFLOAT3 Start;
	XMFLOAT3 End;
	XMFLOAT3* Path;
	int PathSize;
};
#endif