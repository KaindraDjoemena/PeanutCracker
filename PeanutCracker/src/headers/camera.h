#pragma once

#include "ray.h"
#include "frustum.h"
#include "sphereColliderComponent.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <array>


// CAMERA MOVEMENT ENUM
enum Camera_Movement {
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
const float FOV = 45.0;
const float SPEED = 5.0f;
const float SENSITIVITY = 0.3f;
const float ZOOM = FOV;
const float LOOK_SPEED = 10.0f;


class Camera {
public:
	// ATTRIBUTES
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	float yaw;
	float pitch;

	float movementSpeed;
	float mouseSensitivity;
	float zoom;
	float lookSpeed;

	float nearPlane;
	float farPlane;
	float aspect;

	Frustum frustum;

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

	void setPitchYaw(float pitch, float yaw);

	std::array<float, 2> getPitchYaw() const;

	glm::mat4 getViewMat() const;

	glm::mat4 getProjMat(float aspect) const;

	MouseRay getMouseRay(float mouseX, float mouseY, int viewportHeight, int viewportWidth);

	void processInput(Camera_Movement type, float deltaTime);

	void processMouseMovement(double xOffset, double yOffset, GLboolean constrainPitch = true);

	void processMouseScroll(double yOffset);

	void updateCameraVectors();

private:
	bool m_isDirtyCamVectors = true;
};