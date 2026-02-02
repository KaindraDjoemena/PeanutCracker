#pragma once

#define GLM_ENABLE_EXPERIMENTAL 

#include "scene.h"
#include "camera.h"
#include "light.h"
#include "object.h"
#include "transform.h"
#include "renderer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <ImGuizmo.h>

#include <imgui_impl_opengl3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp> 

#include <array>


template<typename T>
void widgetStretch(T&& func) {
	ImGui::PushItemWidth(-FLT_MIN);
	func();
	ImGui::PopItemWidth();
}

class GUI {
public:
	float panelWidth = 300.0f;
	bool isViewportHovered = false;
	bool pathErrorState = false;

	ImVec2 viewportBoundsMin;
	ImVec2 viewportSize;


	GUI(GLFWwindow* window, const char* glsl_version);

	~GUI();

	// Returns the available viewport size
	ImVec2 update(float deltaTime, Camera& camera, Scene& scene, Renderer& renderer, unsigned int textureID);

	void render();

private:
	bool mUniformScale = false;

	// IMGUIZMO
	ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::LOCAL;

	void setPurpleTheme() const;

	// STATUS BAR
	void showStatusBar(int statusbarHeight, Camera& camera) const;
};