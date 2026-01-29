#ifndef OBJECT_H
#define OBJECT_H

#define GLM_ENABLE_EXPERIMENTAL 

#include "shader.h"
#include "light.h"
#include "model.h"
#include "transform.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <string>
#include <memory>
#include <unordered_map>

#include <imgui.h>
#include <imgui_impl_glfw.h>


const float epsilon = 0.00001;

enum Primitive_Type {
	CUBE,
	CYLINDER,
	PILL,
	SPHERE,
	PYRAMID,
	MODEL
};

// TODO: ADD OBJECT PRIMITIVES
class Object {
public:
	Model*		modelPtr;
	Shader*		shaderPtr;
	Transform	transform;
	bool		isSelected = false;

	glm::mat4	modelMatrixCache;
	glm::mat4	normalMatrixCache;

	Object(Model* i_modelPtr, Shader* i_shaderPtr)
		: modelPtr(i_modelPtr)
		, shaderPtr(i_shaderPtr)
		, transform() {}

	void setPosition(const glm::vec3& pos) {
		transform.position = pos;
	}
	void setScale(const glm::vec3& scl) {
		transform.scale.x = scl.x < epsilon ? epsilon : scl.x;
		transform.scale.y = scl.y < epsilon ? epsilon : scl.y;
		transform.scale.z = scl.z < epsilon ? epsilon : scl.z;
	}
	void setEulerRotation(const glm::vec3& eulerRotDegrees) {
		glm::vec3 radians = glm::radians(eulerRotDegrees);
		transform.quatRotation = glm::quat(radians);
	}
	void setQuatRotation(const glm::quat& quatRot) {
		transform.quatRotation = quatRot;
	}

	glm::vec3 getPosition() const {
		return transform.position;
	}
	glm::vec3 getScale() const {
		return transform.scale;
	}
	glm::vec3 getEulerRotation() const {
		return glm::degrees(glm::eulerAngles(transform.quatRotation));
	}

	void draw(const glm::mat4& worldMatrix) const {
		shaderPtr->use();
		shaderPtr->setMat4("model", worldMatrix);
		glm::mat4 normalMatrix = glm::transpose(glm::inverse(worldMatrix));
		shaderPtr->setMat4("normalMatrix", normalMatrix);
		shaderPtr->setFloat("material.shininess", 32.0f);
		modelPtr->draw(shaderPtr);
	}

	void drawShadow(const glm::mat4& modelMatrix, Shader* depthShader) const {
		if (depthShader) {
			depthShader->use();

			depthShader->setMat4("model", modelMatrix);

			modelPtr->draw(depthShader);
		}
		else {
			draw(modelMatrix);
		}
	}
};

#endif