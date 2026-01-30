#include "headers/ray.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "headers/object.h"


float MouseRay::calcRayDist(Object* object) {
	float globalEntry = -INFINITY;
	float globalExit = INFINITY;

	// --X Planes
	float inverseDirX = 1.0f / direction.x;
	float dist1X = (object->modelPtr->aabb.min.x - origin.x) * inverseDirX;
	float dist2X = (object->modelPtr->aabb.max.x - origin.x) * inverseDirX;

	float distEntryX = std::min(dist1X, dist2X);
	float distExitX = std::max(dist1X, dist2X);

	globalEntry = std::max(globalEntry, distEntryX);
	globalExit = std::min(globalExit, distExitX);

	// --Y Planes
	float inverseDirY = 1.0f / direction.y;
	float dist1Y = (object->modelPtr->aabb.min.y - origin.y) * inverseDirY;
	float dist2Y = (object->modelPtr->aabb.max.y - origin.y) * inverseDirY;

	float distEntryY = std::min(dist1Y, dist2Y);
	float distExitY = std::max(dist1Y, dist2Y);

	globalEntry = std::max(globalEntry, distEntryY);
	globalExit = std::min(globalExit, distExitY);

	// -- Z Planes
	float inverseDirZ = 1.0f / direction.z;
	float dist1Z = (object->modelPtr->aabb.min.z - origin.z) * inverseDirZ;
	float dist2Z = (object->modelPtr->aabb.max.z - origin.z) * inverseDirZ;

	float distEntryZ = std::min(dist1Z, dist2Z);
	float distExitZ = std::max(dist1Z, dist2Z);

	globalEntry = std::max(globalEntry, distEntryZ);
	globalExit = std::min(globalExit, distExitZ);


	if (globalExit >= globalEntry && globalExit > 0) {
		hit = true;
		dist = (globalEntry < 0) ? 0.0f : globalEntry;
	}
	else {
		hit = false;
		dist = INFINITY;
	}

	return dist;
}