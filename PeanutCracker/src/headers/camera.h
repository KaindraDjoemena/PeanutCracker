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

// DEFAULT CAMERA VALUES
const double YAW = 0.0f;
const double PITCH = 0.0f;
const float FOV = 70.0f;
const float SPEED = 5.0f;
const float SENSITIVITY = 0.3f;
const float ZOOM = FOV;
const float LOOK_SPEED = 10.0f;
const float MAX_ZOOM = 75.0f;
const float MIN_ZOOM = 20.0f;


class Camera {
public:
	// CONSTRUCTOR WITH VECTORS
	Camera(const glm::vec3& i_position = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3& i_worldUP = glm::vec3(0.0f, 1.0f, 0.0f),
		float i_nearPlane = 0.1f,
		float i_farPlane = 100.0f,
		double i_yaw = YAW,
		double i_pitch = PITCH,
		float i_aspect = (16.0f / 9.0f),
		float i_lookSpeed = LOOK_SPEED);

	// CONSTRUCTOR WITH SCALARS
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float i_nearPlane, float i_farPlane, float i_aspect, double yawIn, double pitchIn);

	void setAspect(float aspect);

	void setPitchYaw(float pitch, float yaw);

	std::array<float, 2> getPitchYaw() const;

	glm::vec3 getPos() const;

	glm::vec3 getDir() const;

	float getZoom() const;

	glm::mat4 getViewMat() const;

	glm::mat4 getProjMat(float aspect) const;

	Frustum getFrustum() const;

	MouseRay getMouseRay(float mouseX, float mouseY, int viewportHeight, int viewportWidth);

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

	bool m_isDirtyCamVectors = true;
};