#ifndef TRANSFORM_H
#define TRANSFORM_H

#define GLM_ENABLE_EXPERIMENTAL 

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>


struct Transform {
	glm::vec3 position		= glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 scale			= glm::vec3(1.0f, 1.0f, 1.0f);
	glm::quat quatRotation	= glm::vec3(0.0f, 0.0f, 0.0f);

	inline glm::mat4 getModelMatrix() {
		// Model Space Matrix = T x R x S
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat  = glm::translate(modelMat, position);
		modelMat *= glm::mat4_cast(quatRotation);
		modelMat  = glm::scale(modelMat, scale);

		return modelMat;
	}
};

#endif