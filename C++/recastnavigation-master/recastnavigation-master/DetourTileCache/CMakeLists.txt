CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(recast_SRCS
	Source/DetourTileCache.cpp
	Source/DetourTileCacheBuilder.cpp)

SET(recast_HDRS
	Include/DetourTileCache.h
	Include/DetourTileCacheBuilder.h)

IF(WIN32)
	SET(recast_SRCS ${recast_SRCS} ${recast_HDRS})
ENDIF(WIN32)

INCLUDE_DIRECTORIES(Include)

ADD_LIBRARY(DetourTileCache ${recast_SRCS})