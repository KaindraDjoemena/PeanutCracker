#include "headers/camera.h"
#include "headers/ray.h"
#include "headers/frustum.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(
    const glm::vec3& i_target,
    float            i_distance,
    float            i_nearPlane,
    float            i_farPlane,
    float            i_aspect)
    : m_target(i_target)
    , m_distance(i_distance)
    , m_nearPlane(i_nearPlane)
    , m_farPlane(i_farPlane)
    , m_aspect(i_aspect)
    , m_yaw(0.0f)
    , m_pitch(0.0f)
{
    updateVectors();
}

void Camera::setAspect(float aspect) {
    m_aspect = aspect;
    m_isDirtyCamVectors = true;
}

void Camera::setTarget(const glm::vec3& target) {
    m_target = target;
    m_isDirtyCamVectors = true;
}

glm::vec3 Camera::getPos() const {
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    float x = m_distance * cos(pitchRad) * sin(yawRad);
    float y = m_distance * sin(pitchRad);
    float z = m_distance * cos(pitchRad) * cos(yawRad);

    return m_target + glm::vec3(x, y, z);
}

glm::mat4 Camera::getViewMat() const {
    return glm::lookAt(getPos(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

MouseRay Camera::getMouseRay(float mouseX, float mouseY, int viewportHeight, int viewportWidth) {
    MouseRay mouseRay;

    float winX = mouseX;
    float winY = static_cast<float>(viewportHeight) - mouseY;
    glm::vec4 viewport(0.0f, 0.0f, (float)viewportWidth, (float)viewportHeight);

    glm::mat4 proj = getProjMat();
    glm::mat4 view = getViewMat();
    glm::mat4 projView = proj * view;

    glm::vec3 nearPt = glm::unProject(glm::vec3(winX, winY, 0.0f), glm::mat4(1.0f), projView, viewport);
    glm::vec3 farPt = glm::unProject(glm::vec3(winX, winY, 1.0f), glm::mat4(1.0f), projView, viewport);

    mouseRay.origin = nearPt;
    mouseRay.direction = glm::normalize(farPt - nearPt);
    mouseRay.hit = false;
    mouseRay.dist = -1.0f;

    return mouseRay;
}

void Camera::beginDrag(glm::vec2 mousePos, bool isPan) {
    m_lastMousePos = mousePos;
    m_isDragging = !isPan;
    m_isPanning = isPan;
}

void Camera::endDrag() {
    m_isDragging = false;
    m_isPanning = false;
}

void Camera::processDrag(glm::vec2 mousePos, glm::vec2 viewportSize) {
    glm::vec2 delta = mousePos - m_lastMousePos;

    if (m_isDragging) {
        m_yaw -= delta.x * ROTATION_SENSITIVITY;
        m_pitch += delta.y * ROTATION_SENSITIVITY;
        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);
        m_isDirtyCamVectors = true;
    }

    if (m_isPanning) {
        float panScale = m_distance * PAN_SENSITIVITY;

        glm::vec3 pos = getPos();
        glm::vec3 forward = glm::normalize(m_target - pos);

        glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward));

        glm::vec3 up = glm::cross(forward, right);

        m_target += right * delta.x * panScale;
        m_target += up * delta.y * panScale;

        m_isDirtyCamVectors = true;
    }

    m_lastMousePos = mousePos;
}
void Camera::processMouseScroll(double yOffset) {
    float zoomSpeed = m_distance * SCROLL_SENSITIVITY;
    m_distance -= (float)yOffset * zoomSpeed;

    m_distance = glm::clamp(m_distance, c_minDistance, c_maxDistance);

    m_isDirtyCamVectors = true;
}

void Camera::updateVectors() {
    if (!m_isDirtyCamVectors) return;

    m_frustum.constructFrustum(m_aspect, getProjMat(), getViewMat());
    
    m_isDirtyCamVectors = false;
}