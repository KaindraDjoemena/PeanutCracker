#pragma once

#include "ray.h"
#include "frustum.h"
#include "sphereColliderComponent.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <array>


// CAMERA MOVEMENT ENUM
enum class Camera_Movement {
    FORWARD,	// W
    BACKWARD,	// S
    LEFT,		// A
    RIGHT,		// D
    UP,			// E
    DOWN,		// Q
    LOOK_UP,	// Up
    LOOK_DOWN,	// Down
    LOOK_LEFT,	// Left
    LOOK_RIGHT	// Right
};

enum class Camera_Mode {
    ARCBALL,
    FLY
};

const float c_yaw = 0.0f;
const float c_pitch = 0.0f;
const float c_fov = 70.0f;
const float c_speed = 5.0f;
const float c_sensitivity = 0.3f;
const float c_zoom = c_fov;
const float c_lookSpeed = 10.0f;
const float c_maxZoom = 75.0f;
const float c_minZoom = 20.0f;

class Camera {
public:
    
    Camera(     // CONSTRUCTOR WITH VECTORS
        const glm::vec3& i_position = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec3& i_worldUP = glm::vec3(0.0f, 1.0f, 0.0f),
        float i_nearPlane = 0.1f,
        float i_farPlane = 100.0f,
        float i_yaw = c_yaw,
        float i_pitch = c_pitch,
        float i_aspect = (16.0f / 9.0f),
        float i_lookSpeed = c_lookSpeed
    );

    
    Camera(     // CONSTRUCTOR WITH SCALARS
        float posX, float posY, float posZ,
        float upX, float upY, float upZ,
        float i_nearPlane, float i_farPlane,
        float i_aspect,
        float yawIn, float pitchIn
    );

    /* === SETTERS =========================================================== */
    void setAspect(float aspect);

    void setPitchYaw(float pitch, float yaw);


    /* === GETTERS =========================================================== */
    float getZoom() const { return m_zoom; }

    glm::vec3 getPos() const { return m_pos; }

    glm::vec3 getDir() const { return m_front; }

    std::array<float, 2> getPitchYaw() const { return { m_pitch, m_yaw }; }

    glm::mat4 getViewMat() const { return glm::lookAt(m_pos, m_pos + m_front, m_up); }

    glm::mat4 getProjMat(float aspect) const { return glm::perspective(glm::radians(m_zoom), aspect, m_nearPlane, m_farPlane); }
   
    Frustum getFrustum() const { return m_frustum; }
    
    MouseRay getMouseRay(float mouseX, float mouseY, int viewportHeight, int viewportWidth);


    /* === INTERFACE =========================================================== */
    void processInput(Camera_Movement type, float deltaTime);

    void processMouseMovement(double xOffset, double yOffset, GLboolean constrainPitch = true);

    void processMouseScroll(double yOffset);

    void updateVectors();

private:
    glm::vec3 m_pos;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_zoom;
    float m_lookSpeed;

    float m_nearPlane;
    float m_farPlane;
    float m_aspect;

    Frustum m_frustum;

    // --- flags ---
    bool m_isDirtyCamVectors = true;
};