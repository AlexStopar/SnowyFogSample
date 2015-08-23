CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(recast_SRCS
	Source/DetourCrowd.cpp
	Source/DetourLocalBoundary.cpp
	Source/DetourObstacleAvoidance.cpp
	Source/DetourPathCorridor.cpp
	Source/DetourPathQueue.cpp
	Source/DetourProximityGrid.cpp)

SET(recast_HDRS
	Include/DetourCrowd.h
	Include/DetourLocalBoundary.h
	Include/DetourObstacleAvoidance.h
	Include/DetourPathCorridor.h
	Include/DetourPathQueue.h
	Include/DetourProximityGrid.h
)

IF(WIN32)
	SET(recast_SRCS ${recast_SRCS} ${recast_HDRS})
ENDIF(WIN32)

INCLUDE_DIRECTORIES(Include)

ADD_LIBRARY(DetourCrowd ${recast_SRCS})
