#ifndef RAY_H
#define RAY_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "object.h"

struct MouseRay {
	bool		hit;
	float		dist;			// distance
	glm::vec3	origin;			// mouse origin
	glm::vec3	direction;		// direction vector
};

float calcRayDist(MouseRay& mouseRay, Object* object) {
	float globalEntry = -INFINITY;
	float globalExit = INFINITY;

	// --X Planes
	float inverseDirX = 1.0f / mouseRay.direction.x;
	float dist1X = (object->modelPtr->aabb.min.x - mouseRay.origin.x) * inverseDirX;
	float dist2X = (object->modelPtr->aabb.max.x - mouseRay.origin.x) * inverseDirX;

	float distEntryX = std::min(dist1X, dist2X);
	float distExitX  = std::max(dist1X, dist2X);

	globalEntry = std::max(globalEntry, distEntryX);
	globalExit = std::min(globalExit, distExitX);

	// --Y Planes
	float inverseDirY = 1.0f / mouseRay.direction.y;
	float dist1Y = (object->modelPtr->aabb.min.y - mouseRay.origin.y) * inverseDirY;
	float dist2Y = (object->modelPtr->aabb.max.y - mouseRay.origin.y) * inverseDirY;

	float distEntryY = std::min(dist1Y, dist2Y);
	float distExitY  = std::max(dist1Y, dist2Y);

	globalEntry = std::max(globalEntry, distEntryY);
	globalExit = std::min(globalExit, distExitY);

	// -- Z Planes
	float inverseDirZ = 1.0f / mouseRay.direction.z;
	float dist1Z = (object->modelPtr->aabb.min.z - mouseRay.origin.z) * inverseDirZ;
	float dist2Z = (object->modelPtr->aabb.max.z - mouseRay.origin.z) * inverseDirZ;

	float distEntryZ = std::min(dist1Z, dist2Z);
	float distExitZ  = std::max(dist1Z, dist2Z);

	globalEntry = std::max(globalEntry, distEntryZ);
	globalExit  = std::min(globalExit, distExitZ);


	if (globalExit >= globalEntry && globalExit > 0) {
		mouseRay.hit = true;
		mouseRay.dist = (globalEntry < 0) ? 0.0f : globalEntry;
	}
	else {
		mouseRay.hit = false;
		mouseRay.dist = INFINITY;
	}

	return mouseRay.dist;
}

#endif