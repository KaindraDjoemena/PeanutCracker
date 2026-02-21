#pragma once

#include "ray.h"
#include "frustum.h"

const float c_fov = 60.0f;
const float c_minDistance = 0.01f;
const float c_maxDistance = 1000.0f;

class Camera {
public:
    Camera(
        const glm::vec3& i_target = glm::vec3(0.0f),
        float            i_distance = 10.0f,
        float            i_nearPlane = 0.1f,
        float            i_farPlane = 1000.0f,
        float            i_aspect = (16.0f / 9.0f)
    );

    void setAspect(float aspect);
    void setTarget(const glm::vec3& target);

    float     getFov()      const { return c_fov; }
    glm::vec3 getPos()      const;
    glm::mat4 getViewMat()  const;
    glm::mat4 getProjMat(float i_aspect) const { return glm::perspective(glm::radians(c_fov), i_aspect, m_nearPlane, m_farPlane); }
    glm::mat4 getProjMat()  const { return glm::perspective(glm::radians(c_fov), m_aspect, m_nearPlane, m_farPlane); }
    Frustum   getFrustum()  const { return m_frustum; }
    MouseRay  getMouseRay(float mouseX, float mouseY, int viewportHeight, int viewportWidth);

    void beginDrag(glm::vec2 mousePos, bool isPan);
    void endDrag();
    void processDrag(glm::vec2 mousePos, glm::vec2 viewportSize);
    void processMouseScroll(double yOffset);
    void updateVectors();

private:
    float m_yaw = 0.0f;  
    float m_pitch = 0.0f;

    glm::vec3 m_target = glm::vec3(0.0f);
    float m_distance = 10.0f;

    float m_nearPlane;
    float m_farPlane;
    float m_aspect;

    glm::vec2 m_lastMousePos = glm::vec2(0.0f);
    bool m_isDragging = false;
    bool m_isPanning = false;

    Frustum m_frustum;
    bool m_isDirtyCamVectors = true;

    static constexpr float ROTATION_SENSITIVITY = 0.3f;
    static constexpr float PAN_SENSITIVITY = 0.002f;
    static constexpr float SCROLL_SENSITIVITY = 0.1f;
};