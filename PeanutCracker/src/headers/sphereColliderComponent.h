#ifndef SPHERE_COLLIDER_COMPONENT_H
#define SPHERE_COLLIDER_COMPONENT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class SphereColliderComponent {
public:
	glm::vec3 localCenter;
	glm::vec3 worldCenter;
	float	  localRadius;
	float	  worldRadius;

	SphereColliderComponent(const glm::vec3& i_center = glm::vec3(0.0f, 0.0f, 0.0f),
							float i_radius = 1.0f)
		: localCenter(i_center), worldCenter(i_center), localRadius(i_radius), worldRadius(i_radius) {}
};

#endif