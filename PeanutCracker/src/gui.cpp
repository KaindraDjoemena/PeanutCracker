#include "headers/gui.h"
#include "headers/scene.h"
#include "headers/camera.h"
#include "headers/light.h"
#include "headers/object.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp> 


GUI::GUI(GLFWwindow* window, const char* glsl_version) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";
    
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

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
ImVec2 GUI::update(float deltaTime, GLFWwindow* window, Camera& camera, Scene& scene, Renderer& renderer, unsigned int textureID) {
    float screenWidth = ImGui::GetIO().DisplaySize.x;
    float screenHeight = ImGui::GetIO().DisplaySize.y;
    float statusBarHeight = 25.0f;

    // ImGui frame init
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(viewport->ID, viewport,
        ImGuiDockNodeFlags_PassthruCentralNode|
        ImGuiDockNodeFlags_NoDockingOverCentralNode |
        ImGuiTreeNodeFlags_CollapsingHeader
    );

    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        // Split the dockspace
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, NULL, &dock_main_id);
        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, NULL, &dock_main_id);
        ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.20f, NULL, &dock_main_id);

        // Dock windows
        //ImGui::DockBuilderDockWindow("INSPECTOR", dock_right_id);
        //ImGui::DockBuilderDockWindow("Configuration", dock_right_id);
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id); // Central node
        ImGui::DockBuilderFinish(dockspace_id);
    }

    // TODO: WINDOWING CLASS
    //if (ImGui::IsAnyItemActive()) {
    //    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !ImGui::GetIO().WantTextInput) {
    //        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //    }
    //} 
    //else {
    //    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    //}


    /* ===== INSPECTOR PANEL ====================================================== */
    ImGui::Begin("INSPECTOR", NULL);
    panelWidth = ImGui::GetWindowWidth();

    if (ImGui::BeginTabBar("InspectorTabs")) {
        if (ImGui::BeginTabItem("Selection")) {
            // --Objects
            int itemID = 0;
            for (auto* node : scene.getSelectedEnts()) {
                if (!node) continue;

                ImGui::PushID(itemID);

                ImGui::TextWrapped(node->name.c_str());

                // --Object Path
                if (node->object && node->object->modelPtr) {

                    char pathBuffer[256];
                    strcpy_s(pathBuffer, 256, node->object->modelPtr->path.c_str());
                    if (ImGui::InputText("Path", pathBuffer, IM_ARRAYSIZE(pathBuffer))) {
                        node->object->modelPtr->path = pathBuffer;
                    }

                    if (pathErrorState) {
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(100, 0, 0, 255));
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid Path!");
                    }
                }

                // --Node Transformation
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // --Position
                    glm::vec3 pos = node->getPosition();
                    DrawProperty("Pos", [&]() {
                        if (ImGui::DragFloat3("##Position", glm::value_ptr(pos), 0.05f)) {
                            node->setPosition(pos);
                        }
                        });

                    // --Scale
                    glm::vec3 scaleDisplay = node->getScale();
                    glm::vec3 tempScale = scaleDisplay;
                    DrawProperty("Scale", [&]() {
                        ImGui::Checkbox("##UniformLock", &mUniformScale);
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(-1.0f);

                        if (ImGui::DragFloat3("##Scale", glm::value_ptr(tempScale), 0.05f)) {
                            node->setScale(tempScale, mUniformScale);
                        }
                    });

                    // --Rotation
                    glm::vec3 rot = node->getEulerRotation();
                    DrawProperty("Rot", [&]() {
                        if (ImGui::DragFloat3("##Rotation", glm::value_ptr(rot), 0.05f)) {
                            node->setEulerRotation(rot);
                        }
                    });
                }

                // --Textures Section
                //if (node->object && node->object->modelPtr) {
                //    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {

                //        float thumbSize = 64.0f;
                //        float padding = 8.0f;
                //        float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

                //        for (int i = 0; i < node->object->modelPtr->textures_loaded.size(); i++) {
                //            auto& tex = node->object->modelPtr->textures_loaded[i];

                //            ImGui::PushID(i);
                //            ImGui::BeginGroup();

                //            ImGui::TextDisabled("%s", tex.type.c_str());

                //            // Thumbnail
                //            ImTextureID texID = (ImTextureID)(intptr_t)tex.id;
                //            ImGui::Image(texID, ImVec2(thumbSize, thumbSize), ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f));
                //            if (ImGui::IsItemHovered()) {
                //                ImGui::BeginTooltip();
                //                ImGui::Text("ID: %d", tex.id);
                //                ImGui::Text("Path: %s", tex.path.c_str());
                //                ImGui::Image(texID, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
                //                ImGui::EndTooltip();
                //            }
                //            ImGui::EndGroup();

                //            float lastX2 = ImGui::GetItemRectMax().x;
                //            float nextX2 = lastX2 + padding + thumbSize;
                //            if (i + 1 < node->object->modelPtr->material.size() && nextX2 < windowVisibleX2)
                //                ImGui::SameLine();

                //            ImGui::PopID();
                //        }
                //    }
                //}
                ImGui::PopID();
                itemID++;
            }
            ImGui::EndTabItem();
        }

        //--Lights
        if (ImGui::BeginTabItem("Lights")) {
            if (ImGui::BeginTabBar("LightingTabs")) {

                // --Directional Lights
                static int selectedDir = 0;
                if (ImGui::BeginTabItem("Directional")) {
                    ImGui::BeginGroup();
                    ImGui::BeginChild("DirList", ImVec2(130, -70), true);
                    for (int i = 0; i < (int)scene.getDirectionalLights().size(); i++) {
                        char label[32]; sprintf(label, "Dir %d", i);
                        if (ImGui::Selectable(label, selectedDir == i)) selectedDir = i;
                    }
                    ImGui::EndChild();
                    // --Add button
                    bool isMaxDir = (scene.getDirectionalLights().size() == MAX_LIGHTS);
                    bool isEmptyDir = (scene.getDirectionalLights().empty());
                    if (isMaxDir) { ImGui::BeginDisabled(); }
                    if (ImGui::Button("(+)", ImVec2(130, 0))) {
                        scene.createAndAddDirectionalLight(std::make_unique<DirectionalLight>());
                        selectedDir = (int)scene.getDirectionalLights().size() - 1;
                    }
                    if (isMaxDir) { ImGui::EndDisabled(); }
                    // --Delete button
                    if (isEmptyDir) { ImGui::BeginDisabled(); }
                    if (ImGui::Button("(-)", ImVec2(130, 0))) {
                        if (selectedDir >= 0 && selectedDir < (int)scene.getDirectionalLights().size()) {
                            scene.deleteDirLight(selectedDir);
                            selectedDir--;
                        }
                    }
                    if (isEmptyDir) { ImGui::EndDisabled(); }
                    ImGui::EndGroup();
                    ImGui::SameLine();

                    ImGui::BeginChild("DirDetails", ImVec2(0, 0), false);
                    if (selectedDir >= 0 && selectedDir < (int)scene.getDirectionalLights().size()) {
                        auto& l = *scene.getDirectionalLights()[selectedDir];

                        // Debug wireframe visibility
                        ImGui::Checkbox("Visible Area", &l.isVisible);

                        // Transform
                        ImGui::SeparatorText("Transform");
                        DrawProperty("Dir", [&]() { if (ImGui::DragFloat3("##d", &l.direction.x, 0.01f)) { l.direction = glm::normalize(l.direction); } });

                        // Properties
                        ImGui::SeparatorText("Properties");
                        DrawProperty("Col", [&]() { ImGui::ColorEdit3("##c", &l.light.color.x); });
                        DrawProperty("Pow", [&]() { ImGui::SliderFloat("##pow", &l.light.power, 0.0f, 1000.0f); });

                        // Range
                        ImGui::SeparatorText("Range");
                        DrawProperty("Range", [&]() { if (ImGui::SliderFloat("##range", &l.range, 0.1f, 1000.0f)) { l.shadowCasterComponent.setFarPlane(l.range); } });

                        // Shadow Bias
                        ImGui::SeparatorText("Shadow Bias");
                        DrawProperty("Normal", [&]() { ImGui::DragFloat("##nbias", &l.light.normalBias, 0.00001f, 0.00001f, 0.1f, "%.5f");  });
                        DrawProperty("Depth",  [&]() { ImGui::DragFloat("##dbias", &l.light.depthBias, 0.00001f, 0.00001f, 0.1f, "%.5f");  });

                        ImGui::Spacing();
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                // --Point Lights
                static int selectedPoint = 0;
                if (ImGui::BeginTabItem("Point")) {
                    ImGui::BeginGroup();
                    ImGui::BeginChild("PointList", ImVec2(130, -70), true);
                    for (int i = 0; i < (int)scene.getPointLights().size(); i++) {
                        char label[32]; sprintf(label, "Point %d", i);
                        if (ImGui::Selectable(label, selectedPoint == i)) selectedPoint = i;
                    }
                    ImGui::EndChild();
                    // --Add button
                    bool isMaxPoint = (scene.getPointLights().size() >= MAX_LIGHTS);
                    bool isEmptyPoint = (scene.getPointLights().empty());
                    if (isMaxPoint) { ImGui::BeginDisabled(); }
                    if (ImGui::Button("(+)", ImVec2(130, 0))) {
                        scene.createAndAddPointLight(std::make_unique<PointLight>());
                        selectedPoint = (int)scene.getPointLights().size() - 1;
                    }
                    if (isMaxPoint) { ImGui::EndDisabled(); }
                    // --Delete button
                    if (isEmptyPoint) { ImGui::BeginDisabled(); }
                    if (ImGui::Button("(-)", ImVec2(130, 0))) {
                        if (selectedPoint >= 0 && selectedPoint < (int)scene.getPointLights().size()) {
                            scene.deletePointLight(selectedPoint);
                            selectedPoint--;
                        }
                    }
                    if (isEmptyPoint) { ImGui::EndDisabled(); }
                    ImGui::EndGroup();
                    ImGui::SameLine();

                    ImGui::BeginChild("PointDetails", ImVec2(0, 0), false);
                    if (selectedPoint >= 0 && selectedPoint < (int)scene.getPointLights().size()) {
                        auto& l = *scene.getPointLights()[selectedPoint];

                        // Debug wireframe visibility
                        ImGui::Checkbox("Visible Area", &l.isVisible);

                        // Transform
                        ImGui::SeparatorText("Transform");
                        DrawProperty("Pos", [&]() { ImGui::DragFloat3("##p", &l.position.x, 0.01f); });

                        // Properties
                        ImGui::SeparatorText("Properties");
                        DrawProperty("Col", [&]() { ImGui::ColorEdit3("##a", &l.light.color.x); });
                        DrawProperty("Pow", [&]() { ImGui::SliderFloat("##pow", &l.light.power, 0.0f, 1000.0f); });

                        // Radius
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::SeparatorText("Radius");
                        DrawProperty("Rad", [&]() { if (ImGui::SliderFloat("##rad", &l.radius, 0.1f, 1000.0f)) { l.shadowCasterComponent.setFarPlane(l.radius); } });

                        // Shadow Bias
                        ImGui::SeparatorText("Shadow Bias");
                        DrawProperty("Normal", [&]() { ImGui::DragFloat("##nbias", &l.light.normalBias, 0.00001f, 0.00001f, 0.1f, "%.5f");  });
                        DrawProperty("Depth",  [&]() { ImGui::DragFloat("##dbias", &l.light.depthBias, 0.00001f, 0.00001f, 0.1f, "%.5f");  });

                        // Frustum
                        //float nearP = l.shadowCasterComponent.getNearPlane();
                        //float farP  = l.shadowCasterComponent.getFarPlane();
                        //ImGui::SeparatorText("Frustum Planes");
                        //DrawProperty("Near", [&]() { if (ImGui::DragFloat("##n", &nearP, 0.1f, 0.01f, 10.0f)) { l.shadowCasterComponent->setNearPlane(nearP); }});
                        //DrawProperty("Far", [&]() { if (ImGui::DragFloat("##f", &farP, 1.0f, 0.1f, 1000.0f)) { l.shadowCasterComponent->setFarPlane(farP); }});

                        //ImGui::Spacing();
                        //DrawAttenuationGraph(nearP, farP, l.attenuation);
                        //ImGui::Spacing();
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                // --Spot Lights
                static int selectedSpot = 0;
                if (ImGui::BeginTabItem("Spot")) {
                    ImGui::BeginGroup();
                    ImGui::BeginChild("SpotList", ImVec2(130, -70), true);
                    for (int i = 0; i < (int)scene.getSpotLights().size(); i++) {
                        char label[32]; sprintf(label, "Spot %d", i);
                        if (ImGui::Selectable(label, selectedSpot == i)) selectedSpot = i;
                    }
                    ImGui::EndChild();
                    // --Add button
                    bool isMaxSpot = (scene.getSpotLights().size() == MAX_LIGHTS);
                    bool isEmptySpot = (scene.getSpotLights().empty());
                    if (isMaxSpot) { ImGui::BeginDisabled(); }
                    if (ImGui::Button("(+)", ImVec2(130, 0))) {
                        scene.createAndAddSpotLight(std::make_unique<SpotLight>());
                        selectedSpot = (int)scene.getSpotLights().size() - 1;
                    }
                    if (isMaxSpot) { ImGui::EndDisabled(); }
                    // --Delete button
                    if (isEmptySpot) { ImGui::BeginDisabled(); }
                    if (ImGui::Button("(-)", ImVec2(130, 0))) {
                        if (selectedSpot >= 0 && selectedSpot < (int)scene.getSpotLights().size()) {
                            scene.deleteSpotLight(selectedSpot);
                            selectedSpot--;
                        }
                    }
                    if (isEmptySpot) { ImGui::EndDisabled(); }
                    ImGui::EndGroup();
                    ImGui::SameLine();

                    ImGui::BeginChild("SpotDetails", ImVec2(0, 0), false);
                    if (selectedSpot >= 0 && selectedSpot < (int)scene.getSpotLights().size()) {
                        auto& l = *scene.getSpotLights()[selectedSpot];

                        // Debug wireframe visibility
                        ImGui::Checkbox("Visible Area", &l.isVisible);

                        // Transform
                        ImGui::Separator();
                        DrawProperty("Pos", [&]() { ImGui::DragFloat3("##p", &l.position.x, 0.01f); });
                        DrawProperty("Dir", [&]() { if (ImGui::DragFloat3("##d", &l.direction.x, 0.01f)) { l.direction = glm::normalize(l.direction); } });

                        // Angles
                        ImGui::SeparatorText("Light Cone Angles");
                        ImGui::Separator();
                        float iDeg = glm::degrees(acos(l.inCosCutoff));
                        float oDeg = glm::degrees(acos(l.outCosCutoff));
                        DrawProperty("Inner", [&]() { if (ImGui::SliderFloat("##i", &iDeg, 0.0f, oDeg)) { l.inCosCutoff = cosf(glm::radians(iDeg)); } });
                        DrawProperty("Outer", [&]() { if (ImGui::SliderFloat("##o", &oDeg, iDeg, 90.0f)) { l.outCosCutoff = cosf(glm::radians(oDeg)); l.shadowCasterComponent.setFOVDeg(oDeg); } });

                        // Colors
                        ImGui::SeparatorText("Colors");
                        DrawProperty("Col", [&]() { ImGui::ColorEdit3("##a", &l.light.color.x); });
                        DrawProperty("Pow", [&]() { ImGui::SliderFloat("##pow", &l.light.power, 0.0f, 1000.0f); });

                        // Range
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::SeparatorText("Range");
                        DrawProperty("Range", [&]() { if (ImGui::SliderFloat("##range", &l.range, 0.01f, 1000.0f)) { l.shadowCasterComponent.setFarPlane(l.range); } });

                        // Shadow Bias
                        ImGui::SeparatorText("Shadow Bias");
                        DrawProperty("Normal", [&]() { ImGui::DragFloat("##nbias", &l.light.normalBias, 0.00001f, 0.00001f, 0.1f, "%.5f");  });
                        DrawProperty("Depth",  [&]() { ImGui::DragFloat("##dbias", &l.light.depthBias, 0.00001f, 0.00001f, 0.1f, "%.5f");  });

                        // Frustum
                        //ImGui::SeparatorText("Frustum Planes");
                        //float nearP = l.shadowCasterComponent->getNearPlane();
                        //float farP  = l.shadowCasterComponent->getFarPlane();
                        //DrawProperty("Near", [&]() { if (ImGui::DragFloat("##n", &nearP, 0.1f, 0.01f, 10.0f)) { l.shadowCasterComponent->setNearPlane(nearP); }});
                        //DrawProperty("Far",  [&]() { if (ImGui::DragFloat("##f", &farP, 1.0f, 0.1f, 1000.0f)) { l.shadowCasterComponent->setFarPlane(farP); }});

                        //ImGui::Spacing();
                        //DrawAttenuationGraph(nearP, farP, l.attenuation);
                        //ImGui::Spacing();
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndTabItem();
        }

        //--Probes
        if (ImGui::BeginTabItem("Probes")) {
            static int selectedProbe = 0;

            ImGui::BeginGroup();
            ImGui::BeginChild("ProbeList", ImVec2(130, -70), true);
            for (int i = 0; i < (int)scene.getRefProbes().size(); i++) {
                char label[32]; sprintf(label, "Probe %d", i);
                if (ImGui::Selectable(label, selectedProbe == i)) selectedProbe = i;
            }
            ImGui::EndChild();

            bool isMax = (scene.getRefProbes().size() >= MAX_LIGHTS);
            bool isEmpty = (scene.getRefProbes().empty());

            if (isMax) { ImGui::BeginDisabled(); }
            if (ImGui::Button("(+)", ImVec2(130, 0))) {
                scene.createAndAddReflectionProbe(std::make_unique<RefProbe>(scene.getConvolutionShader(), scene.getConversionShader(), scene.getPrefilterShader()));
                selectedProbe = (int)scene.getRefProbes().size() - 1;
            }
            if (isMax) { ImGui::EndDisabled(); }

            if (isEmpty) { ImGui::BeginDisabled(); }
            //if (ImGui::Button("(-)", ImVec2(130, 0))) {
            //    if (selectedProbe >= 0 && selectedProbe < (int)scene.getRefProbes().size()) {
            //        scene.deleteRefProbe(selectedProbe);
            //        selectedProbe--;
            //    }
            //}
            if (isEmpty) { ImGui::EndDisabled(); }
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginChild("ProbeDetails", ImVec2(0, 0), false);
            if (selectedProbe >= 0 && selectedProbe < (int)scene.getRefProbes().size()) {
                auto& p = *scene.getRefProbes()[selectedProbe];

                // Debug wireframe visibility
                ImGui::Checkbox("Visible Proxy Volume", &p.isVisible);

                // Transform
                ImGui::SeparatorText("Transform");
                DrawProperty("Pos", [&]() { ImGui::DragFloat3("##pp", glm::value_ptr(p.transform.position), 0.05f); });
                DrawProperty("Rot", [&]() {
                    glm::vec3 rot = glm::degrees(glm::eulerAngles(p.transform.quatRotation));
                        if (ImGui::DragFloat3("##pr", glm::value_ptr(rot), 0.05f)) {
                            p.transform.setRotDeg(rot);
                        }
                });

                // Proxy object dimensions
                ImGui::SeparatorText("Proxy Volume");
                DrawProperty("Size", [&]() { ImGui::DragFloat3("##ps", glm::value_ptr(p.proxyDims), 0.1f, 0.1f, 1000.0f); });
                DrawProperty("Far", [&]() { ImGui::DragFloat("##pf", &p.farPlane, 1.0f, 0.1f, 10000.0f); });

                ImGui::SeparatorText("Baking");
                if (ImGui::Button("Bake", ImVec2(-FLT_MIN, 0))) {
                    p.toBeBaked = true;
                }
            }
            ImGui::EndChild();

            ImGui::EndTabItem();
        }
    }
    ImGui::EndTabBar();
    ImGui::End();


    /* ===== CONFIGURATION PANEL ==================================================== */
    ImGui::Begin("CONFIGURATION", NULL);

    // --Rendering Pipeline
    if (ImGui::CollapsingHeader("Rendering Pipeline", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* modes[] = { "PBR", "Wireframe" };
        static int renderMode = 0;
        DrawProperty("Mode", [&]() {
            ImGui::Combo("##rm", &renderMode, modes, IM_ARRAYSIZE(modes));
            switch (renderMode) {
            case 0: renderer.setRenderMode(Render_Mode::PBR); break;
            case 1: renderer.setRenderMode(Render_Mode::WIREFRAME); break;
            default: break;
            }
        });

        DrawProperty("BG", [&]() {
            glm::vec3 newBgCol = glm::vec3(renderer.getBgCol());
            if (ImGui::ColorEdit3("##bg", glm::value_ptr(newBgCol))) {
                renderer.setBgCol(glm::vec4(newBgCol, 1.0f));
            }
        });
    }

    // --Environment
    if (ImGui::CollapsingHeader("Environment")) {
        // --Skybox
        ImGui::SeparatorText("SKYBOX");
        ImGui::Indent();
        static char skyboxDirBuffer[512] = "";

        widgetStretch([&]() {
            ImGui::InputText("##SkyboxDir", skyboxDirBuffer, IM_ARRAYSIZE(skyboxDirBuffer));
            });

        if (ImGui::Button("Load .HDR", ImVec2(-FLT_MIN, 0))) {
            bool _validPath = true;

            std::filesystem::path path(skyboxDirBuffer);

            if (!std::filesystem::exists(path)) {
                std::cerr << "ERROR: Skybox path does not exist: " << path << '\n';
                _validPath = false;
            }

            auto ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            const bool isValidExt = (ext == ".hdr" || ext == ".exr" || ext == ".hdri");

            if (!isValidExt) {
                std::cerr << "ERROR: Invalid skybox file\n";
                _validPath = false;
            }


            if (_validPath) {
                scene.createAndAddSkyboxHDR(path);
            }
        }
        if (ImGui::Button("Delete", ImVec2(-FLT_MIN, 0))) {
            scene.deleteSkybox();
        }

        ImGui::Unindent();
    }

    // --Viewport
    if (ImGui::CollapsingHeader("Viewport Post Processing")) {
        DrawProperty("EV", [&]() {
            float ev100 = renderer.getEV100();
            if (ImGui::SliderFloat("##ev", &ev100, -10.0f, 10.0f)) {
                renderer.setEV100(ev100);
            }
            });
        DrawProperty("MSAA", [&]() {
            int msaa = renderer.getViewportFBO()->samples;
            const char* items[] = { "Off", "2x", "4x", "8x", "16x" };
            int current = static_cast<int>(std::log2(msaa));

            if (ImGui::Combo("##msaa", &current, items, IM_ARRAYSIZE(items))) {
                msaa = 1 << current;
                renderer.getViewportFBO()->samples = msaa;
                renderer.getViewportFBO()->rescale(screenWidth, screenHeight);
            }
            });
    }

    ImGui::End();


    // === VIEWPORT PANEL ===========================================================
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_HiddenTabBar;
    ImGui::SetNextWindowClass(&windowClass);

    ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |      // Locks it in place
        ImGuiWindowFlags_NoCollapse |  // Optional: prevent collapsing
        ImGuiWindowFlags_NoScrollbar;

    // Capture current state
    ImGui::Begin("Viewport", NULL, viewportFlags);
    this->isViewportHovered = ImGui::IsWindowHovered();
    ImVec2 newSize = ImGui::GetContentRegionAvail();

    // Resize FBO if panel size changed
    if (newSize.x != this->viewportSize.x || newSize.y != this->viewportSize.y) {
        if (newSize.x > 0 && newSize.y > 0) {
            renderer.getViewportFBO()->rescale(static_cast<int>(newSize.x), static_cast<int>(newSize.y));
        }
    }
    this->viewportSize = ImGui::GetContentRegionAvail();
    this->viewportBoundsMin = ImGui::GetCursorScreenPos();

    // Draw the FBO texture
    if (textureID != 0) {
        ImGui::Image((void*)(intptr_t)textureID, this->viewportSize, ImVec2(0, 1), ImVec2(1, 0));
    }

    // CAMERA MATRICES
    glm::mat4 view = camera.getViewMat();
    float aspect   = viewportSize.x / viewportSize.y;
    glm::mat4 proj = camera.getProjMat(aspect);

    // === GIZMO ======================================================================
    if (ImGui::IsKeyPressed(ImGuiKey_1) && !ImGui::GetIO().WantTextInput) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_2) && !ImGui::GetIO().WantTextInput) mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::IsKeyPressed(ImGuiKey_3) && !ImGui::GetIO().WantTextInput) mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (!scene.getSelectedEnts().empty()) {
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(viewportBoundsMin.x, viewportBoundsMin.y, viewportSize.x, viewportSize.y);

        // Select the first object in the selection
        auto selectedNode = scene.getSelectedEnts()[0];
        glm::mat4 modelMatrix = selectedNode->worldMatrix;

        // Draw and Interact
        ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
            mCurrentGizmoOperation,
            ImGuizmo::LOCAL,
            glm::value_ptr(modelMatrix)
        );

        // If the user is dragging the gizmo, update the object
        if (ImGuizmo::IsUsing()) {
            if (selectedNode->parent) {
                glm::mat4 invParent = glm::inverse(selectedNode->parent->worldMatrix);
                glm::mat4 localMatrix = invParent * modelMatrix;
                selectedNode->updateFromMatrix(localMatrix);
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
    float viewManipulateTop   = ImGui::GetWindowPos().y;

    // After ViewManipulate call
    ImVec2 gizmoPos(viewManipulateRight - 96, viewManipulateTop);

    ImGuizmo::ViewManipulate(glm::value_ptr(view), 10.0f, ImVec2(viewManipulateRight - 96, viewManipulateTop), ImVec2(96, 96), 0x10101010);

    // Now draw axis labels
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 center = ImVec2(gizmoPos.x + 48, gizmoPos.y + 48);

    // Extract rotation from view matrix
    glm::mat3 rotation = glm::mat3(view);

    // Define axis directions in world space
    glm::vec3 axes[] = {
        glm::vec3( 1,  0,  0),    // +X
        glm::vec3(-1,  0,  0),   // -X
        glm::vec3( 0,  1,  0),    // +Y
        glm::vec3( 0, -1,  0),   // -Y
        glm::vec3( 0,  0, -1),   // -Z
        glm::vec3( 0,  0,  1)     // +Z
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
        glm::vec3 viewAxis = glm::transpose(rotation) * axes[i];

        if (viewAxis.z > 0) continue;

        float scale = 24.0f;
        ImVec2 labelPos = ImVec2(
            center.x + viewAxis.x * scale,
            center.y - viewAxis.y * scale
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

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// STATUS BAR
void GUI::showStatusBar(int statusbarHeight, Camera& camera) const {
    float screenWidth = ImGui::GetIO().DisplaySize.x;
    float screenHeight = ImGui::GetIO().DisplaySize.y;
    float statusBarHeight = 25.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 4));

    if (ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(),
        ImGuiDir_Down, statusBarHeight, ImGuiWindowFlags_NoScrollbar)) {

       
        if (ImGui::BeginTable("##StatusBarTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit)) {

            // Column 0: Stretch (Takes all available space)
            ImGui::TableSetupColumn("##Stats", ImGuiTableColumnFlags_WidthStretch);
            // Column 1: Fixed (Only takes what it needs)
            ImGui::TableSetupColumn("##Device", ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();

            /*
            // --- LEFT COLUMN ---
            ImGui::TableNextColumn();
            glm::vec3 camPos = camera.getPos();
            glm::vec3 camDir = camera.getDir();
            // Use a more compact format to save space
            ImGui::Text("Zoom: %.1f% | P: (%.1f, %.1f, %.1f) | D: (%.1f, %.1f, %.1f)",
                camera.getZoom(), camPos.x, camPos.y, camPos.z, camDir.x, camDir.y, camDir.z);

            // --- RIGHT COLUMN ---
            ImGui::TableNextColumn();
            const char* gpuName = (const char*)glGetString(GL_RENDERER);
            char rightText[256];
            sprintf_s(rightText, "FPS: %.1f | %s", ImGui::GetIO().Framerate, gpuName);

            // Right-align the text INSIDE this column
            float textSize = ImGui::CalcTextSize(rightText).x;
            float colWidth = ImGui::GetColumnWidth();
            if (colWidth > textSize) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (colWidth - textSize));
            }

            ImGui::TextDisabled("%s", rightText);
            */

            ImGui::EndTable();
        }


        ImGui::End();
    }
    ImGui::PopStyleVar();
}

/* ===== HELPERS ========================================================== */
void GUI::DrawProperty(const char* label, std::function<void()> widget, float labelWidth) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(ImGui::GetWindowWidth() * labelWidth);
    ImGui::SetNextItemWidth(-1);
    widget();
}

/*
void GUI::DrawAttenuationGraph(float nearP, float farP, const Attenuation& atten) {
    ImGui::Spacing();
    ImGui::TextDisabled("Intensity / Distance");

    float samples[64];
    float range = std::max(0.1f, farP - nearP);

    for (int i = 0; i < 64; i++) {
        float d = nearP + (static_cast<float>(i) / 63.0f) * range;
        float denom = atten.constant + atten.linear * d + atten.quadratic * (d * d);
        samples[i] = (denom > 0.001f) ? (1.0f / denom) : 1.0f;
    }

    float graphHeight = 80.0f;

    ImGui::BeginGroup();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
    ImGui::TextDisabled("1.0");
    ImGui::Dummy(ImVec2(0, graphHeight - 32.0f));
    ImGui::TextDisabled("0.0");
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.44f, 0.22f, 1.00f, 1.00f));

    ImGui::PlotLines("##AttenGraph", samples, 64, 0, nullptr, 0.0f, 1.0f, ImVec2(-FLT_MIN, graphHeight));

    ImGui::PopStyleColor(2);

    ImGui::TextDisabled("%.1f", nearP);
    ImGui::SameLine();

    float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail / 2.0f) - 30.0f);

    ImGui::SameLine(ImGui::GetWindowWidth() - 75.0f);
    ImGui::TextDisabled("%.1f", farP);
    ImGui::EndGroup();
}
*/

void GUI::setPurpleTheme() const {
    auto& style = ImGui::GetStyle();
    style.TabRounding = 0.0f;
    ImVec4* colors = style.Colors;

    // --Palette
    ImVec4 purple       = ImVec4(0.44f, 0.22f, 1.00f, 1.00f);
    ImVec4 lightPurple  = ImVec4(0.54f, 0.36f, 1.00f, 1.00f);
    ImVec4 darkPurple   = ImVec4(0.35f, 0.15f, 0.80f, 1.00f);
    ImVec4 darkBg       = ImVec4(0.15f, 0.15f, 0.18f, 0.80f);
    ImVec4 darkerBg     = ImVec4(0.10f, 0.10f, 0.12f, 0.60f);
    ImVec4 peanutYellow = ImVec4(1.00f, 0.84f, 0.30f, 1.00f);

    // --Applying palette
    // Window Backgrounds
    colors[ImGuiCol_WindowBg] = darkerBg;
    colors[ImGuiCol_ChildBg]  = darkerBg;
    colors[ImGuiCol_PopupBg]  = darkBg;

    // Frame Backgrounds (Input boxes, sliders, checkboxes)
    colors[ImGuiCol_FrameBg]        = darkBg;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(purple.x, purple.y, purple.z, 0.40f); // Semi-transparent hover
    colors[ImGuiCol_FrameBgActive]  = darkPurple;

    // Title Bars
    colors[ImGuiCol_TitleBg]          = darkBg;
    colors[ImGuiCol_TitleBgActive]    = ImVec4(purple.x, purple.y, purple.z, 0.70f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(purple.x, purple.y, purple.z, 0.40f);

    // Headers (Collapsing Headers, Selectables)
    colors[ImGuiCol_Header]        = ImVec4(purple.x, purple.y, purple.z, 0.45f);
    colors[ImGuiCol_HeaderHovered] = lightPurple;
    colors[ImGuiCol_HeaderActive]  = darkPurple;

    // Buttons
    colors[ImGuiCol_Button]        = ImVec4(purple.x, purple.y, purple.z, 0.80f);
    colors[ImGuiCol_ButtonHovered] = lightPurple;
    colors[ImGuiCol_ButtonActive]  = darkPurple;

    // Checkmark/Slider Grab
    colors[ImGuiCol_CheckMark]        = lightPurple;
    colors[ImGuiCol_SliderGrab]       = lightPurple;
    colors[ImGuiCol_SliderGrabActive] = darkPurple;

    // Resize Grip (for resizable windows)
    colors[ImGuiCol_ResizeGrip]        = darkPurple;
    colors[ImGuiCol_ResizeGripHovered] = lightPurple;
    colors[ImGuiCol_ResizeGripActive]  = darkPurple;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]   = darkBg;
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);

    // Text
    colors[ImGuiCol_Text]         = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = peanutYellow;

    // Tabs
    colors[ImGuiCol_Tab]                = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]         = lightPurple;
    colors[ImGuiCol_TabActive]          = purple;
    colors[ImGuiCol_TabUnfocused]       = darkerBg;
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
}