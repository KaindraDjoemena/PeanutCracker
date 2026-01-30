#include "headers/scene.h"
#include "headers/vao.h"
#include "headers/vbo.h"
#include "headers/shader.h"
#include "headers/cubemap.h"
#include "headers/object.h"
#include "headers/model.h"
#include "headers/light.h"
#include "headers/assetManager.h"
#include "headers/ray.h"
#include "headers/camera.h"
#include "headers/sceneNode.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <iostream>


Scene::Scene(AssetManager* i_assetManager) : m_assetManager(i_assetManager) {
	worldNode = std::make_unique<SceneNode>("Root");
	m_depthShader = m_assetManager->loadShaderObject("defaultshaders/depth.vert", "defaultshaders/depth.frag");
}
//Scene::Scene(const Scene&) = delete;
//Scene::Scene& operator = (const Scene&) = delete;

/* ===== OBJECT PICKING & OPERATIONS ================================================================= */
// --PICKING
void Scene::selectEntity(MouseRay& worldRay, bool isHoldingShift) {
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
// --DELETION
void Scene::deleteSelectedEntities() {
	if (selectedEntities.empty()) return;
	std::cout << "[SCENE] Deleting " << selectedEntities.size() << " entities..." << '\n';

	for (SceneNode* node : selectedEntities) {
		if (node == worldNode.get()) {
			std::cout << "  WARNING: Cannot delete root node!" << '\n';
			continue;
		}

		if (!node->parent) {
			std::cout << "  WARNING: Node has no parent, cannot delete: " << node->name << '\n';
			continue;
		}

		auto& siblings = node->parent->children;

		// Remove from parent
		siblings.erase(
			std::remove_if(siblings.begin(), siblings.end(), [node](const std::unique_ptr<SceneNode>& child) {
				return child.get() == node;
			}),
			siblings.end()
		);
	}

	selectedEntities.clear();

	worldNode->update(glm::mat4(1.0f), false);

	std::cout << "[SCENE] Deleted selected entities" << '\n';
}
// --DUPLICATION
void Scene::duplicateSelectedEntities() {
	if (selectedEntities.empty()) return;
	std::cout << "[SCENE] Duplicating " << selectedEntities.size() << " selected entities..." << '\n';

	std::vector<SceneNode*> newSelection;

	for (SceneNode* source : selectedEntities) {
		std::cout << "  Processing: " << source->name << '\n';

		if (!source->parent) {
			std::cout << "    ERROR: No parent pointer! Skipping." << '\n';
			continue;
		}

		// Clone the node
		std::unique_ptr<SceneNode> clonedNodePtr = source->clone();

		if (!clonedNodePtr) {
			std::cout << "    ERROR: Clone failed!" << '\n';
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

	worldNode->update(glm::mat4(1.0f), false);

	std::cout << "[SCENE] Duplicated " << newSelection.size() << " entities." << '\n';
}


/* ===== OBJECT LOADING QUEUE ================================================================= */
void Scene::queueModelLoad(const std::filesystem::path& path) {
	loadQueue.push_back(path);
}
void Scene::processLoadQueue() {
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


/* ===== ADDING ENTITIES ================================================================= */
void Scene::createAndAddObject(const std::string& modelPath, const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
	std::shared_ptr<Model> modelPtr = m_assetManager->loadModel(modelPath);
	std::shared_ptr<Shader> shaderObjectPtr = m_assetManager->loadShaderObject(vertPath, fragPath);

	// Instantiate new node
	std::string name = std::filesystem::path(modelPath).filename().string();
	auto newNode = std::make_unique<SceneNode>(name);
	newNode->object = std::make_unique<Object>(modelPtr.get(), shaderObjectPtr.get());

	newNode->setSphereComponentRadius();

	bindToUBOs(shaderObjectPtr.get());
	worldNode->addChild(std::move(newNode));
}
void Scene::createAndAddSkybox(const Faces& faces, const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
	std::shared_ptr<Shader> shaderObjectPtr = m_assetManager->loadShaderObject(vertPath, fragPath);
	auto skyboxPtr = std::make_unique<Cubemap>(faces, shaderObjectPtr.get());

	setupSkyboxShaderUBOs(shaderObjectPtr.get());
	skybox = std::move(skyboxPtr);
}
void Scene::createAndAddDirectionalLight(std::unique_ptr<DirectionalLight> light) {
	directionalLights.push_back(std::move(light));
	numDirectionalLights++;
}
void Scene::createAndAddPointLight(std::unique_ptr<PointLight> light) {
	pointLights.push_back(std::move(light));
	numPointLights++;
}
void Scene::createAndAddSpotLight(std::unique_ptr<SpotLight> light) {
	spotLights.push_back(std::move(light));
	numSpotLights++;
}
void Scene::createAndAddSkyboxFromDirectory(const std::string& directory) {
	namespace fs = std::filesystem;
	fs::path root(directory);

	std::vector<std::string> faceNames = { "right", "left", "top", "bottom", "front", "back" };
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
			std::cerr << "[Skybox Error] Missing face: " << faceNames[i] << " in directory " << directory << '\n';
			return;
		}
	}

	Faces faces(
		finalPaths[0], finalPaths[1], finalPaths[3],
		finalPaths[2], finalPaths[4], finalPaths[5]
	);

	createAndAddSkybox(faces, "defaultshaders/skybox.vert", "defaultshaders/skybox.frag");

	std::cout << "[SKYBOX] Successfully loaded environment from: " << directory << '\n';
}


/* ===== SCENE SETTERS (RENDERER) ================================================================= */
void Scene::setRenderMode(Render_Mode mode) {
	renderMode = mode;
}


/* ===== UBOs ============================================================================*/
// --ALLOCATION
void Scene::setupUBOBindings() {
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
// --SHADER BINDING
void Scene::bindToUBOs(const Shader* shader) const {
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
void Scene::setupSkyboxShaderUBOs(const Shader* shader) const {
	if (!skybox || !shader) return;

	unsigned int camIndex = glGetUniformBlockIndex(shader->ID, "CameraMatricesUBOData");
	if (camIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader->ID, camIndex, CAMERA_BINDING_POINT);
	}
}


/* ===== RENDERING ================================================================================== */
void Scene::draw(Camera& camera, float vWidth, float vHeight) {
	GLint previousFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

	// --Updating Camera vectors
	camera.updateCameraVectors();

	// -- Updating light space matrices
	for (auto& dirLight : directionalLights) {
		if (dirLight->shadowCasterComponent) {
			// Check if direction is valid before calc
			if (glm::length(dirLight->direction) < 0.001f) {
				std::cerr << "ERROR: Direction is zero or near-zero!" << '\n';
			}
			if (glm::any(glm::isnan(dirLight->direction))) {
				std::cerr << "ERROR: Direction contains NaN!" << '\n';
			}

			dirLight->shadowCasterComponent->calcLightSpaceMat(dirLight->direction, glm::vec3(0.0f, 0.0f, 0.0f));

			// Check result immediately
			glm::mat4 test = dirLight->shadowCasterComponent->getLightSpaceMatrix();
			if (glm::any(glm::isnan(test[0]))) {
				std::cerr << "Matrix became NaN inside calcLightSpaceMat!" << '\n';
			}
		}
	}
	// PointLights
	for (auto& spotLight : spotLights) {
		if (spotLight->shadowCasterComponent) {
			// Check if direction is valid before calc
			if (glm::length(spotLight->direction) < 0.001f) {
				std::cerr << "ERROR: Direction is zero or near-zero!" << '\n';
			}
			if (glm::any(glm::isnan(spotLight->direction))) {
				std::cerr << "ERROR: Direction contains NaN!" << '\n';
			}

			spotLight->shadowCasterComponent->calcLightSpaceMat(spotLight->direction, spotLight->position);

			// Check result immediately
			glm::mat4 test = spotLight->shadowCasterComponent->getLightSpaceMatrix();
			if (glm::any(glm::isnan(test[0]))) {
				std::cerr << "SPOTLIGHT: Matrix became NaN inside calcLightSpaceMat!" << '\n';
			}
		}
	}

	// --Updating UBOs
	updateCameraUBO(camera.getProjMat(vWidth / vHeight), camera.getViewMat(), camera.position);
	updateLightingUBO();
	updateShadowUBO();


	// SHADOW PASS
	bool hasAnyShadowCaster = false;
	for (auto& light : directionalLights) {
		if (light->shadowCasterComponent) {
			hasAnyShadowCaster = true;
			break;
		}
	}
	if (hasAnyShadowCaster) {
		const glm::vec2 res = directionalLights[0]->shadowCasterComponent->getShadowMapRes();
		glViewport(0, 0, res.x, res.y);

		m_depthShader->use();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);		// Sampilng on the far face

		// --Rendering each object for each light from the lights pov,
		// and storing the depth value to an FBO
		// -- Directional lights
		for (auto& dirLight : directionalLights) {
			if (!dirLight->shadowCasterComponent) continue;

			ShadowCasterComponent* shadowComponent = dirLight->shadowCasterComponent.get();

			glBindFramebuffer(GL_FRAMEBUFFER, shadowComponent->getFboID());
			glClear(GL_DEPTH_BUFFER_BIT);

			m_depthShader->setMat4("lightSpaceMatrix", shadowComponent->getLightSpaceMatrix());

			renderShadowRecursive(worldNode.get());
		}
		// -- Point lights
		for (auto& pointLight : pointLights) {
			if (!pointLight->shadowCasterComponent) continue;

			ShadowCasterComponent* shadowComponent = pointLight->shadowCasterComponent.get();

			glBindFramebuffer(GL_FRAMEBUFFER, shadowComponent->getFboID());
			glClear(GL_DEPTH_BUFFER_BIT);

			m_depthShader->setMat4("lightSpaceMatrix", shadowComponent->getLightSpaceMatrix());

			renderShadowRecursive(worldNode.get());
		}
		// -- Spot lights
		for (auto& spotLight : spotLights) {
			if (!spotLight->shadowCasterComponent) continue;

			ShadowCasterComponent* shadowComponent = spotLight->shadowCasterComponent.get();

			glBindFramebuffer(GL_FRAMEBUFFER, shadowComponent->getFboID());
			glClear(GL_DEPTH_BUFFER_BIT);

			m_depthShader->setMat4("lightSpaceMatrix", shadowComponent->getLightSpaceMatrix());

			renderShadowRecursive(worldNode.get());
		}

		// --Reset opengl stuff
		glCullFace(GL_BACK);
	}


	// LIGHT PASS
	glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
	glViewport(0, 0, vWidth, vHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// --Binding the depth maps
	// --Directinoal lights
	for (size_t i = 0; i < directionalLights.size() && i < MAX_LIGHTS; ++i) {
		if (directionalLights[i]->shadowCasterComponent) {
			glActiveTexture(GL_TEXTURE0 + 10 + i);
			glBindTexture(GL_TEXTURE_2D, directionalLights[i]->shadowCasterComponent->getDepthMapTexID());
		}
	}
	// --Point lights
	for (size_t i = 0; i < pointLights.size() && i < MAX_LIGHTS; ++i) {
		if (pointLights[i]->shadowCasterComponent) {
			glActiveTexture(GL_TEXTURE0 + 20 + i);
			glBindTexture(GL_TEXTURE_2D, pointLights[i]->shadowCasterComponent->getDepthMapTexID());
		}
	}
	// --Spot lights
	for (size_t i = 0; i < spotLights.size() && i < MAX_LIGHTS; ++i) {
		if (spotLights[i]->shadowCasterComponent) {
			glActiveTexture(GL_TEXTURE0 + 30 + i);
			glBindTexture(GL_TEXTURE_2D, spotLights[i]->shadowCasterComponent->getDepthMapTexID());
		}
	}

	// --Rendering the final scene
	// Skybox
	glDisable(GL_STENCIL_TEST);
	if (skybox) skybox->draw();


	worldNode->update(glm::mat4(1.0f), true);

	setNodeShadowMapUniforms(worldNode.get());		// Set fragment shader shadow map uniformms

	if (renderMode == Render_Mode::WIREFRAME) { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
	renderRecursive(camera, worldNode.get());
	if (renderMode == Render_Mode::WIREFRAME) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }



	// --Drawing selected objects
	drawSelectionStencil();

	// --Debug drawing
	drawDirectionalLightFrustums(camera.getProjMat(vWidth / vHeight), camera.getViewMat());
	drawSpotLightFrustums(camera.getProjMat(vWidth / vHeight), camera.getViewMat());
	drawDebugAABBs(worldNode.get());
}

// RENDERING
// --For objects
void Scene::renderRecursive(const Camera& camera, SceneNode* node) const {
	// Camera Frustum Culling
	bool isVisible = true;
	if (node->sphereColliderComponent && node != worldNode.get()) {
		BoundingSphere boundingSphere = { node->sphereColliderComponent->worldCenter, node->sphereColliderComponent->worldRadius };
		if (!camera.frustum.isInFrustum(boundingSphere)) {
			isVisible = false;
		}
	}

	if (isVisible) {
		if (node->object) {
			//std::cout << "rendering " << node->name << '\n';
			node->object->draw(node->worldMatrix);
		}

		for (auto& child : node->children) {
			renderRecursive(camera, child.get());
		}
	}
}
// --For the shadow map
void Scene::renderShadowRecursive(SceneNode* node) const {
	if (node->object) {
		node->object->drawShadow(node->worldMatrix, m_depthShader.get());
	}

	for (auto& child : node->children) {
		renderShadowRecursive(child.get());
	}
}

void Scene::init() {
	setupUBOBindings();				// Camera, Lighting, Shadow

	initLightFrustumDebug();
	initDebugAABBDrawing();

	initSelectionOutline();
	bindToUBOs(m_outlineShader.get());

	setupNodeUBOs(worldNode.get());

	if (skybox && skybox->shaderPtr) {
		setupSkyboxShaderUBOs(skybox->shaderPtr);
	}
}

/* ===== PICKING OPERATIONS ================================================================= */
void Scene::findBestNodeRecursive(SceneNode* node, MouseRay& worldRay, float& shortestDist, SceneNode*& bestNode) {
	if (node->object && node->object->modelPtr) {
		glm::mat4 invWorld = glm::inverse(node->worldMatrix);
		MouseRay localRay = worldRay;
		localRay.origin = glm::vec3(invWorld * glm::vec4(worldRay.origin, 1.0f));
		localRay.direction = glm::normalize(glm::vec3(invWorld * glm::vec4(worldRay.direction, 0.0f)));

		localRay.calcRayDist(node->object.get());

		if (localRay.hit && localRay.dist < shortestDist) {
			shortestDist = localRay.dist;
			bestNode = node;
		}
	}
	for (auto& child : node->children) {
		findBestNodeRecursive(child.get(), worldRay, shortestDist, bestNode);
	}
}
void Scene::handleSelectionLogic(SceneNode* node, bool isHoldingShift) {
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
void Scene::clearSelection() {
	for (auto* node : selectedEntities) node->isSelected = false;
	selectedEntities.clear();
}
void Scene::debugPrintSceneGraph(SceneNode* node, int depth) {
	for (auto& child : node->children) {
		debugPrintSceneGraph(child.get(), depth + 1);
	}
}


/* ===== LIGHT FRUSTUM DRAWING ================================================================= */
void Scene::initLightFrustumDebug() {
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


	m_lightFrustumVAO.bind();
	m_lightFrustumVBO = VBO(frustumLines, sizeof(frustumLines), GL_STATIC_DRAW);
	m_lightFrustumVAO.linkAttrib(m_lightFrustumVBO, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);
	m_lightFrustumVAO.unbind();

	m_frustumShader = m_assetManager->loadShaderObject(
		"defaultshaders/lightFrustum.vert",
		"defaultshaders/lightFrustum.frag"
	);
}
void Scene::drawDirectionalLightFrustums(const glm::mat4& projMat, const glm::mat4& viewMat, const glm::vec4& color, float lineWidth) const {
	if (!drawLightFrustums || !m_frustumShader) return;

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "[DRAW DIRLIGHT FRUST] OpenGL error: " << err << '\n';
	}

	glLineWidth(lineWidth);
	m_frustumShader->use();
	m_frustumShader->setMat4("projection", projMat);
	m_frustumShader->setMat4("view", viewMat);
	m_frustumShader->setVec4("color", color);

	m_lightFrustumVAO.bind();

	for (auto& dirLight : directionalLights) {
		if (!dirLight->shadowCasterComponent) continue;

		glm::mat4 lightVP = dirLight->shadowCasterComponent->getLightSpaceMatrix();
		if (glm::any(glm::isnan(lightVP[0])) || glm::any(glm::isinf(lightVP[0]))) {
			continue;
		}

		glm::mat4 invLightVP = glm::inverse(lightVP);
		m_frustumShader->setMat4("model", invLightVP);
		glDrawArrays(GL_LINES, 0, 24);
	}
	m_lightFrustumVAO.unbind();
}
//void drawPointLightFrustums();
void Scene::drawSpotLightFrustums(const glm::mat4& projMat, const glm::mat4& viewMat, const glm::vec4& color, float lineWidth) const {
	if (!drawLightFrustums || !m_frustumShader) return;

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "[DRAW SPOTLIGHT FRUST] OpenGL error: " << err << '\n';
	}

	glLineWidth(lineWidth);
	m_frustumShader->use();
	m_frustumShader->setMat4("projection", projMat);
	m_frustumShader->setMat4("view", viewMat);
	m_frustumShader->setVec4("color", color);

	m_lightFrustumVAO.bind();

	for (auto& spotLight : spotLights) {
		if (!spotLight->shadowCasterComponent) continue;

		glm::mat4 lightVP = spotLight->shadowCasterComponent->getLightSpaceMatrix();
		if (glm::any(glm::isnan(lightVP[0])) || glm::any(glm::isinf(lightVP[0]))) {
			continue;
		}

		// Transform unit cube [-1,1] by inverse perspective matrix to get pyramid frustum corners
		glm::mat4 invLightVP = glm::inverse(lightVP);
		m_frustumShader->setMat4("model", invLightVP);
		glDrawArrays(GL_LINES, 0, 24);
	}
	m_lightFrustumVAO.unbind();
}


/* ===== UPDATING UBOs ================================================================= */
// BINDING NODE SHADERS TO UBOs
void Scene::setupNodeUBOs(SceneNode* node) {
	// Setting the model UBOs
	if (node->object && node->object->shaderPtr) {
		bindToUBOs(node->object->shaderPtr);
	}
	for (auto& child : node->children) {
		setupNodeUBOs(child.get());
	}
}
// CAMERA UBO
void Scene::updateCameraUBO(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) const {
	CameraMatricesUBOData data = { projection, view, glm::vec4(cameraPos, 1.0f) };

	glBindBuffer(GL_UNIFORM_BUFFER, cameraMatricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraMatricesUBOData), &data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
// LIGHTING UBO
void Scene::updateLightingUBO() const {
	LightingUBOData data = {};

	data.numDirLights = static_cast<int>(directionalLights.size());
	data.numPointLights = static_cast<int>(pointLights.size());
	data.numSpotLights = static_cast<int>(spotLights.size());

	// --Directional
	for (size_t i = 0; i < directionalLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = directionalLights[i];
		auto& dst = data.directionalLight[i];
		dst.direction = glm::vec4(src->direction, 0.0f);
		dst.ambient = glm::vec4(src->light.ambient, 1.0f);
		dst.diffuse = glm::vec4(src->light.diffuse, 1.0f);
		dst.specular = glm::vec4(src->light.specular, 1.0f);
	}
	// --Point
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
	// --Spot
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
		dst.inCosCutoff = src->inCosCutoff;
		dst.outCosCutoff = src->outCosCutoff;
	}

	glBindBuffer(GL_UNIFORM_BUFFER, lightingUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightingUBOData), &data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
// SHADOW UBO
void Scene::updateShadowUBO() const {
	ShadowMatricesUBOData data = {};

	// --Directional lights
	for (size_t i = 0; i < directionalLights.size() && i < MAX_LIGHTS; ++i) {
		if (directionalLights[i]->shadowCasterComponent) {
			data.directionalLightSpaceMatrices[i] = directionalLights[i]->shadowCasterComponent->getLightSpaceMatrix();
		}
		else {
			data.directionalLightSpaceMatrices[i] = glm::mat4(1.0f);
		}
	}
	// --Point lights
	for (size_t i = 0; i < pointLights.size(); ++i) {
		if (pointLights[i]->shadowCasterComponent) {
			data.pointLightSpaceMatrices[i] = pointLights[i]->shadowCasterComponent->getLightSpaceMatrix();
		}
	}
	// --Spot lights
	for (size_t i = 0; i < spotLights.size(); ++i) {
		if (spotLights[i]->shadowCasterComponent) {
			data.spotLightSpaceMatrices[i] = spotLights[i]->shadowCasterComponent->getLightSpaceMatrix();
		}
	}

	glBindBuffer(GL_UNIFORM_BUFFER, shadowUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShadowMatricesUBOData), &data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// LOAD EVERY SHADOW MAP TO OBJECT SHADERS
void Scene::setNodeShadowMapUniforms(SceneNode* node) const {
	if (node->object && node->object->shaderPtr) {
		node->object->shaderPtr->use();
		for (size_t i = 0; i < directionalLights.size(); ++i) {
			std::string uniformName = "DirectionalShadowMap[" + std::to_string(i) + "]";
			node->object->shaderPtr->setInt(uniformName, 10 + i);
		}
		for (size_t i = 0; i < pointLights.size(); ++i) {
			std::string uniformName = "PointShadowMap[" + std::to_string(i) + "]";
			node->object->shaderPtr->setInt(uniformName, 20 + i);
		}
		for (size_t i = 0; i < spotLights.size(); ++i) {
			std::string uniformName = "SpotShadowMap[" + std::to_string(i) + "]";
			node->object->shaderPtr->setInt(uniformName, 30 + i);
		}
	}

	for (auto& child : node->children) {
		setNodeShadowMapUniforms(child.get());
	}
}

/* ===== SELECTION DRAWING ================================================================= */
void Scene::initSelectionOutline() {
	m_outlineShader = m_assetManager->loadShaderObject("defaultshaders/outline.vert", "defaultshaders/outline.frag");
}
void Scene::drawSelectionStencil() const {
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
		m_outlineShader->use();
		m_outlineShader->setMat4("model", selectedNode->worldMatrix);
		selectedNode->object->modelPtr->draw(m_outlineShader.get());
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	// DRAWING THE OUTLINE
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glDisable(GL_DEPTH_TEST);

	m_outlineShader->use();
	m_outlineShader->setVec4("color", glm::vec4(0.8f, 0.4f, 1.0f, 0.2f));

	for (SceneNode* selectedNode : selectedEntities) {
		if (!selectedNode->object) continue;

		glm::mat4 fatterModel = glm::scale(selectedNode->worldMatrix, glm::vec3(1.03f));
		m_outlineShader->setMat4("model", fatterModel);
		selectedNode->object->modelPtr->draw(m_outlineShader.get());
	}

	// OPENGL STATE CLEANUP
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
}


/* ===== AABB BOX DRAWING ================================================================= */
void Scene::initDebugAABBDrawing() {
	// For the box
	float vertices[] = {
		0,0,0, 1,0,0,  1,0,0, 1,1,0,  1,1,0, 0,1,0,  0,1,0, 0,0,0, // Back
		0,0,1, 1,0,1,  1,0,1, 1,1,1,  1,1,1, 0,1,1,  0,1,1, 0,0,1, // Front
		0,0,0, 0,0,1,  1,0,0, 1,0,1,  1,1,0, 1,1,1,  0,1,0, 0,1,1  // Connections
	};

	m_debugVAO.bind();
	m_debugVBO = VBO(vertices, sizeof(vertices), GL_STATIC_DRAW);
	m_debugVAO.linkAttrib(m_debugVBO, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);
	m_debugVAO.unbind();

	m_debugShader = m_assetManager->loadShaderObject("defaultshaders/debug.vert", "defaultshaders/debug.frag");
}
void Scene::drawDebugAABBs(SceneNode* node) const {
	if (!m_debugShader) return;

	// Draw this node's AABB if it has an object
	if (node->object && node->object->modelPtr) {
		glm::vec3 size = node->object->modelPtr->aabb.max - node->object->modelPtr->aabb.min;
		glm::vec3 offset = node->object->modelPtr->aabb.min;

		glm::mat4 boxTransform = node->worldMatrix * glm::translate(glm::mat4(1.0f), offset) * glm::scale(glm::mat4(1.0f), size);


		// Check if this node is selected
		bool isSelected = false;
		for (auto* sel : selectedEntities) {
			if (sel == node) {
				isSelected = true;
				break;
			}
		}

		m_debugShader->use();
		m_debugShader->setMat4("model", boxTransform);
		m_debugShader->setVec3("color", isSelected ? glm::vec3(0, 1, 0) : glm::vec3(1, 1, 1));
		m_debugVAO.bind();
		glDrawArrays(GL_LINES, 0, 24);
		m_debugVAO.unbind();
	}

	// Recurse to children
	for (auto& child : node->children) {
		drawDebugAABBs(child.get());
	}
}