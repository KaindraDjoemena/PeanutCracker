#include "headers/object.h"
#include "headers/shader.h"
#include "headers/light.h"
#include "headers/model.h"
#include "headers/transform.h"

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


Object::Object(Model* i_modelPtr, Shader* i_shaderPtr)
	: modelPtr(i_modelPtr)
	, shaderPtr(i_shaderPtr)
	, transform() {
}

void Object::setPosition(const glm::vec3& pos) {
	transform.position = pos;
}
void Object::setScale(const glm::vec3& scl) {
	transform.scale.x = scl.x < epsilon ? epsilon : scl.x;
	transform.scale.y = scl.y < epsilon ? epsilon : scl.y;
	transform.scale.z = scl.z < epsilon ? epsilon : scl.z;
}
void Object::setEulerRotation(const glm::vec3& eulerRotDegrees) {
	glm::vec3 radians = glm::radians(eulerRotDegrees);
	transform.quatRotation = glm::quat(radians);
}
void Object::setQuatRotation(const glm::quat& quatRot) {
	transform.quatRotation = quatRot;
}

glm::vec3 Object::getPosition() const {
	return transform.position;
}
glm::vec3 Object::getScale() const {
	return transform.scale;
}
glm::vec3 Object::getEulerRotation() const {
	return glm::degrees(glm::eulerAngles(transform.quatRotation));
}

void Object::draw(const glm::mat4& worldMatrix) const {
	shaderPtr->use();
	shaderPtr->setMat4("model", worldMatrix);
	glm::mat4 normalMatrix = glm::transpose(glm::inverse(worldMatrix));
	shaderPtr->setMat4("normalMatrix", normalMatrix);
	shaderPtr->setFloat("material.shininess", 32.0f);
	modelPtr->draw(shaderPtr);
}

void Object::drawShadow(const glm::mat4& modelMatrix, Shader* depthShader) const {
	if (depthShader) {
		depthShader->use();

		depthShader->setMat4("model", modelMatrix);

		modelPtr->draw(depthShader);
	}
	else {
		draw(modelMatrix);
	}
}