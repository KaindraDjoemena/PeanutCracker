#include "headers/camera.h"
#include "headers/ray.h"
#include "headers/frustum.h"
#include "headers/sphereColliderComponent.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>


// CONSTRUCTOR WITH VECTORS
Camera::Camera(
	const glm::vec3& i_position,
	const glm::vec3& i_worldUP,
	float i_nearPlane,
	float i_farPlane,
	float i_yaw ,
	float i_pitch,
	float i_aspect,
	float i_lookSpeed)
	: m_pos(i_position)
	, m_worldUp(i_worldUP)
	, m_front(glm::vec3(0.0f, 0.0f, -1.0f))
	, m_movementSpeed(c_speed)
	, m_mouseSensitivity(c_sensitivity)
	, m_nearPlane(i_nearPlane)
	, m_farPlane(i_farPlane)
	, m_aspect(i_aspect)
	, m_zoom(c_zoom) {
	m_lookSpeed = i_lookSpeed;
	m_frustum.constructFrustum(m_aspect, getProjMat(m_aspect), getViewMat());
}

// CONSTRUCTOR WITH SCALARS
Camera::Camera(
	float posX, float posY, float posZ,
	float upX, float upY, float upZ,
	float i_nearPlane, float i_farPlane,
	float i_aspect,
	float yawIn, float pitchIn)
	: m_pos(posX, posY, posZ)
	, m_worldUp(upX, upY, upZ)
	, m_front(glm::vec3(0.0f, 0.0f, -1.0f))
	, m_movementSpeed(c_zoom)
	, m_mouseSensitivity(c_sensitivity)
	, m_nearPlane(i_nearPlane)
	, m_farPlane(i_farPlane)
	, m_aspect(i_aspect)
	, m_zoom(c_zoom) {
	m_frustum.constructFrustum(m_aspect, getProjMat(m_aspect), getViewMat());
}

void Camera::setAspect(float aspect) {
	m_aspect = aspect;

	// *** FLAGS ***
	m_isDirtyCamVectors = true;
}

void Camera::setPitchYaw(float pitch, float yaw) {
	m_yaw = yaw;
	m_pitch = pitch;
	if (m_pitch > 89.0f)  { m_pitch = 89.0f; }
	if (m_pitch < -89.0f) { m_pitch = -89.0f; }

	// *** FLAGS ***
	m_isDirtyCamVectors = true;
}

float Camera::getZoom() const {
	return m_zoom;
}
glm::vec3 Camera::getPos() const {
	return m_pos;
}
glm::vec3 Camera::getDir() const {
	return m_front;
}
std::array<float, 2> Camera::getPitchYaw() const {
	return { m_pitch, m_yaw };
}
glm::mat4 Camera::getViewMat() const {
	return glm::lookAt(m_pos, m_pos + m_front, m_up);
}
Frustum Camera::getFrustum() const {
	return m_frustum;
}
glm::mat4 Camera::getProjMat(float aspect) const {
	return glm::perspective(glm::radians(m_zoom), aspect, m_nearPlane, m_farPlane);
}
MouseRay Camera::getMouseRay(float mouseX, float mouseY, int viewportHeight, int viewportWidth) {
	MouseRay mouseRay;

	float aspect = (float)viewportWidth / (float)viewportHeight;
	glm::mat4 proj = getProjMat(aspect);
	glm::mat4 view = getViewMat();
	glm::mat4 projView = proj * view;

	float winX = mouseX;
	float winY = (float)viewportHeight - mouseY;

	glm::vec4 viewport(0.0f, 0.0f, (float)viewportWidth, (float)viewportHeight);

	glm::vec3 nearPt = glm::unProject(glm::vec3(winX, winY, 0.0f), glm::mat4(1.0f), projView, viewport);
	glm::vec3 farPt  = glm::unProject(glm::vec3(winX, winY, 1.0f), glm::mat4(1.0f), projView, viewport);

	mouseRay.origin = nearPt;
	mouseRay.direction = glm::normalize(farPt - nearPt);
	mouseRay.hit = false;
	mouseRay.dist = -1.0f;

	return mouseRay;
}

void Camera::processInput(Camera_Movement type, float deltaTime) {
	bool changedView = false;

	float moveVelocity = m_movementSpeed * deltaTime;
	if (type == Camera_Movement::FORWARD)  { m_pos += m_front * moveVelocity; changedView = true; }
	if (type == Camera_Movement::BACKWARD) { m_pos -= m_front * moveVelocity; changedView = true; }
	if (type == Camera_Movement::LEFT)     { m_pos -= m_right * moveVelocity; changedView = true; }
	if (type == Camera_Movement::RIGHT)    { m_pos += m_right * moveVelocity; changedView = true; }
	if (type == Camera_Movement::UP)       { m_pos += m_up *    moveVelocity; changedView = true; }
	if (type == Camera_Movement::DOWN)     { m_pos -= m_up *    moveVelocity; changedView = true; }

	float lookVelocity = m_lookSpeed * deltaTime;
	if (type == Camera_Movement::LOOK_UP)    { m_pitch += m_lookSpeed * lookVelocity; changedView = true; if (m_pitch > 89.0) { m_pitch = 89.0; } }
	if (type == Camera_Movement::LOOK_DOWN)  { m_pitch -= m_lookSpeed * lookVelocity; changedView = true; if (m_pitch < -89.0) { m_pitch = -89.0; } }
	if (type == Camera_Movement::LOOK_LEFT)  { m_yaw   -= m_lookSpeed * lookVelocity; changedView = true; }
	if (type == Camera_Movement::LOOK_RIGHT) { m_yaw   += m_lookSpeed * lookVelocity; changedView = true; }


	// *** FLAGS ***
	if (changedView) { m_isDirtyCamVectors = true; }
}

void Camera::processMouseMovement(double xOffset, double yOffset, GLboolean constrainPitch) {
	xOffset *= m_mouseSensitivity;
	yOffset *= m_mouseSensitivity;

	m_yaw += xOffset;
	m_pitch += yOffset;

	setPitchYaw(m_pitch, m_yaw);


	// *** FLAGS ***
	m_isDirtyCamVectors = true;
}

void Camera::processMouseScroll(double yOffset) {
	m_zoom -= (double)yOffset;
	if (m_zoom < c_minZoom) { m_zoom = c_minZoom; }
	if (m_zoom > c_maxZoom) { m_zoom = c_maxZoom; }


	// *** FLAGS ***
	m_isDirtyCamVectors = true;
}

void Camera::updateVectors() {
	if (m_isDirtyCamVectors) {
		// Calculating new camera front vector
		glm::vec3 tempFront;
		tempFront.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
		tempFront.y = sin(glm::radians(m_pitch));
		tempFront.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
		m_front = tempFront;

		// Calculating the right and up vectors
		m_right = glm::normalize(glm::cross(m_front, m_worldUp));
		m_up    = glm::normalize(glm::cross(m_right, m_front));

		// Reconstruct frustum
		m_frustum.constructFrustum(m_aspect, getProjMat(m_aspect), getViewMat());

		m_isDirtyCamVectors = false;
	}

}
