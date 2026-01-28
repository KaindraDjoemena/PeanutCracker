#ifndef SCENE_H
#define SCENE_H

#include "cubemap.h"
#include "object.h"
#include "light.h"
#include "assetManager.h"
#include "ray.h"
#include "sceneNode.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "ImGuizmo.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <vector>
#include <filesystem>
#include <algorithm>


const unsigned int MAX_LIGHTS = 8;

// BINDING POINT ENUM
enum Binding_Point {
	CAMERA_BINDING_POINT,	// 0
	LIGHTS_BINDING_POINT,	// 1
	SHADOW_BINDING_POINT	// 2
};

enum class Render_Mode {
	STANDARD_DIFFUSE,
	IMAGE_BASED_LIGHTING,
	WIREFRAME
};

// UBO STRUCTS
struct DirectionalLightStruct {
	glm::vec4 direction;	// 16
	glm::vec4 ambient;		// 16
	glm::vec4 diffuse;		// 16
	glm::vec4 specular;		// 16
};							// 64 Bytes

struct PointLightStruct {
	glm::vec4 position;		// 16
	glm::vec4 ambient;		// 16
	glm::vec4 diffuse;		// 16
	glm::vec4 specular;		// 16
	float constant;			// 4
	float linear;			// 4
	float quadratic;		// 4
	float _padding;			// 4
};							// 80 Bytes

struct SpotLightStruct {
	glm::vec4 position;		// 16
	glm::vec4 direction;	// 16
	glm::vec4 ambient;		// 16
	glm::vec4 diffuse;		// 16
	glm::vec4 specular;		// 16
	float constant;			// 4
	float linear;			// 4
	float quadratic;		// 4
	float innerCutoff;		// 4
	float outerCutoff;		// 4
	float _padding0;		// 4
	float _padding1;		// 4
	float _padding2;		// 4
};							// 112 Bytes

// UBO DATA
struct LightingUBOData {									// 2064 Bytes
	DirectionalLightStruct directionalLight[MAX_LIGHTS];	// 64 * 8  = 512
	PointLightStruct       pointLight[MAX_LIGHTS];			// 80 * 8  = 640
	SpotLightStruct        spotLight[MAX_LIGHTS];			// 112 * 8 = 896
	int numDirLights;										// 4
	int numPointLights;										// 4
	int numSpotLights;										// 4
	int padding;											// 4
};

struct ShadowMatricesUBOData {								// 1536 Bytes
	glm::mat4 directionalLightSpaceMatrices[MAX_LIGHTS];	// 64 * 8  = 512
	glm::mat4 pointLightSpaceMatrices[MAX_LIGHTS];			// 64 * 8  = 512
	glm::mat4 spotLightSpaceMatrices[MAX_LIGHTS];			// 64 * 8  = 512
};

struct CameraMatricesUBOData {	// 144 Bytes
	glm::mat4 projection;		// 64
	glm::mat4 view;				// 64
	glm::vec4 cameraPos;		// 16
};

class Scene {
public:
	std::unique_ptr<SceneNode> worldNode;	// Scene graph parent

	std::unique_ptr<Cubemap>						skybox;
	std::vector<std::unique_ptr<DirectionalLight>>	directionalLights;
	std::vector<std::unique_ptr<PointLight>>		pointLights;
	std::vector<std::unique_ptr<SpotLight>>			spotLights;

	std::vector<SceneNode*>							selectedEntities;

	Scene(AssetManager* i_assetManager) : assetManager(i_assetManager) {
		worldNode = std::make_unique<SceneNode>("Root");
		depthShaderObject = assetManager->loadShaderObject("defaultshaders/depth.vert", "defaultshaders/depth.frag");
	}
	Scene(const Scene&) = delete;
	Scene& operator = (const Scene&) = delete;

	// OBJECT PICKING
	void selectEntity(MouseRay& worldRay, bool isHoldingShift) {
		float shortestDist = FLT_MAX;
		SceneNode* bestNode = nullptr;

		if (worldNode) {
			findBestNodeRecursive(worldNode.get(), worldRay, shortestDist, bestNode);
		}

		if (bestNode) {
			handleSelectionLogic(bestNode, isHoldingShift);
		}
		else if (!isHoldingShift) {
			clearSelection();
		}
	}

	void findBestNodeRecursive(SceneNode* node, MouseRay& worldRay, float& shortestDist, SceneNode*& bestNode) {
		if (node->object && node->object->modelPtr) {
			glm::mat4 invWorld = glm::inverse(node->worldMatrix);
			MouseRay localRay = worldRay;
			localRay.origin = glm::vec3(invWorld * glm::vec4(worldRay.origin, 1.0f));
			localRay.direction = glm::normalize(glm::vec3(invWorld * glm::vec4(worldRay.direction, 0.0f)));

			calcRayDist(localRay, node->object.get());

			if (localRay.hit && localRay.dist < shortestDist) {
				shortestDist = localRay.dist;
				bestNode = node;
			}
		}
		for (auto& child : node->children) {
			findBestNodeRecursive(child.get(), worldRay, shortestDist, bestNode);
		}
	}

	void handleSelectionLogic(SceneNode* node, bool isHoldingShift) {
		if (isHoldingShift) {
			// Toggle selection
			auto it = std::find(selectedEntities.begin(), selectedEntities.end(), node);
			if (it == selectedEntities.end()) {
				node->isSelected = true;
				selectedEntities.push_back(node);
			}
			else {
				node->isSelected = false;
				selectedEntities.erase(it);
			}
		}
		else {
			// Single select
			clearSelection();
			node->isSelected = true;
			selectedEntities.push_back(node);
		}
	}

	void clearSelection() {
		for (auto* node : selectedEntities) node->isSelected = false;
		selectedEntities.clear();
	}


	void debugPrintSceneGraph(SceneNode* node, int depth = 0) {
		std::string indent(depth * 2, ' ');
		std::cout << indent << "- " << node->name
			<< (node->isSelected ? " [SELECTED]" : "")
			<< std::endl;
		for (auto& child : node->children) {
			debugPrintSceneGraph(child.get(), depth + 1);
		}
	}

	void deleteSelectedEntities() {
		if (selectedEntities.empty()) return;
		std::cout << "Deleting " << selectedEntities.size() << " entities..." << std::endl;

		for (SceneNode* node : selectedEntities) {
			if (node == worldNode.get()) {
				std::cout << "  WARNING: Cannot delete root node!" << std::endl;
				continue;
			}

			if (!node->parent) {
				std::cout << "  WARNING: Node has no parent, cannot delete: " << node->name << std::endl;
				continue;
			}

			auto& siblings = node->parent->children;

			// Remove from parent
			siblings.erase(
				std::remove_if(siblings.begin(), siblings.end(),
					[node](const std::unique_ptr<SceneNode>& child) {
						return child.get() == node;
					}),
				siblings.end()
			);
		}

		selectedEntities.clear();

		worldNode->update(glm::mat4(1.0f), false);

		std::cout << "SUCCESS: Deleted selected entities." << std::endl;
	}


	void duplicateSelectedEntities() {
		if (selectedEntities.empty()) return;
		std::cout << "Duplicating " << selectedEntities.size() << " selected entities..." << std::endl;

		std::vector<SceneNode*> newSelection;

		for (SceneNode* source : selectedEntities) {
			std::cout << "  Processing: " << source->name << std::endl;

			if (!source->parent) {
				std::cout << "    ERROR: No parent pointer! Skipping." << std::endl;
				continue;
			}

			// Clone the node
			std::unique_ptr<SceneNode> clonedNodePtr = source->clone();

			if (!clonedNodePtr) {
				std::cout << "    ERROR: Clone failed!" << std::endl;
				continue;
			}

			// Deselect original, select clone
			source->isSelected = false;
			clonedNodePtr->isSelected = true;

			// Capture raw pointer BEFORE moving
			SceneNode* rawPtr = clonedNodePtr.get();

			// Add to parent (this will set the clone's parent pointer via addChild)
			source->parent->addChild(std::move(clonedNodePtr));

			newSelection.push_back(rawPtr);
		}

		selectedEntities = newSelection;

		// CRITICAL: Recalculate world matrices
		worldNode->update(glm::mat4(1.0f), false);

		std::cout << "SUCCESS: Duplicated " << newSelection.size() << " entities." << std::endl;
	}

	// OBJECT LOADING QUEUE
	void queueModelLoad(const std::filesystem::path& path) {
		loadQueue.push_back(path);
	}

	void processLoadQueue() {
		if (loadQueue.empty()) return;

		for (const auto& path : loadQueue) {
			createAndAddObject(
				path.string(),
				"defaultshaders/model.vert",
				"defaultshaders/model.frag"
			);

		}
		loadQueue.clear();
	}

	// CREATING AND ADDING ENTITIES
	void createAndAddObject(const std::string& modelPath, const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
		std::shared_ptr<Model> modelPtr = assetManager->loadModel(modelPath);
		std::shared_ptr<Shader> shaderObjectPtr = assetManager->loadShaderObject(vertPath, fragPath);

		// Instantiate new node
		std::string name = std::filesystem::path(modelPath).filename().string();
		auto newNode	 = std::make_unique<SceneNode>(name);
		newNode->object  = std::make_unique<Object>(modelPtr.get(), shaderObjectPtr.get());
		
		newNode->setSphereComponentRadius();

		setupModelShaderUBOs(shaderObjectPtr.get());
		worldNode->addChild(std::move(newNode));
	}
	void createAndAddSkybox(const Faces& faces, const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
		std::shared_ptr<Shader> shaderObjectPtr = assetManager->loadShaderObject(vertPath, fragPath);
		auto skyboxPtr = std::make_unique<Cubemap>(faces, shaderObjectPtr.get());

		setupSkyboxShaderUBOs(shaderObjectPtr.get());
		skybox = std::move(skyboxPtr);
	}
	void createAndAddDirectionalLight(std::unique_ptr<DirectionalLight> light) {
		directionalLights.push_back(std::move(light));
		numDirectionalLights++;
	}
	void createAndAddPointLight(std::unique_ptr<PointLight> light) {
		pointLights.push_back(std::move(light));
		numPointLights++;
	}
	void createAndAddSpotLight(std::unique_ptr<SpotLight> light) {
		spotLights.push_back(std::move(light));
		numSpotLights++;
	}

	void createAndAddSkyboxFromDirectory(const std::string& directory) {
		namespace fs = std::filesystem;
		fs::path root(directory);

		std::vector<std::string> faceNames  = { "right", "left", "top", "bottom", "front", "back" };
		std::vector<std::string> extensions = { ".jpg", ".png", ".tga", ".jpeg", ".bmp" };
		std::string finalPaths[6];

		// Iterate over the faces
		for (int i = 0; i < 6; ++i) {
			bool found = false;
			for (const auto& ext : extensions) {
				fs::path fullPath = root / (faceNames[i] + ext);
				if (fs::exists(fullPath)) {
					finalPaths[i] = fullPath.string();
					found = true;
					break; // Found the file for this face, move to next face
				}
			}

			if (!found) {
				std::cerr << "[Skybox Error] Missing face: " << faceNames[i] << " in directory " << directory << std::endl;
				return; // Exit early if any face is missing to prevent OpenGL errors
			}
		}

		Faces faces(
			finalPaths[0], finalPaths[1], finalPaths[3],
			finalPaths[2], finalPaths[4], finalPaths[5]
		);

		this->createAndAddSkybox(
			faces,
			"defaultshaders/skybox.vert",
			"defaultshaders/skybox.frag"
		);

		std::cout << "[Skybox] Successfully loaded environment from: " << directory << std::endl;
	}

	void setSkybox(std::unique_ptr<Cubemap> sb) {
		skybox = std::move(sb);
	}


	// RENDERING MODE
	void setRenderMode(Render_Mode mode) {
		renderMode = mode;
	}


	// ALLOCATING UBO SPACE AND BINDING TO THEIR POINTS
	void setupUBOs() {
		// Setup Camera UBO
		glGenBuffers(1, &cameraMatricesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, cameraMatricesUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraMatricesUBOData), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, CAMERA_BINDING_POINT, cameraMatricesUBO); // binding Point 0

		// Setup Lighting UBO
		glGenBuffers(1, &lightingUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, lightingUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(LightingUBOData), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, LIGHTS_BINDING_POINT, lightingUBO); // binding Point 1

		// Setup Shadow UBO
		glGenBuffers(1, &shadowUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, shadowUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(ShadowMatricesUBOData), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, SHADOW_BINDING_POINT, shadowUBO);	// binding point 2

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	// BINDS SHADERS TO UBO BINDING POINTS
	void setupModelShaderUBOs(const Shader* shader) const {
		if (!shader) return;

		// Camera
		unsigned int camIndex = glGetUniformBlockIndex(shader->ID, "CameraMatricesUBOData");
		if (camIndex != GL_INVALID_INDEX) {
			glUniformBlockBinding(shader->ID, camIndex, CAMERA_BINDING_POINT);
		}

		// Lighting
		unsigned int lightIndex = glGetUniformBlockIndex(shader->ID, "LightingUBOData");
		if (lightIndex != GL_INVALID_INDEX) {
			glUniformBlockBinding(shader->ID, lightIndex, LIGHTS_BINDING_POINT);
		}

		// Shadow
		unsigned int shadowIndex = glGetUniformBlockIndex(shader->ID, "ShadowMatricesUBOData");
		if (shadowIndex != GL_INVALID_INDEX) {
			glUniformBlockBinding(shader->ID, shadowIndex, SHADOW_BINDING_POINT);
		}
	}

	void setupSkyboxShaderUBOs(const Shader* shader) const {
		if (!skybox || !shader) return;

		unsigned int camIndex = glGetUniformBlockIndex(shader->ID, "CameraMatricesUBOData");
		if (camIndex != GL_INVALID_INDEX) {
			glUniformBlockBinding(shader->ID, camIndex, CAMERA_BINDING_POINT);
		}
	}

	// RENDERING
	void draw(const Camera& cameraObject, float vWidth, float vHeight, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
		GLint previousFBO;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

		// --Updating Light Space Matrices
		for (auto& dirLight : directionalLights) {
			if (dirLight->shadowCasterComponent) {
				dirLight->shadowCasterComponent->calcLightSpaceMat(dirLight->direction);
			}
		}

		// --Updating UBOs
		updateCameraUBO(projection, view, cameraPos);
		updateLightingUBO();
		updateShadowUBO();


		// DEPTH PASS
		if (!directionalLights.empty() && directionalLights[0]->shadowCasterComponent) {
			const glm::vec2 res = directionalLights[0]->shadowCasterComponent->getShadowMapRes();
			glViewport(0, 0, res.x, res.y);

			depthShaderObject->use();
			
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);   // IMPORTANT

			// --Rendering each object for each light from the lights pov
			for (auto& dirLight : directionalLights) {
				if (!dirLight->shadowCasterComponent) continue;

				ShadowCasterComponent* shadowComponent = dirLight->shadowCasterComponent.get();

				glBindFramebuffer(GL_FRAMEBUFFER, shadowComponent->getFboID());
				glClear(GL_DEPTH_BUFFER_BIT);

				depthShaderObject->setMat4("lightSpaceMatrix", shadowComponent->getLightSpaceMatrix());

				renderShadowRecursive(worldNode.get());
			}

			// --Reset opengl stuff
			//glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);

		// RENDER PASS
		glViewport(0, 0, vWidth, vHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// --Binding the depth maps
		for (size_t i = 0; i < directionalLights.size() && i < MAX_LIGHTS; ++i) {
			if (directionalLights[i]->shadowCasterComponent) {
				glActiveTexture(GL_TEXTURE0 + 10 + i);
				glBindTexture(GL_TEXTURE_2D, directionalLights[i]->shadowCasterComponent->getDepthMapTexID());
			}
		}

		// --Rendering the final scene
		// Skybox
		glDisable(GL_STENCIL_TEST);
		if (skybox) skybox->draw();


		worldNode->update(glm::mat4(1.0f), true);

		setNodeShadowMapUniforms(worldNode.get());

		if (renderMode == Render_Mode::WIREFRAME) { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
		renderRecursive(cameraObject, worldNode.get());
		if (renderMode == Render_Mode::WIREFRAME) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }




		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		drawDirectionalLightFrustums(view, projection);

		// Drawing selected objects
		drawSelectionStencil();

		drawDebugAABBs();
	}

	// RENDERING
	// --For objects
	void renderRecursive(const Camera& cameraObject, SceneNode* node) const {
		// Camera Frustum Culling
		bool isVisible = true;
		if (node->sphereColliderComponent && node != worldNode.get()) {
			BoundingSphere boundingSphere = { node->sphereColliderComponent->worldCenter, node->sphereColliderComponent->worldRadius };
			if (!cameraObject.frustum.isInFrustum(boundingSphere)) {
				//std::cout << "culled " << node->name << std::endl;
				isVisible = false;
			}
		}

		if (isVisible) {
			if (node->object) {
				//std::cout << "rendering " << node->name << std::endl;
				node->object->draw(node->worldMatrix);
			}

			for (auto& child : node->children) {
				renderRecursive(cameraObject, child.get());
			}
		}
	}

	// --For the shadow map
	void renderShadowRecursive(SceneNode* node) const {
		if (node->object) {
			node->object->drawShadow(node->worldMatrix, depthShaderObject.get());
		}

		for (auto& child : node->children) {
			renderShadowRecursive(child.get());
		}
	}

	void init() {
		setupUBOs();



		initLightFrustumDebug();
		initDebugDrawing();




		initSelectionOutline();
		setupModelShaderUBOs(outlineShader.get());
		//setupModelShaderUBOs(debugShader.get());

		setupNodeUBOs(worldNode.get());

		if (skybox && skybox->shaderPtr) {
			setupSkyboxShaderUBOs(skybox->shaderPtr);
		}
	}

	void setupNodeUBOs(SceneNode* node) {
		if (node->object && node->object->shaderPtr) {
			setupModelShaderUBOs(node->object->shaderPtr);
		}
		for (auto& child : node->children) {
			setupNodeUBOs(child.get());
		}
	}

private:
	AssetManager* assetManager;

	Render_Mode renderMode = Render_Mode::STANDARD_DIFFUSE;

	std::vector<std::filesystem::path> loadQueue;

	unsigned int numDirectionalLights = 0;
	unsigned int numPointLights = 0;
	unsigned int numSpotLights = 0;

	unsigned int cameraMatricesUBO	= 0;
	unsigned int lightingUBO		= 0;
	unsigned int shadowUBO			= 0;

	std::shared_ptr<Shader> depthShaderObject;


	unsigned int frustumVAO = 0, frustumVBO = 0;
	bool drawLightFrustums = true;
	std::shared_ptr<Shader> frustumShader;
	void initLightFrustumDebug() {
		float frustumLines[] = {
			// Near
			-1,-1,-1,  1,-1,-1,
			 1,-1,-1,  1, 1,-1,
			 1, 1,-1, -1, 1,-1,
			-1, 1,-1, -1,-1,-1,

			// Far
			-1,-1, 1,  1,-1, 1,
			 1,-1, 1,  1, 1, 1,
			 1, 1, 1, -1, 1, 1,
			-1, 1, 1, -1,-1, 1,

			// Sides
			-1,-1,-1, -1,-1, 1,
			 1,-1,-1,  1,-1, 1,
			 1, 1,-1,  1, 1, 1,
			-1, 1,-1, -1, 1, 1,
		};

		glGenVertexArrays(1, &frustumVAO);
		glGenBuffers(1, &frustumVBO);

		glBindVertexArray(frustumVAO);
		glBindBuffer(GL_ARRAY_BUFFER, frustumVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(frustumLines), frustumLines, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		frustumShader = assetManager->loadShaderObject(
			"defaultshaders/lightFrustum.vert",
			"defaultshaders/lightFrustum.frag"
		);
	}
	void drawDirectionalLightFrustums(const glm::mat4& view, const glm::mat4& proj) const {
		if (!drawLightFrustums || !frustumVAO) return;

		glDisable(GL_DEPTH_TEST);

		frustumShader->use();
		frustumShader->setMat4("view", view);
		frustumShader->setMat4("projection", proj);
		frustumShader->setVec3("color", glm::vec3(1, 0, 1));

		glBindVertexArray(frustumVAO);

		for (auto& dirLight : directionalLights) {
			if (!dirLight->shadowCasterComponent) continue;

			glm::mat4 invLightVP =
				glm::inverse(dirLight->shadowCasterComponent->getLightSpaceMatrix());

			frustumShader->setMat4("model", invLightVP);
			glDrawArrays(GL_LINES, 0, 24);
		}

		glEnable(GL_DEPTH_TEST);
	}




	// UPDATING THE UBOS
	void updateCameraUBO(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) const {
		CameraMatricesUBOData data = { projection, view, glm::vec4(cameraPos, 1.0f) };
		glBindBuffer(GL_UNIFORM_BUFFER, cameraMatricesUBO);
		//glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraMatricesUBOData), &data);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraMatricesUBOData), &data, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void updateLightingUBO() const {
		LightingUBOData data = {};

		data.numDirLights = static_cast<int>(directionalLights.size());
		data.numPointLights = static_cast<int>(pointLights.size());
		data.numSpotLights = static_cast<int>(spotLights.size());

		for (size_t i = 0; i < directionalLights.size() && i < MAX_LIGHTS; ++i) {
			auto& src = directionalLights[i];
			auto& dst = data.directionalLight[i];
			dst.direction = glm::vec4(src->direction, 0.0f);
			dst.ambient = glm::vec4(src->light.ambient, 1.0f);
			dst.diffuse = glm::vec4(src->light.diffuse, 1.0f);
			dst.specular = glm::vec4(src->light.specular, 1.0f);
		}

		for (size_t i = 0; i < pointLights.size() && i < MAX_LIGHTS; ++i) {
			auto& src = pointLights[i];
			auto& dst = data.pointLight[i];
			dst.position = glm::vec4(src->position, 1.0f);
			dst.ambient = glm::vec4(src->light.ambient, 1.0f);
			dst.diffuse = glm::vec4(src->light.diffuse, 1.0f);
			dst.specular = glm::vec4(src->light.specular, 1.0f);
			dst.constant = src->attenuation.constant;
			dst.linear = src->attenuation.linear;
			dst.quadratic = src->attenuation.quadratic;
		}

		for (size_t i = 0; i < spotLights.size() && i < MAX_LIGHTS; ++i) {
			auto& src = spotLights[i];
			auto& dst = data.spotLight[i];
			dst.position = glm::vec4(src->position, 1.0f);
			dst.direction = glm::vec4(src->direction, 0.0f);
			dst.ambient = glm::vec4(src->light.ambient, 1.0f);
			dst.diffuse = glm::vec4(src->light.diffuse, 1.0f);
			dst.specular = glm::vec4(src->light.specular, 1.0f);
			dst.constant = src->attenuation.constant;
			dst.linear = src->attenuation.linear;
			dst.quadratic = src->attenuation.quadratic;
			dst.innerCutoff = glm::cos(glm::radians(src->innerCutoff));
			dst.outerCutoff = glm::cos(glm::radians(src->outerCutoff));
		}

		glBindBuffer(GL_UNIFORM_BUFFER, lightingUBO);
		//glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightingUBOData), &data);

		glBufferData(GL_UNIFORM_BUFFER, sizeof(LightingUBOData), &data, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void updateShadowUBO() const {
		ShadowMatricesUBOData data = {};

		for (size_t i = 0; i < directionalLights.size() && i < MAX_LIGHTS; ++i) {
			if (directionalLights[i]->shadowCasterComponent) {
				data.directionalLightSpaceMatrices[i] = directionalLights[i]->shadowCasterComponent->getLightSpaceMatrix();
			}
			else {
				data.directionalLightSpaceMatrices[i] = glm::mat4(1.0f);
			}
		}

		for (size_t i = 0; i < MAX_LIGHTS; ++i) {
			data.pointLightSpaceMatrices[i] = glm::mat4(1.0f);
			data.spotLightSpaceMatrices[i] = glm::mat4(1.0f);
		}

		glBindBuffer(GL_UNIFORM_BUFFER, shadowUBO);
		//glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShadowMatricesUBOData), &data);

		glBufferData(GL_UNIFORM_BUFFER, sizeof(ShadowMatricesUBOData), &data, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void setNodeShadowMapUniforms(SceneNode* node) const {
		if (node->object && node->object->shaderPtr) {
			node->object->shaderPtr->use();
			for (size_t i = 0; i < directionalLights.size() && i < MAX_LIGHTS; ++i) {
				std::string uniformName = "DirectionalShadowMap[" + std::to_string(i) + "]";
				node->object->shaderPtr->setInt(uniformName, 10 + i);
			}
		}

		for (auto& child : node->children) {
			setNodeShadowMapUniforms(child.get());
		}
	}

	// SELECTION STENCIL/HIGHLIGHT
	std::shared_ptr<Shader> outlineShader;

	void initSelectionOutline() {
		outlineShader = assetManager->loadShaderObject("defaultshaders/outline.vert", "defaultshaders/outline.frag");
	}

	
	void drawSelectionStencil() const {
		if (selectedEntities.empty()) return;

		// WRITE TO THE STENCIL BUFFER
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);

		for (SceneNode* selectedNode : selectedEntities) {
			if (!selectedNode->object) continue;
			outlineShader->use();
			outlineShader->setMat4("model", selectedNode->worldMatrix);
			selectedNode->object->modelPtr->draw(outlineShader.get());
		}

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);

		// DRAWING THE OUTLINE
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF); // Draw where stencil is NOT 1
		glStencilMask(0x00); // Don't modify stencil
		glDisable(GL_DEPTH_TEST); // Outline shows through everything

		outlineShader->use();
		outlineShader->setVec4("color", glm::vec4(0.8f, 0.4f, 1.0f, 0.2f));

		for (SceneNode* selectedNode : selectedEntities) {
			if (!selectedNode->object) continue;

			glm::mat4 fatterModel = glm::scale(selectedNode->worldMatrix, glm::vec3(1.03f));
			outlineShader->setMat4("model", fatterModel);
			selectedNode->object->modelPtr->draw(outlineShader.get());
		}

		// OPENGL STATE CLEANUP
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glStencilMask(0xFF);
	}

	// AABB BOX DRAWING
	unsigned int debugVAO = 0, debugVBO = 0;
	std::shared_ptr<Shader> debugShader;
	void initDebugDrawing() {
		// For the box
		float vertices[] = {
			0,0,0, 1,0,0,  1,0,0, 1,1,0,  1,1,0, 0,1,0,  0,1,0, 0,0,0, // Back
			0,0,1, 1,0,1,  1,0,1, 1,1,1,  1,1,1, 0,1,1,  0,1,1, 0,0,1, // Front
			0,0,0, 0,0,1,  1,0,0, 1,0,1,  1,1,0, 1,1,1,  0,1,0, 0,1,1  // Connections
		};

		glGenVertexArrays(1, &debugVAO);
		glGenBuffers(1, &debugVBO);
		glBindVertexArray(debugVAO);
		glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		debugShader = assetManager->loadShaderObject("defaultshaders/debug.vert", "defaultshaders/debug.frag");
	}

	void drawDebugAABBsRecursive(SceneNode* node) const {
		if (!debugVAO || !debugShader) return;

		// Draw this node's AABB if it has an object
		if (node->object && node->object->modelPtr) {
			glm::vec3 size = node->object->modelPtr->aabb.max - node->object->modelPtr->aabb.min;
			glm::vec3 offset = node->object->modelPtr->aabb.min;

			glm::mat4 boxTransform = node->worldMatrix * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), size);

			debugShader->setMat4("model", boxTransform);

			// Check if this node is selected
			bool isSelected = false;
			for (auto* sel : selectedEntities) {
				if (sel == node) {
					isSelected = true;
					break;
				}
			}

			debugShader->setVec3("color", isSelected ? glm::vec3(0, 1, 0) : glm::vec3(1, 1, 1));

			glDrawArrays(GL_LINES, 0, 24);
		}

		// Recurse to children
		for (auto& child : node->children) {
			drawDebugAABBsRecursive(child.get());
		}
	}

	void drawDebugAABBs() const {
		if (!debugVAO || !debugShader) return;

		debugShader->use();
		glBindVertexArray(debugVAO);

		drawDebugAABBsRecursive(worldNode.get());
	}
};


#endif