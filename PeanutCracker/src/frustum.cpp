#include "headers/frustum.h"


void Frustum::constructFrustum(float aspect, const glm::mat4& projectionMat, const glm::mat4& viewMat) {
	glm::mat4 viewProjMat = projectionMat * viewMat;

	for (int i = 0; i < 3; i++) {
		float posA = viewProjMat[0][3] + viewProjMat[0][i];
		float posB = viewProjMat[1][3] + viewProjMat[1][i];
		float posC = viewProjMat[2][3] + viewProjMat[2][i];
		float posD = viewProjMat[3][3] + viewProjMat[3][i];
		float posLen = glm::length(glm::vec3(posA, posB, posC));
		planes[i * 2] = Plane{
			glm::vec3(posA, posB, posC) / posLen,
			posD / posLen
		};

		float negA = viewProjMat[0][3] - viewProjMat[0][i];
		float negB = viewProjMat[1][3] - viewProjMat[1][i];
		float negC = viewProjMat[2][3] - viewProjMat[2][i];
		float negD = viewProjMat[3][3] - viewProjMat[3][i];
		float negLen = glm::length(glm::vec3(negA, negB, negC));
		planes[i * 2 + 1] = Plane{
			glm::vec3(negA, negB, negC) / negLen,
			negD / negLen
		};
	}
}

bool Frustum::isInFrustum(const BoundingSphere& sphere) const {
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