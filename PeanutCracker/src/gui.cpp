#include "headers/gui.h"
#include "headers/scene.h"
#include "headers/camera.h"
#include "headers/light.h"
#include "headers/object.h"
#include "headers/transform.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp> 

#include <array>


GUI::GUI(GLFWwindow* window, const char* glsl_version) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	setPurpleTheme();
}

GUI::~GUI() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

// Returns the available viewport size
ImVec2 GUI::update(float deltaTime, Camera& camera, Scene& scene, unsigned int textureID) {
	float screenWidth = ImGui::GetIO().DisplaySize.x;
	float screenHeight = ImGui::GetIO().DisplaySize.y;
	float statusBarHeight = 25.0f;
	float toolbarHeight = 0.0f;		// change this when adding toolbar
	float viewportHeight = screenHeight - statusBarHeight - toolbarHeight;
	float rightPanelHeight = viewportHeight;
	float inspectorHeight = rightPanelHeight * 0.65f;
	float sceneConfigurationHeight = rightPanelHeight - inspectorHeight;

	// ImGui frame init
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	if (ImGui::IsKeyPressed(ImGuiKey_1)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed(ImGuiKey_2)) mCurrentGizmoOperation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed(ImGuiKey_3)) mCurrentGizmoOperation = ImGuizmo::SCALE;


	// === INSPECTOR PANEL ======================================================
	ImGui::SetNextWindowPos(ImVec2(screenWidth - panelWidth, toolbarHeight));
	ImGui::SetNextWindowSize(ImVec2(panelWidth, inspectorHeight));

	ImGuiWindowFlags inspectorFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

	ImGui::Begin("Inspector", NULL, inspectorFlags);
	panelWidth = ImGui::GetWindowWidth(); // Capture resizing

	// --Light
	//if (ImGui::CollapsingHeader("Scene Light", ImGuiTreeNodeFlags_DefaultOpen)) {
	//    ImGui::PushID("Light");
	//    ImGui::DragFloat3("Position", glm::value_ptr(light.position), 0.1f);
	//    ImGui::ColorEdit3("Color", glm::value_ptr(light.light.diffuse));
	//    ImGui::PopID();
	//}

	//ImGui::Separator

	// -- Objects
	int itemID = 0;
	for (auto* node : scene.selectedEntities) {
		if (!node) continue;

		ImGui::PushID(itemID);

		ImGui::TextWrapped(node->name.c_str());

		// -- Object Path
		if (node->object && node->object->modelPtr) {

			char pathBuffer[256];
			strcpy_s(pathBuffer, 256, node->object->modelPtr->path.c_str());
			if (ImGui::InputText("Path", pathBuffer, IM_ARRAYSIZE(pathBuffer))) {
				node->object->modelPtr->path = pathBuffer;
				// Handle model reloading logic here
			}

			if (pathErrorState) {
				ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(100, 0, 0, 255));
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid Path!");
			}
		}

		// -- Node Transformation
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

			// --Position
			ImGui::SeparatorText("POSITION");
			ImGui::Indent();
			glm::vec3 positionDisplay = node->getPosition();
			widgetStretch([&]() {
				if (ImGui::DragFloat3("##Position", glm::value_ptr(positionDisplay), 0.05f)) {
					node->setPosition(positionDisplay);
				}
				});
			ImGui::Unindent();

			// --Scale
			ImGui::SeparatorText("SCALE");
			ImGui::Indent();
			ImGui::Checkbox("##UniformLock", &mUniformScale);
			ImGui::SameLine();
			glm::vec3 scaleDisplay = node->getScale();
			glm::vec3 tempScale = scaleDisplay;
			widgetStretch([&]() {
				if (ImGui::DragFloat3("##Scale", glm::value_ptr(tempScale), 0.05f)) {
					if (mUniformScale) {
						const float EPSILON = 0.0001f;

						if (std::abs(tempScale.x - scaleDisplay.x) > 0.0f) {
							if (std::abs(scaleDisplay.x) < EPSILON) {
								tempScale.y = tempScale.x;
								tempScale.z = tempScale.x;
							}
							else {
								float factor = tempScale.x / scaleDisplay.x;
								tempScale.y = scaleDisplay.y * factor;
								tempScale.z = scaleDisplay.z * factor;
							}
						}
						else if (std::abs(tempScale.y - scaleDisplay.y) > 0.0f) {
							if (std::abs(scaleDisplay.y) < EPSILON) {
								tempScale.x = tempScale.y;
								tempScale.z = tempScale.y;
							}
							else {
								float factor = tempScale.y / scaleDisplay.y;
								tempScale.x = scaleDisplay.x * factor;
								tempScale.z = scaleDisplay.z * factor;
							}
						}
						else if (std::abs(tempScale.z - scaleDisplay.z) > 0.0f) {
							if (std::abs(scaleDisplay.z) < EPSILON) {
								tempScale.x = tempScale.z;
								tempScale.y = tempScale.z;
							}
							else {
								float factor = tempScale.z / scaleDisplay.z;
								tempScale.x = scaleDisplay.x * factor;
								tempScale.y = scaleDisplay.y * factor;
							}
						}
					}
					node->setScale(tempScale);
				}
				});
			ImGui::Unindent();

			// --Rotation
			ImGui::SeparatorText("ROTATION");
			ImGui::Indent();
			glm::vec3 eulerRotationDisplay = node->getEulerRotation();
			widgetStretch([&]() {
				if (ImGui::DragFloat3("##Rotation", glm::value_ptr(eulerRotationDisplay), 0.05f)) {
					node->setEulerRotation(eulerRotationDisplay);
				}
				});
			ImGui::Unindent();
		}

		ImGui::PopID();
		itemID++;
	}
	ImGui::End();


	// === SCENE CONFIGURATION PANEL =====================================================
	ImGui::SetNextWindowPos(ImVec2(screenWidth - panelWidth, toolbarHeight + inspectorHeight));
	ImGui::SetNextWindowSize(ImVec2(panelWidth, sceneConfigurationHeight));
	ImGui::Begin("Scene Configuration", NULL, inspectorFlags);

	//if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
	//	ImGui::ColorEdit3("Ambient Color", glm::value_ptr(scene.ambientColor));
	//	ImGui::SliderFloat("Skybox Intensity", &scene.skyboxIntensity, 0.0f, 5.0f);
	//}

	// --Rendering Pipeline
	if (ImGui::CollapsingHeader("Rendering Pipeline", ImGuiTreeNodeFlags_DefaultOpen)) {
		const char* modes[] = { "Standard Diffuse", "IBL", "Wireframe" };
		static int renderMode = 0;
		ImGui::Combo("Render Mode", &renderMode, modes, IM_ARRAYSIZE(modes));
		switch (renderMode) {
		case 0: scene.setRenderMode(Render_Mode::STANDARD_DIFFUSE); break;
		case 1: scene.setRenderMode(Render_Mode::IMAGE_BASED_LIGHTING); break;
		case 2: scene.setRenderMode(Render_Mode::WIREFRAME); break;
		default: break;
		}

		static bool checkboxPlaceholder = false;
		ImGui::Checkbox("Enable Shadow Mapping", &checkboxPlaceholder);
	}
	// --Environment
	if (ImGui::CollapsingHeader("Environment")) {
		// --Skybox
		ImGui::SeparatorText("SKYBOX");
		ImGui::Indent();
		static char skyboxDirBuffer[512] = "";

		ImGui::Text("Enter Directory Path:");
		widgetStretch([&]() {
			ImGui::InputText("##SkyboxDir", skyboxDirBuffer, IM_ARRAYSIZE(skyboxDirBuffer));
			});

		if (ImGui::Button("Load Skybox from Folder", ImVec2(-FLT_MIN, 0))) {
			scene.createAndAddSkyboxFromDirectory(skyboxDirBuffer);
		}
		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
		ImGui::TextWrapped("Expected files: right, left, top, bottom, front, back (jpg/png)");
		ImGui::PopStyleColor();
		ImGui::Unindent();

		// --Directional light
		ImGui::Spacing();
		ImGui::SeparatorText("DIRECTIONAL LIGHT");
		for (auto& dirLight : scene.directionalLights) {

			ImGui::Indent();
			ImGui::Text("Direction (World Space)");
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Direction the light points toward");

			glm::vec3 tempDir = dirLight->direction;
			widgetStretch([&]() {
				if (ImGui::DragFloat3("##Direction", glm::value_ptr(tempDir), 0.05f)) {
					dirLight->direction = glm::normalize(tempDir);
				}
				});

			//ImGui::Spacing();
			ImGui::Separator();
			//ImGui::Spacing();

			ImGui::Text("Frustum (Light Space)");

			std::array<float, 6> planes = dirLight->shadowCasterComponent->getPlanes();
			bool changed = false;

			if (ImGui::BeginTable("FrustumPlanes", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableNextColumn();
				ImGui::Text("L/R");
				ImGui::TableNextColumn();
				changed |= ImGui::DragFloat2("##LR", &planes[0], 0.1f);

				ImGui::TableNextColumn();
				ImGui::Text("B/T");
				ImGui::TableNextColumn();
				changed |= ImGui::DragFloat2("##BT", &planes[2], 0.1f);

				ImGui::TableNextColumn();
				ImGui::Text("N/F");
				ImGui::TableNextColumn();
				changed |= ImGui::DragFloat2("##NF", &planes[4], 0.1f);

				ImGui::EndTable();
			}

			if (changed) {
				// Validation prevents NaN matrices
				if (planes[0] < planes[1] && planes[2] < planes[3] && planes[4] < planes[5]) {
					dirLight->shadowCasterComponent->setFrustumPlanes(
						planes[0], planes[1], planes[2], planes[3], planes[4], planes[5]
					);
				}
				else {
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid: Min must be < Max!");
				}
			}

			// Presets
			ImGui::Spacing();
			if (ImGui::Button("10m^3")) {
				dirLight->shadowCasterComponent->setFrustumPlanes(-5, 5, -5, 5, 0.1f, 20);
			}
			ImGui::SameLine();
			if (ImGui::Button("50m^3")) {
				dirLight->shadowCasterComponent->setFrustumPlanes(-25, 25, -25, 25, 0.1f, 50);
			}
			ImGui::SameLine();
			if (ImGui::Button("100m^2")) {
				dirLight->shadowCasterComponent->setFrustumPlanes(-50, 50, -50, 50, 0.1f, 100);
			}


			ImGui::Unindent();
		}
	}

	ImGui::End();


	// === VIEWPORT PANEL ===========================================================
	ImGui::SetNextWindowPos(ImVec2(0, toolbarHeight));
	ImGui::SetNextWindowSize(ImVec2(screenWidth - panelWidth, viewportHeight));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar;

	// Capture current state
	ImGui::Begin("Viewport", NULL, viewportFlags);
	this->isViewportHovered = ImGui::IsWindowHovered();
	this->viewportSize = ImGui::GetContentRegionAvail();
	this->viewportBoundsMin = ImGui::GetCursorScreenPos();

	// Draw the FBO texture
	if (textureID != 0) {
		ImGui::Image((void*)(intptr_t)textureID, this->viewportSize, ImVec2(0, 1), ImVec2(1, 0));
	}

	// CAMERA MATRICES
	glm::mat4 view = camera.getViewMat();
	float aspect = viewportSize.x / viewportSize.y;
	glm::mat4 proj = camera.getProjMat(aspect);

	// === GIZMO ======================================================================
	if (!scene.selectedEntities.empty()) {
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(viewportBoundsMin.x, viewportBoundsMin.y, viewportSize.x, viewportSize.y);

		// Select the first object in the selection
		auto selectedNode = scene.selectedEntities[0];
		glm::mat4 modelMatrix = selectedNode->worldMatrix;

		// Draw and Interact
		ImGuizmo::Manipulate(
			glm::value_ptr(view),
			glm::value_ptr(proj),
			mCurrentGizmoOperation, // Ensure this member exists (TRANSLATE by default)
			ImGuizmo::LOCAL,
			glm::value_ptr(modelMatrix)
		);

		// If the user is dragging the gizmo, update the object
		if (ImGuizmo::IsUsing()) {
			if (selectedNode->parent) {
				glm::mat4 invParent = glm::inverse(selectedNode->parent->worldMatrix);
				glm::mat4 localMatrix = invParent * modelMatrix;
				selectedNode->updateFromMatrix(localMatrix); // Decompose into Pos/Rot/Scale
			}
			else {
				selectedNode->updateFromMatrix(modelMatrix);
			}
			selectedNode->isDirty = true;
		}
	}


	// === VIEW ===========================================================================
	ImGuizmo::SetDrawlist();
	float viewManipulateRight = ImGui::GetWindowPos().x + ImGui::GetWindowWidth();
	float viewManipulateTop = ImGui::GetWindowPos().y;

	// After ViewManipulate call
	ImVec2 gizmoPos(viewManipulateRight - 96, viewManipulateTop);

	ImGuizmo::ViewManipulate(glm::value_ptr(view), 10.0f, ImVec2(viewManipulateRight - 96, viewManipulateTop), ImVec2(96, 96), 0x10101010);

	// Now draw axis labels
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 center = ImVec2(gizmoPos.x + 48, gizmoPos.y + 48); // Center of gizmo

	// Extract rotation from view matrix
	glm::mat3 rotation = glm::mat3(view);

	// Define axis directions in world space
	glm::vec3 axes[] = {
		glm::vec3(1, 0, 0),  // +X
		glm::vec3(-1, 0, 0), // -X
		glm::vec3(0, 1, 0),  // +Y
		glm::vec3(0, -1, 0), // -Y
		glm::vec3(0, 0, -1),  // -Z
		glm::vec3(0, 0, 1) // +Z
	};

	const char* labels[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
	ImU32 colors[] = {
		IM_COL32(255, 0, 0, 255),   // +X red
		IM_COL32(255, 0, 0, 255),   // -X dark red
		IM_COL32(0, 255, 0, 255),   // +Y green
		IM_COL32(0, 255, 0, 255),   // -Y dark green
		IM_COL32(0, 0, 255, 255),   // +Z blue
		IM_COL32(0, 0, 255, 255)    // -Z dark blue
	};

	// Project each axis to screen space
	for (int i = 0; i < 6; i++) {
		// Transform axis by view rotation (invert because view matrix)
		glm::vec3 viewAxis = glm::transpose(rotation) * axes[i];

		// Skip if pointing away from camera (negative Z in view space)
		if (viewAxis.z > 0) continue; // Facing away

		// Project to 2D (simple orthographic projection)
		float scale = 24.0f; // Distance from center
		ImVec2 labelPos = ImVec2(
			center.x + viewAxis.x * scale,
			center.y - viewAxis.y * scale  // Flip Y for screen coords
		);

		// Draw label
		drawList->AddText(labelPos, colors[i], labels[i]);
	}
	ImGui::End();
	ImGui::PopStyleVar();


	// === STATUS BAR ==================================================================
	showStatusBar(statusBarHeight, camera);


	// === TOOLBAR =====================================================================
	//showToolbar(toolbarHeight);


	return viewportSize;
}

void GUI::render() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GUI::setPurpleTheme() const {
	// Grab the current style
	auto& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// --Palette
	ImVec4 purple = ImVec4(0.44f, 0.22f, 1.00f, 1.00f);
	ImVec4 lightPurple = ImVec4(0.54f, 0.36f, 1.00f, 1.00f);
	ImVec4 darkPurple = ImVec4(0.35f, 0.15f, 0.80f, 1.00f);
	ImVec4 darkBg = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
	ImVec4 darkerBg = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	ImVec4 peanutYellow = ImVec4(1.00f, 0.84f, 0.30f, 1.00f);

	// --Applying palette
	// Window Backgrounds
	colors[ImGuiCol_WindowBg] = darkerBg;
	colors[ImGuiCol_ChildBg] = darkerBg;
	colors[ImGuiCol_PopupBg] = darkBg;

	// Frame Backgrounds (Input boxes, sliders, checkboxes)
	colors[ImGuiCol_FrameBg] = darkBg;
	colors[ImGuiCol_FrameBgHovered] = ImVec4(purple.x, purple.y, purple.z, 0.40f); // Semi-transparent hover
	colors[ImGuiCol_FrameBgActive] = darkPurple;

	// Title Bars
	colors[ImGuiCol_TitleBg] = darkBg;
	colors[ImGuiCol_TitleBgActive] = ImVec4(purple.x, purple.y, purple.z, 0.70f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(purple.x, purple.y, purple.z, 0.40f);

	// Headers (Collapsing Headers, Selectables)
	colors[ImGuiCol_Header] = ImVec4(purple.x, purple.y, purple.z, 0.45f);
	colors[ImGuiCol_HeaderHovered] = lightPurple;
	colors[ImGuiCol_HeaderActive] = darkPurple;

	// Buttons
	colors[ImGuiCol_Button] = ImVec4(purple.x, purple.y, purple.z, 0.80f);
	colors[ImGuiCol_ButtonHovered] = lightPurple;
	colors[ImGuiCol_ButtonActive] = darkPurple;

	// Checkmark/Slider Grab
	colors[ImGuiCol_CheckMark] = lightPurple;
	colors[ImGuiCol_SliderGrab] = lightPurple;
	colors[ImGuiCol_SliderGrabActive] = darkPurple;

	// Resize Grip (for resizable windows)
	colors[ImGuiCol_ResizeGrip] = darkPurple;
	colors[ImGuiCol_ResizeGripHovered] = lightPurple;
	colors[ImGuiCol_ResizeGripActive] = darkPurple;

	// Scrollbar
	colors[ImGuiCol_ScrollbarBg] = darkBg;
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);

	// Text (usually left white/near-white)
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_TextDisabled] = peanutYellow;
}

// STATUS BAR
void GUI::showStatusBar(int statusbarHeight, Camera& camera) const{
	float width = ImGui::GetIO().DisplaySize.x;
	float height = ImGui::GetIO().DisplaySize.y;
	ImGui::SetNextWindowPos(ImVec2(0, height - statusbarHeight));
	ImGui::SetNextWindowSize(ImVec2(width, statusbarHeight));
	ImGui::Begin("##StatusBar", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs);
	ImGui::Text("v0.2 | FPS: %.1f | Zoom: %.1f | P(%.1f, %.1f, %.1f) D(%.1f, %.1f, %.1f)",
		ImGui::GetIO().Framerate, camera.zoom, camera.position.x, camera.position.y, camera.position.z, camera.front.x, camera.front.y, camera.front.z);

	//ImGui::SameLine();
	//ImGui::Text("cam(x/y/z): ");
	//ImGui::SameLine();
	//ImGui::DragFloat3("##CamPos", glm::value_ptr(camera.position), 0.1f);
	//ImGui::SameLine();
	//ImGui::Text("cam(p/y): ");
	//ImGui::SameLine();
	//float pitchYaw[2] = { camera.pitch, camera.yaw };
	//if (ImGui::DragFloat2("##Cam", pitchYaw, 0.1f)) {
	//	camera.setPitchYaw(pitchYaw[0], pitchYaw[1]);
	//}


	ImGui::End();
}