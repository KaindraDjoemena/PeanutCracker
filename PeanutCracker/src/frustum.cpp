#include "headers/frustum.h"

void Frustum::constructFrustum(float aspect, const glm::mat4& projectionMat, const glm::mat4& viewMat) {
	glm::mat4 viewProjMat = projectionMat * viewMat;

	for (int i = 0; i < 3; i++) {
		float posA = viewProjMat[3][0] + viewProjMat[i][0];
		float posB = viewProjMat[3][1] + viewProjMat[i][1];
		float posC = viewProjMat[3][2] + viewProjMat[i][2];
		float posD = viewProjMat[3][3] + viewProjMat[i][3];
		glm::vec3 posN = glm::normalize(glm::vec3(posA, posB, posC));

		float negA = viewProjMat[3][0] - viewProjMat[i][0];
		float negB = viewProjMat[3][1] - viewProjMat[i][1];
		float negC = viewProjMat[3][2] - viewProjMat[i][2];
		float negD = viewProjMat[3][3] - viewProjMat[i][3];
		glm::vec3 negN = glm::normalize(glm::vec3(negA, negB, negC));


		planes[i]	  = Plane{ posN, posD };
		planes[i + 1] = Plane{ negN, posD };
	}
}

bool Frustum::isInFrustum(const BoundingSphere& sphere) const {
	// Iterate over planes
	for (int i = 0; i < 6; i++) {
		if (isOutsidePlane(sphere, planes[i])) {
			return false;
		}
	}

	return true;
}

bool Frustum::isOutsidePlane(const BoundingSphere& sphere, Plane plane) const {
	float distance = plane.signedDistance(sphere.center);
	
	if (distance < -sphere.radius) { return true; }
	return false;
}