#ifndef CAMERA_H
#define CAMERA_H

#include "ray.h"
#include "sphereColliderComponent.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


struct Frustum_Plane {
	glm::vec3 normal;
	float	  distance;
};

// CAMERA FRUSTUM
struct Camera_Frustum {
	Frustum_Plane left;
	Frustum_Plane right;
	Frustum_Plane bottom;
	Frustum_Plane top;
	Frustum_Plane near;
	Frustum_Plane far;
};

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

	Camera_Frustum frustum;

	// CONSTRUCTOR WITH VECTORS
	Camera(const glm::vec3& positionIn = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3& worldUpIn = glm::vec3(0.0f, 1.0f, 0.0f),
		float i_nearPlane = 0.1f,
		float i_farPlane = 100.0f,
		double yawIn = YAW,
		double pitchIn = PITCH,
		float i_aspect = (16.0f/9.0f),
		float i_lookSpeed = LOOK_SPEED)
		: front(glm::vec3(0.0f, 0.0f, -1.0f))
		, movementSpeed(SPEED)
		, mouseSensitivity(SENSITIVITY)
		, nearPlane(i_nearPlane)
		, farPlane(i_farPlane)
		, aspect(i_aspect)
		, zoom(ZOOM) {
		lookSpeed = i_lookSpeed;
		position = positionIn;
		worldUp = worldUpIn;
		createCameraFrustum(aspect);
		updateCameraVectors();
	}

	// CONSTRUCTOR WITH SCALARS
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float i_nearPlane, float i_farPlane, float i_aspect, double yawIn, double pitchIn)
		: front(glm::vec3(0.0f, 0.0f, -1.0f))
		, movementSpeed(SPEED)
		, mouseSensitivity(SENSITIVITY)
		, nearPlane(i_nearPlane)
		, farPlane(i_farPlane)
		, aspect(i_aspect)
		, zoom(ZOOM) {
		position = glm::vec3(posX, posY, posZ);
		worldUp = glm::vec3(upX, upY, upZ);
		createCameraFrustum(aspect);
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

	bool isInFrustum(const SphereColliderComponent& sphere) const {
		if (isOutsidePlane(sphere, frustum.left)   || isOutsidePlane(sphere, frustum.right) ||
			isOutsidePlane(sphere, frustum.bottom) || isOutsidePlane(sphere, frustum.top)	||
			isOutsidePlane(sphere, frustum.near)   || isOutsidePlane(sphere, frustum.far)
		) {
			std::cout << "NOT IN frustum" << '\n';
			return false;
		}

		std::cout << "IN frustum" << '\n';
		return true;

	}

	bool isOutsidePlane(const SphereColliderComponent& sphere, Frustum_Plane plane) const {
		float distance = glm::dot(plane.normal, sphere.worldCenter) + plane.distance;
		//std::cout << "Dist: " << distance << " | -Radius: " << -sphere.worldRadius << std::endl;
		if (distance < -sphere.localRadius) { return true; }
		return false;
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

	void createCameraFrustum(float aspect) {
		glm::mat4 viewProjMat = getPerspectiveProjectionMatrix(aspect) * getViewMatrix();

		for (int i = 0; i < 3; i++) {
			float posA = viewProjMat[3][0] + viewProjMat[i][0];
			float posB = viewProjMat[3][1] + viewProjMat[i][1];
			float posC = viewProjMat[3][2] + viewProjMat[i][2];
			float posD = viewProjMat[3][3] + viewProjMat[i][3];
			glm::vec3 posN = glm::normalize(glm::vec3(posA, posB, posC));

			float negA = viewProjMat[3][0] - viewProjMat[i][0];
			float negB = viewProjMat[3][1] - viewProjMat[i][1];
			float negC = viewProjMat[3][2] - viewProjMat[i][2];
			float negD = viewProjMat[3][3] - viewProjMat[i][3];
			glm::vec3 negN = glm::normalize(glm::vec3(negA, negB, negC));


			switch (i) {
			case 0:
				frustum.left	= { posN, posD };
				frustum.right	= { negN, negD };
				break;
			case 1:
				frustum.bottom	= { posN, posD };
				frustum.top		= { negN, negD };
				break;
			case 2:
				frustum.near	= { posN, posD };
				frustum.far		= { negN, negD };
				break;
			default:
				break;
			}
		}
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