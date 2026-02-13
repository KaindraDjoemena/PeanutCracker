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


Object::Object(Model* i_modelPtr)
    : modelPtr(i_modelPtr)
    , transform() {
}


/* === SETTERS =========================================================== */
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


/* === INTERFACE =========================================================== */
void Object::draw(const Shader& shader, const glm::mat4& worldMatrix) const {
    shader.use();
    shader.setMat4("model", worldMatrix);
    glm::mat4 normalMatrix = glm::transpose(glm::inverse(worldMatrix));
    shader.setMat4("normalMatrix", normalMatrix);
    shader.setFloat("material.shininess", 32.0f);
    modelPtr->draw(shader);
}

void Object::drawShadow(const glm::mat4& modelMatrix, const Shader& depthShader) const {
    depthShader.use();
    depthShader.setMat4("model", modelMatrix);
    modelPtr->draw(depthShader);
}