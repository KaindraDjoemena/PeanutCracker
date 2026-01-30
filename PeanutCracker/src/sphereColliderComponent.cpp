#include "headers/sphereColliderComponent.h"

#include <glm/glm.hpp>


SphereColliderComponent::SphereColliderComponent(const glm::vec3& i_center, float i_radius)
	: localCenter(i_center), worldCenter(i_center), localRadius(i_radius), worldRadius(i_radius) {
}
