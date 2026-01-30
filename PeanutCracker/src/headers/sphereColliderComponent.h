#pragma once

#include <glm/glm.hpp>


class SphereColliderComponent {
public:
	glm::vec3 localCenter;
	glm::vec3 worldCenter;
	float	  localRadius;
	float	  worldRadius;

	SphereColliderComponent(const glm::vec3& i_center = glm::vec3(0.0f, 0.0f, 0.0f), float i_radius = 1.0f);
};