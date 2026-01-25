#ifndef CAMERA_H
#define CAMERA_H

#include "ray.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


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
const double YAW = -90.0;
const double PITCH = 0.0;
const float FOV = 45.0f;
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

	// CONSTRUCTOR WITH VECTORS
	Camera(const glm::vec3& positionIn = glm::vec3(0.0f, 0.0f, 0.0f),
		   const glm::vec3& worldUpIn = glm::vec3(0.0f, 1.0f, 0.0f),
		   float i_nearPlane = 0.1f,
		   float i_farPlane = 100.0f,
		   double yawIn = YAW,
		   double pitchIn = PITCH,
		   float i_lookSpeed = LOOK_SPEED)
		: front(glm::vec3(0.0f, 0.0f, -1.0f))
		, movementSpeed(SPEED)
		, mouseSensitivity(SENSITIVITY)
		, nearPlane(i_nearPlane)
		, farPlane(i_farPlane)
		, zoom(ZOOM) {
		lookSpeed = i_lookSpeed;
		position = positionIn;
		worldUp = worldUpIn;
		updateCameraVectors();
	}

	// CONSTRUCTOR WITH SCALARS
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float i_nearPlane, float i_farPlane, double yawIn, double pitchIn)
		: front(glm::vec3(0.0f, 0.0f, -1.0f))
		, movementSpeed(SPEED)
		, mouseSensitivity(SENSITIVITY)
		, nearPlane(i_nearPlane)
		, farPlane(i_farPlane)
		, zoom(ZOOM) {
		position = glm::vec3(posX, posY, posZ);
		worldUp = glm::vec3(upX, upY, upZ);
		updateCameraVectors();
	}

	glm::mat4 getViewMatrix() const {
		return glm::lookAt(position, position + front, up);
	}

	glm::mat4 getPerspectiveProjectionMatrix(float aspect) const {
		return glm::perspective(glm::radians(zoom), aspect, nearPlane, farPlane);
	}

	MouseRay getMouseRay(float mouseX, float mouseY, int viewportHeight, int viewportWidth) {
		MouseRay mouseRay;

		float aspect = (float)viewportWidth / (float)viewportHeight;
		glm::mat4 proj = getPerspectiveProjectionMatrix(aspect);
		glm::mat4 view = getViewMatrix();
		glm::mat4 projView = proj * view;
		
		float winX = mouseX;
		float winY = (float)viewportHeight - mouseY;

		glm::vec4 viewport(0.0f, 0.0f, (float)viewportWidth, (float)viewportHeight);

		glm::vec3 nearPt = glm::unProject(glm::vec3(winX, winY, 0.0f), glm::mat4(1.0f), projView, viewport);
		glm::vec3 farPt = glm::unProject(glm::vec3(winX, winY, 1.0f), glm::mat4(1.0f), projView, viewport);

		mouseRay.origin = nearPt;
		mouseRay.direction = glm::normalize(farPt - nearPt);
		mouseRay.hit = false;
		mouseRay.dist = -1.0f;

		return mouseRay;
	}

	void processInput(Camera_Movement type, float deltaTime) {
		float moveVelocity = movementSpeed * deltaTime;
		if (type == FORWARD)	position += front * moveVelocity;
		if (type == BACKWARD)	position -= front * moveVelocity;
		if (type == LEFT)		position -= right * moveVelocity;
		if (type == RIGHT)		position += right * moveVelocity;
		if (type == UP)			position += up * moveVelocity;
		if (type == DOWN)		position -= up * moveVelocity;

		float lookVelocity = lookSpeed * deltaTime;
		bool changedView = false;
		if (type == LOOK_UP)	{ pitch += lookSpeed * lookVelocity; changedView = true; if (pitch > 89.0) { pitch = 89.0; }}
		if (type == LOOK_DOWN)	{ pitch -= lookSpeed * lookVelocity; changedView = true; if (pitch < -89.0) { pitch = -89.0; }}
		if (type == LOOK_LEFT)	{ yaw -= lookSpeed * lookVelocity; changedView = true; }
		if (type == LOOK_RIGHT) { yaw += lookSpeed * lookVelocity; changedView = true; }

		if (changedView) { updateCameraVectors(); }
	}

	void processMouseMovement(double xOffset, double yOffset, GLboolean constrainPitch = true) {
		xOffset *= mouseSensitivity;
		yOffset *= mouseSensitivity;

		yaw	  += xOffset;
		pitch += yOffset;

		// Clamping the pitch values
		if (constrainPitch) {
			if (pitch > 89.0) { pitch = 89.0; }
			if (pitch < -89.0) { pitch = -89.0; }
		}

		updateCameraVectors();
	}

	void processMouseScroll(double yOffset) {
		zoom -= (double)yOffset;
		if (zoom < 1.0) { zoom = 1.0; }
		if (zoom > 45.0) { zoom = 45.0; }
	}

private:
	void updateCameraVectors() {
		// Calculating new camera front vector
		glm::vec3 tempFront;
		tempFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		tempFront.y = sin(glm::radians(pitch));
		tempFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front = tempFront;

		// Calculating the right and up vectors
		right = glm::normalize(glm::cross(front, worldUp));
		up = glm::normalize(glm::cross(right, front));

	}
};

#endif CAMERA_H