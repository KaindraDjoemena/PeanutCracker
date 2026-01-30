#pragma once

#include <glm/glm.hpp>
#include "object.h"


class MouseRay {
public:
	bool		hit;
	float		dist;			// distance
	glm::vec3	origin;			// mouse origin
	glm::vec3	direction;		// direction vector

	float calcRayDist(Object* object);
};