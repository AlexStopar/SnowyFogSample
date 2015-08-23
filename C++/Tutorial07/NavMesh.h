#ifndef NAVMESH_H
#define NAVMESH_H
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <vector>
#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
using namespace std;
using namespace DirectX;



class NavMesh
{
public:
	NavMesh(){}
	NavMesh(float zmax, float zmin, float xmax, float xmin, float ymax, float ymin) {
		bMin[0] = xmin;
		bMin[1] = ymin;
		bMin[2] = zmin;
		bMax[0] = xmax;
		bMax[1] = ymax;
		bMax[2] = zmax;
		vCap = 0;
		tCap = 0;
		tris = 0;
		verts = 0;
		triAreas = 0;
		vertexCount = 0;
		triCount = 0;
		hField = 0;
		context = new rcContext(true);
		config = rcConfig();
		chField = 0;
		cSet = 0;
		pMesh = 0;
		dMesh = 0;
		dNavMesh = 0;
		dNavQuery = 0;
	}
	NavMesh(const NavMesh&)
	{
	}
	~NavMesh() {}
	void GenerateNavMap()
	{
		//
		// Step 1. Initialize build config.
		//
		memset(&config, 0, sizeof(config));
		config.cs = 1.0f;
		config.ch = 1.0f;
		config.walkableSlopeAngle = 10;
		config.walkableHeight = (int)ceilf(1.0f / config.ch);
		config.walkableClimb = (int)floorf(1.0f / config.ch);
		config.walkableRadius = (int)ceilf(1.0f / config.cs);
		config.maxEdgeLen = (int)(10.0f / config.cs);
		config.maxSimplificationError = 0.1f;
		config.minRegionArea = (int)rcSqr(50.0f);		// Note: area = size*size
		config.mergeRegionArea = (int)rcSqr(50.0f);	// Note: area = size*size
		config.maxVertsPerPoly = 3;
		config.detailSampleDist = 1.0f < 0.9f ? 0 : config.cs * 1.0f;
		config.detailSampleMaxError = config.ch * 1.0f;

		// Set the area where the navigation will be build.
		// Here the bounds of the input mesh are used, but the
		// area could be specified by an user defined box, etc.
		rcVcopy(config.bmin, bMin);
		rcVcopy(config.bmax, bMax);
		rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

		//
		// Step 2. Rasterize input polygon soup.
		//
		hField = rcAllocHeightfield();
		if (!hField) return;
		if (!rcCreateHeightfield(context, *hField, config.width,
			config.height, config.bmin, config.bmax, config.cs, config.ch)) return;
		// Allocate array that can hold triangle area types.
		triAreas = new unsigned char[triCount];
		if (!triAreas) return;
		// Find triangles which are walkable based on their slope and rasterize them.
		rcMarkWalkableTriangles(context, config.walkableSlopeAngle, verts, vertexCount, tris, triCount, triAreas);
		rcRasterizeTriangles(context, verts, vertexCount, tris, triAreas, triCount, *hField, config.walkableClimb);

		//
		// Step 3. Filter walkables surfaces.
		//
		// Once all geoemtry is rasterized, we do initial pass of filtering to
		// remove unwanted overhangs caused by the conservative rasterization
		// as well as filter spans where the character cannot possibly stand.
		rcFilterLowHangingWalkableObstacles(context, config.walkableClimb, *hField);
		rcFilterLedgeSpans(context, config.walkableHeight, config.walkableClimb, *hField);
		rcFilterWalkableLowHeightSpans(context, config.walkableHeight, *hField);
		//
		// Step 4. Partition walkable surface to simple regions.
		//

		// Compact the heightfield so that it is faster to handle from now on.
		// This will result more cache coherent data as well as the neighbours
		// between walkable cells will be calculated.
		chField = rcAllocCompactHeightfield();
		if (!chField) return;
		if (!rcBuildCompactHeightfield(context, config.walkableHeight, config.walkableClimb, *hField, *chField)) return;
		// Erode the walkable area by agent radius.
		if (!rcErodeWalkableArea(context, config.walkableRadius, *chField)) return;

		// Prepare for region partitioning, by calculating distance field along the walkable surface.
		if (!rcBuildDistanceField(context, *chField)) return;

		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildRegions(context, *chField, 0, config.minRegionArea, config.mergeRegionArea)) return;	

		//
		// Step 5. Trace and simplify region contours.
		//
		cSet = rcAllocContourSet();
		if (!cSet) return;
		if (!rcBuildContours(context, *chField, config.maxSimplificationError, config.maxEdgeLen, *cSet)) return;

		//
		// Step 6. Build polygons mesh from contours.
		//

		// Build polygon navmesh from the contours.
		pMesh = rcAllocPolyMesh();
		if (!pMesh) return;
		if (!rcBuildPolyMesh(context, *cSet, config.maxVertsPerPoly, *pMesh)) return;

		//
		// Step 7. Create detail mesh which allows to access approximate height on each polygon.
		//

		dMesh = rcAllocPolyMeshDetail();
		if (!dMesh) return;
		if (!rcBuildPolyMeshDetail(context, *pMesh, *chField, config.detailSampleDist, config.detailSampleMaxError, *dMesh)) return;

		//
		// (Optional) Step 8. Create Detour data from Recast poly mesh.
		//
		unsigned char* navData = 0;
		int navDataSize = 0;

		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = pMesh->verts;
		params.vertCount = pMesh->nverts;
		params.polys = pMesh->polys;
		params.polyAreas = pMesh->areas;
		params.polyFlags = pMesh->flags;
		params.polyCount = pMesh->npolys;
		params.nvp = pMesh->nvp;
		params.detailMeshes = dMesh->meshes;
		params.detailVerts = dMesh->verts;
		params.detailVertsCount = dMesh->nverts;
		params.detailTris = dMesh->tris;
		params.detailTriCount = dMesh->ntris;
		params.walkableHeight = 1.0f;
		params.walkableRadius = 1.0f;
		params.walkableClimb = 2.0f;
		params.offMeshConVerts = 0;
		params.offMeshConRad = 0;
		params.offMeshConDir = 0;
		params.offMeshConAreas = 0;
		params.offMeshConFlags = 0;
		params.offMeshConUserID = 0;
		params.offMeshConCount = 0;
		rcVcopy(params.bmin, pMesh->bmin);
		rcVcopy(params.bmax, pMesh->bmax);
		params.cs = config.cs;
		params.ch = config.ch;
		params.buildBvTree = true;
		
		if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) return;

		dNavMesh = dtAllocNavMesh();
		if (!dNavMesh) return;
		dNavQuery = dtAllocNavMeshQuery();
		if (!dNavQuery) return;
		dtStatus status;

		status = dNavMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status)) return;

		status = dNavQuery->init(dNavMesh, 2048);
		if (dtStatusFailed(status)) return;
		
	}
	void LoadVerticesFromModel(Model model, float xScale, float yScale, float zScale)
	{
		for (vector<Mesh>::iterator it = model.meshes.begin(); it != model.meshes.end(); ++it) {
			vector<SimpleVertex> vertices = it->GetVertices();
			for (vector<SimpleVertex>::iterator it2 = vertices.begin(); it2 != vertices.end(); ++it2)
			{			
				if (vertexCount + 1 > vCap)
				{
					vCap = !vCap ? 8 : vCap * 2;
					float* nv = new float[vCap * 3];
					if (vertexCount)
						memcpy(nv, verts, vertexCount * 3 * sizeof(float));
					delete[] verts;
					verts = nv;
				}
				float* dst = &verts[vertexCount * 3];
				*dst++ = it2->Pos.x*xScale;
				*dst++ = it2->Pos.y*yScale;
				*dst++ = it2->Pos.z*zScale;
				vertexCount++;
			}
		}
	}
	void LoadTrianglesFromModel(Model model)
	{
		for (vector<Mesh>::iterator it = model.meshes.begin(); it != model.meshes.end(); ++it) {
			vector<UINT> indices = it->GetIndices();
			int indexCounter = 0;
			if (triCount + 1 > tCap)
			{
				tCap = !tCap ? 8 : tCap * 2;
				int* nv = new int[tCap * 3];
				if (triCount)
					memcpy(nv, tris, triCount * 3 * sizeof(int));
				delete[] tris;
				tris = nv;
			}
			int* dst = &tris[triCount * 3];
			for (vector<UINT>::iterator it2 = indices.begin(); it2 != indices.end(); ++it2)
			{
				
				*dst++ = *it2;
				indexCounter++;
				if (indexCounter >= 3)
				{
					triCount++;
					if (triCount + 1 > tCap)
					{
						tCap = !tCap ? 8 : tCap * 2;
						int* nv = new int[tCap * 3];
						if (triCount)
							memcpy(nv, tris, triCount * 3 * sizeof(int));
						delete[] tris;
						tris = nv;
					}
					indexCounter = 0;
					dst = &tris[triCount * 3];
				}
			}
		}
	}
	inline const float* getMeshBoundsMin() const { return bMin; }
	inline const float* getMeshBoundsMax() const { return bMax; }

	void ShutdownNavMap()
	{
		delete[] verts;
		verts = 0;
		delete[] tris;
		tris = 0;
		delete[] triAreas;
		triAreas = 0;
		delete hField;
		hField = 0;
		delete context;
		context = 0;
		delete chField;
		chField = 0;
		delete cSet;
		cSet = 0;
		delete pMesh;
		pMesh = 0;
		delete dMesh;
		dMesh = 0;
		delete dNavMesh;
		dNavMesh = 0;
		delete dNavQuery;
		dNavQuery = 0;
	}
	dtNavMesh* GetNavMesh()
	{
		return dNavMesh;
	}
	dtNavMeshQuery* GetNavQuery()
	{
		return dNavQuery;
	}
private:
	float bMin[3];
	float bMax[3];
	float* verts;
	int* tris;
	unsigned char* triAreas;
	int vertexCount;
	int triCount;
	int vCap; //Used for memory allocation
	int tCap; //Used for memory allocation
	rcConfig config; //configuration settings for navmesh
	rcHeightfield* hField; //Set of heights we're working with
	rcContext* context; //Context for build
	rcCompactHeightfield* chField;
	rcContourSet* cSet;
	rcPolyMesh* pMesh;
	rcPolyMeshDetail* dMesh;
	dtNavMesh* dNavMesh;
	dtNavMeshQuery* dNavQuery;
};

#endif