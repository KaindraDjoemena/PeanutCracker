//#include "headers/ebo.h"
#include "headers/vbo.h"
#include "headers/vao.h"
#include "headers/shader.h"
//#include "headers/texture.h"
#include "headers/camera.h"
//#include "headers/material.h"
#include "headers/light.h"
#include "headers/model.h"
#include "headers/object.h"
#include "headers/cubemap.h"
#include "headers/gui.h"
#include "headers/scene.h"
#include "headers/assetManager.h"
#include "headers/renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>


struct WindowContext {
	GUI* gui;
	Scene* scene;
};


// CALLBACK FUNCTIONS
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void keyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseCallback(GLFWwindow* window, double xPos, double yPos);
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
void dropCallback(GLFWwindow* window, int count, const char** paths);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

// POLLING FUNCTIONS
void processInput(GLFWwindow* window);


// WINDOW SETUP
const unsigned int SCR_WIDTH  = 1000;
const unsigned int SCR_HEIGHT = 750;
int scr_width  = SCR_WIDTH;
int scr_height = SCR_HEIGHT;
const glm::vec4 WINDOW_BACKGROUND_COLOR(0.6f, 0.6f, 0.6f, 1.0f);

// MOUSE
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;

// CAMERA SETUP
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraWorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float nearPlane = 0.01f;
float farPlane = 100.0f;
float aspect = (float)scr_width / (float)scr_height;
Camera cameraObject(cameraPos, cameraWorldUp, nearPlane, farPlane, aspect, cameraYaw, cameraPitch);

// DELTA TIME
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// RENDERING STUFF
const int MSAA_SAMPLES = 4;


int main() {
	std::cout << "[MAIN] C++ version: " << __cplusplus << '\n';
	std::cout << "[MAIN] current path: " << std::filesystem::current_path() << '\n';

	// === SETTINGS AND INITS ================================
	// WINDOWING API INITIALIZATION
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, MSAA_SAMPLES);

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// INITALIZING AND ASSIGNING A WINDOW
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Peanut Cracker", NULL, NULL);
	if (window == NULL) {
		std::cout << "[MAIN] Failed to create GLFW window" << '\n';
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);


	// CALLBACK FUNCTIONS
	glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
	glfwSetKeyCallback(window, keyPressCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetDropCallback(window, dropCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// INITALIZING GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "[MAIN] Failed to initialize GLAD" << '\n';
		return -1;
	}

	glfwSwapInterval(0);	// Disable VSYNC

	auto assetManagerPtr = std::make_unique<AssetManager>();
	Scene scene(assetManagerPtr.get());



	// === OPENGL SETTINGS =====================================
	// -- Rendering
	//glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);	 // -> can be put in a method -> [opengl state machine class]
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_MULTISAMPLE);


	// === LIGHT STUFF =========================================
	
	auto pointLight = std::make_unique<PointLight>(
		glm::vec3(0.0f, 0.0f, 0.0f),
		Light(glm::vec3(0.1f), glm::vec3(0.8f), glm::vec3(1.0f)),
		Attenuation(1.0f, 0.022f, 0.009f),
		0.1f, 20.0f
	);
	scene.createAndAddPointLight(std::move(pointLight));
	auto pointLight1 = std::make_unique<PointLight>(
		glm::vec3(0.0f, 0.0f, 10.0f),
		Light(glm::vec3(0.1f), glm::vec3(0.8f), glm::vec3(1.0f)),
		Attenuation(1.0f, 0.022f, 0.009f),
		0.1f, 20.0f
	);
	scene.createAndAddPointLight(std::move(pointLight1));

	//auto directionalLight = std::make_unique<DirectionalLight>(
	//	glm::vec3(0.0f, 0.0f, -1.0f),
	//	Light(glm::vec3(0.1f), glm::vec3(0.8f), glm::vec3(1.0f)),
	//	0.1f, 100.0f
	//);
	//scene.createAndAddDirectionalLight(std::move(directionalLight));

	//auto spotLight = std::make_unique<SpotLight>(
	//	glm::vec3(0.0f, 0.0f, 10.0f),
	//	glm::vec3(0.0f, 0.0f, -1.0f),
	//	Light(glm::vec3(0.1f), glm::vec3(0.8f), glm::vec3(1.0f)),
	//	Attenuation(),
	//	cosf(glm::radians(10.0f)),
	//	cosf(glm::radians(20.0f)),
	//	0.01f, 10.0f);
	//scene.createAndAddSpotLight(std::move(spotLight));

	// STBI IMAGE FLIPPING FOR TEXTURES
	stbi_set_flip_vertically_on_load(true);

	Renderer renderer(SCR_WIDTH, SCR_HEIGHT);

	// === GUI ==================================================
	//Framebuffer sceneFBO;
	//sceneFBO.setup(SCR_WIDTH, SCR_HEIGHT);
	GUI gui(window, "#version 330");
	glfwSetWindowUserPointer(window, &gui);

	scene.init();

	WindowContext context = { &gui, &scene };
	glfwSetWindowUserPointer(window, &context);

	// === RENDER LOOP ==========================================
	while (!glfwWindowShouldClose(window)) {
		// GET TIME
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// A. Update UI and get the Viewport size
		ImVec2 vSize = gui.update(deltaTime, cameraObject, scene, renderer, renderer.getViewportFBO()->resolveTexture);
		gui.render();

		// B. Rescale FBO and Update Projection if the UI viewport changed
		renderer.getViewportFBO()->rescale((int)vSize.x, (int)vSize.y);
		float aspect = (vSize.y > 0) ? vSize.x / vSize.y : 1.0f;

		// RENDER TO FBO
		glBindFramebuffer(GL_FRAMEBUFFER, renderer.getViewportFBO()->fbo);
		glViewport(0, 0, (int)vSize.x, (int)vSize.y);

		// BUFFER STUFF
		glClearColor(WINDOW_BACKGROUND_COLOR.r, WINDOW_BACKGROUND_COLOR.g, WINDOW_BACKGROUND_COLOR.b, WINDOW_BACKGROUND_COLOR.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


		renderer.update(scene, cameraObject, vSize.x, vSize.y);
		renderer.renderScene(scene, cameraObject, vSize.x, vSize.y);

		// SCENE RENDERING
		//scene.draw(cameraObject, vSize.x, vSize.y);

		renderer.getViewportFBO()->resolve();

		// UNBIND FRAME BUFFER
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// WINDOWING STUFF
		glfwSwapBuffers(window);
		glfwPollEvents();

		// INPUT HANDLING
		processInput(window);

		// OBJECT LOADING QUEUE
		scene.processLoadQueue();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

// === CALLBACK FUNCTIONS ===============================================================
void frameBufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow* window, double xPos, double yPos) {
	WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx || !ctx->gui) return;
	if (ctx->gui && ImGui::GetIO().WantCaptureMouse && !ctx->gui->isViewportHovered) return;
	if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) return;

	// --Navigation
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (firstMouse) {
			lastX = xPos;
			lastY = yPos;
			firstMouse = false;
		}

		float xOffset =   xPos - lastX;
		float yOffset = -(yPos - lastY);
		lastX = xPos;
		lastY = yPos;

		cameraObject.processMouseMovement(xOffset, yOffset, true);
	}
	else {
		firstMouse = true;
	}
}

void keyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx || !ctx->gui) return;
	ImGuiIO& io = ImGui::GetIO();
	if (ImGui::GetIO().WantCaptureMouse && !ctx->gui->isViewportHovered) return;
	if (action != GLFW_PRESS) return;


	// --Window Close
	if (key == GLFW_KEY_Q && (mods & GLFW_MOD_CONTROL)) {
		glfwSetWindowShouldClose(window, true);
	}

	// --Entitiy Deletion
	if (key == GLFW_KEY_DELETE) {
		ctx->scene->deleteSelectedEntities();
	}

	// -- Entitiy Duplication
	if (key == GLFW_KEY_D && (mods & GLFW_MOD_CONTROL)) {
		ctx->scene->duplicateSelectedEntities();
	}
}

// MOUSE CLICKING
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx || !ctx->gui || !ctx->scene) return;
	if (ImGui::GetIO().WantCaptureMouse && !ctx->gui->isViewportHovered) return;
	if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) return;


	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);

		float relX = (float)xPos - ctx->gui->viewportBoundsMin.x;
		float relY = (float)yPos - ctx->gui->viewportBoundsMin.y;
		//std::cout << "viewportsize.x: " << ctx->gui->viewportSize.x << '\n';

		if (!ctx->gui->isViewportHovered) return;
		if (ctx->gui->viewportSize.x <= 0 || ctx->gui->viewportSize.y <= 0) return;

		MouseRay ray = cameraObject.getMouseRay(
			relX, relY,
			(int)ctx->gui->viewportSize.x,
			(int)ctx->gui->viewportSize.y
		);

		bool isHoldingShift = false;

		if (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			isHoldingShift = true;
		}

		ctx->scene->selectEntity(ray, isHoldingShift);
	}
}

// MOUSE SCROLLING
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
	WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (ImGui::GetIO().WantCaptureMouse && !ctx->gui->isViewportHovered) return;


	cameraObject.processMouseScroll(yOffset);
}

// USER KEYBOARD INPUT
void processInput(GLFWwindow* window) {
	WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx || !ctx->gui || !ctx->scene) return;
	if (ImGui::GetIO().WantCaptureMouse && !ctx->gui->isViewportHovered) return;


	bool isHoldingCTRL = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
		glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);

	if (!isHoldingCTRL) {
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraObject.processInput(Camera_Movement::FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraObject.processInput(Camera_Movement::BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraObject.processInput(Camera_Movement::LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraObject.processInput(Camera_Movement::RIGHT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraObject.processInput(Camera_Movement::UP, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraObject.processInput(Camera_Movement::DOWN, deltaTime);
	
		if (glfwGetKey(window, GLFW_KEY_UP)		== GLFW_PRESS) cameraObject.processInput(Camera_Movement::LOOK_UP, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_DOWN)	== GLFW_PRESS) cameraObject.processInput(Camera_Movement::LOOK_DOWN , deltaTime);
		if (glfwGetKey(window, GLFW_KEY_LEFT)	== GLFW_PRESS) cameraObject.processInput(Camera_Movement::LOOK_LEFT , deltaTime);
		if (glfwGetKey(window, GLFW_KEY_RIGHT)	== GLFW_PRESS) cameraObject.processInput(Camera_Movement::LOOK_RIGHT , deltaTime);
	}
}

// DROPPING MODEL FILES
void dropCallback(GLFWwindow* window, int count, const char** paths) {
	WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx || !ctx->scene) return;


	for (int i = 0; i < count; i++) {
		std::filesystem::path filePath(paths[i]);
		std::string ext = filePath.extension().string();

		if (ext == ".gltf" || ext == ".glb" || ext == ".obj") {
			ctx->scene->queueModelLoad(filePath);
		}
	}
}