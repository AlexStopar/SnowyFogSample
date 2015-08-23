CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(recast_SRCS
	Source/Recast.cpp
	Source/RecastAlloc.cpp
	Source/RecastArea.cpp
	Source/RecastContour.cpp
	Source/RecastFilter.cpp
	Source/RecastLayers.cpp
	Source/RecastMesh.cpp
	Source/RecastMeshDetail.cpp
	Source/RecastRasterization.cpp
	Source/RecastRegion.cpp)

SET(recast_HDRS
	Include/Recast.h
	Include/RecastAlloc.h
	Include/RecastAssert.h)

IF(WIN32)
	SET(recast_SRCS ${recast_SRCS} ${recast_HDRS})
ENDIF(WIN32)

INCLUDE_DIRECTORIES(Include)

ADD_LIBRARY(Recast ${recast_SRCS})
