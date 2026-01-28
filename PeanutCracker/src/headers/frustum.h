#pragma once

#include <glm/glm.hpp>

struct BoundingSphere {
	glm::vec3 center;
	float	  radius;
};

class Frustum {
public:
	void constructFrustum(float aspect, const glm::mat4& projectionMat, const glm::mat4& viewMat);
	bool isInFrustum(const BoundingSphere& sphere) const;

private:
	struct Plane {
		glm::vec3 normal;
		float distance;

		float signedDistance(const glm::vec3& point) const {
			return glm::dot(normal, point) + distance;
		}
	};

	Plane planes[6];

	bool isOutsidePlane(const BoundingSphere& sphere, Plane plane) const;
};