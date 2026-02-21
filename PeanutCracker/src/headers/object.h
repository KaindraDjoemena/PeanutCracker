#pragma once

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
    Transform	transform;
    bool		isSelected = false;

    glm::mat4	modelMatrixCache  = glm::mat4(1.0f);
    glm::mat4	normalMatrixCache = glm::mat4(1.0f);

    Object(Model* i_modelPtr);


    /* === GETTERS =========================================================== */
    glm::vec3 getPosition() const { return transform.position; }

    glm::vec3 getScale() const { return transform.scale; }

    glm::vec3 getEulerRotation() const { return glm::degrees(glm::eulerAngles(transform.quatRotation)); }


    /* === SETTERS =========================================================== */
    void setPosition(const glm::vec3& pos);

    void setScale(const glm::vec3& scl);

    void setEulerRotation(const glm::vec3& eulerRotDegrees);

    void setQuatRotation(const glm::quat& quatRot);


    /* === INTERFACE =========================================================== */
    void draw(const Shader& shader, const glm::mat4& worldMatrix) const;

    void drawShadow(const glm::mat4& modelMatrix, const Shader& depthShader) const;
};