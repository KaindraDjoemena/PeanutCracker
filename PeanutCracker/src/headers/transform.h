# pragma once

#define GLM_ENABLE_EXPERIMENTAL 

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>


struct Transform {
    glm::vec3 position      = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 scale         = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat quatRotation  = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    inline glm::mat4 getModelMatrix() const {
        // Model Space Matrix = T x R x S
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat  = glm::translate(modelMat, position);
        modelMat *= glm::mat4_cast(quatRotation);
        modelMat  = glm::scale(modelMat, scale);

        return modelMat;
    }

    inline void setPosition(const glm::vec3& pos) { position = pos; }
    inline void setScale(const glm::vec3& scl) { scale = glm::max(scl, glm::vec3(0.0001f)); }
    inline void setUScale(float factor) { setScale(glm::vec3() * factor); }
    inline void setRotDeg(const glm::vec3& rotDeg) { quatRotation = glm::quat(glm::radians(rotDeg)); }
    inline void setRotRad(const glm::vec3& rotRad) { quatRotation = glm::quat(rotRad); }
    inline void setRotQuat(const glm::quat& rot) { quatRotation = rot; }
};