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
	m_worldNode = std::make_unique<SceneNode>("Root");
	m_dirDepthShader  = m_assetManager->loadShaderObject("defaultshaders/dirDepth.vert", "defaultshaders/dirDepth.frag");
	m_omniDepthShader = m_assetManager->loadShaderObject("defaultshaders/omniDepth.vert", "defaultshaders/omniDepth.geom", "defaultshaders/omniDepth.frag");
	m_outlineShader = m_assetManager->loadShaderObject("defaultshaders/outline.vert", "defaultshaders/outline.frag");
}
//Scene::Scene(const Scene&) = delete;
//Scene::Scene& operator = (const Scene&) = delete;

/* ===== OBJECT PICKING & OPERATIONS ================================================================= */
// --PICKING
void Scene::selectEntity(MouseRay& worldRay, bool isHoldingShift) {
	float shortestDist = FLT_MAX;
	SceneNode* bestNode = nullptr;

	if (m_worldNode) {
		findBestNodeRecursive(m_worldNode.get(), worldRay, shortestDist, bestNode);
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
	if (m_selectedEntities.empty()) return;
	std::cout << "[SCENE] Deleting " << m_selectedEntities.size() << " entities..." << '\n';

	for (SceneNode* node : m_selectedEntities) {
		if (node == m_worldNode.get()) {
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

	m_selectedEntities.clear();

	m_worldNode->update(glm::mat4(1.0f), false);

	std::cout << "[SCENE] Deleted selected entities" << '\n';
}
// --DUPLICATION
void Scene::duplicateSelectedEntities() {
	if (m_selectedEntities.empty()) return;
	std::cout << "[SCENE] Duplicating " << m_selectedEntities.size() << " selected entities..." << '\n';

	std::vector<SceneNode*> newSelection;

	for (SceneNode* source : m_selectedEntities) {
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

	m_selectedEntities = newSelection;

	m_worldNode->update(glm::mat4(1.0f), false);

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
			"defaultshaders/colll.frag"
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
	m_selectedEntities.push_back(newNode.get());
	m_worldNode->addChild(std::move(newNode));
}
void Scene::createAndAddSkybox(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
	std::shared_ptr<Shader> shaderObjectPtr = m_assetManager->loadShaderObject(vertPath, fragPath);
	/*auto m_skyboxPtr = std::make_unique<Cubemap>(faces, shaderObjectPtr.get());

	setupSkyboxShaderUBOs(shaderObjectPtr.get());
	m_skybox = std::move(m_skyboxPtr);*/
}
void Scene::createAndAddDirectionalLight(std::unique_ptr<DirectionalLight> light) {
	m_directionalLights.push_back(std::move(light));
	numDirectionalLights++;
}
void Scene::createAndAddPointLight(std::unique_ptr<PointLight> light) {
	m_pointLights.push_back(std::move(light));
	numPointLights++;
}
void Scene::createAndAddSpotLight(std::unique_ptr<SpotLight> light) {
	m_spotLights.push_back(std::move(light));
	numSpotLights++;
}
void Scene::createAndAddSkyboxHDR(const std::string& path) {
	std::shared_ptr<Shader> skyboxShader = m_assetManager->loadShaderObject("defaultshaders/skybox.vert", "defaultshaders/skybox.frag");
	std::shared_ptr<Shader> conversionShader = m_assetManager->loadShaderObject("defaultshaders/equirectToUnitCube.vert", "defaultshaders/equirectToUnitCube.frag");
	std::shared_ptr<Shader> convolutionShader = m_assetManager->loadShaderObject("defaultshaders/cubemapConvolution.vert", "defaultshaders/cubemapConvolution.frag");
	auto m_skyboxPtr = std::make_unique<Cubemap>(path, skyboxShader.get(), convolutionShader.get(), *conversionShader);

	setupSkyboxShaderUBOs(skyboxShader.get());
	setupSkyboxShaderUBOs(convolutionShader.get());
	m_skybox = std::move(m_skyboxPtr);
}


/* ===== DELETING ENTITIES ================================================================= */
void Scene::deleteDirLight(int index) {
	m_directionalLights.erase(m_directionalLights.begin() + index);
}
void Scene::deletePointLight(int index) {
	m_pointLights.erase(m_pointLights.begin() + index);
}
void Scene::deleteSpotLight(int index) {
	m_spotLights.erase(m_spotLights.begin() + index);
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
	if (!m_skybox || !shader) return;

	unsigned int camIndex = glGetUniformBlockIndex(shader->ID, "CameraMatricesUBOData");
	if (camIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader->ID, camIndex, CAMERA_BINDING_POINT);
	}
}

void Scene::updateShadowMapLSMats() const {
	// -- Updating light space matrices
	for (auto& dirLight : m_directionalLights) {
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
	for (auto& pointLight : m_pointLights) {
		if (pointLight->shadowCasterComponent) {

			pointLight->shadowCasterComponent->calcLightSpaceMats(pointLight->position);

			// Check result immediately
			std::array<glm::mat4, 6> test = pointLight->shadowCasterComponent->getLightSpaceMats();
			if (glm::any(glm::isnan(test[0][0]))) {
				std::cerr << "POINTLIGHT: Matrix became NaN inside calcLightSpaceMats!" << '\n';
			}
		}
	}
	for (auto& spotLight : m_spotLights) {
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
}


/* ===== RENDERING ================================================================================== */
// --For objects
void Scene::renderRecursive(const Camera& camera, SceneNode* node) const {
	// Camera Frustum Culling
	bool isVisible = true;
	if (node->sphereColliderComponent && node != m_worldNode.get()) {
		BoundingSphere boundingSphere = { node->sphereColliderComponent->worldCenter, node->sphereColliderComponent->worldRadius };
		if (!camera.getFrustum().isInFrustum(boundingSphere)) {
			isVisible = false;
		}
	}

	if (isVisible) {
		if (node->object) {
			node->object->draw(node->worldMatrix);
		}

		for (auto& child : node->children) {
			renderRecursive(camera, child.get());
		}
	}
}
// --For the shadow map
void Scene::renderShadowRecursive(const SceneNode* node, const Shader& depthShader) const {
	if (!node) return;
	
	if (node->object) {
		node->object->drawShadow(node->worldMatrix, depthShader);
	}

	for (auto& child : node->children) {
		renderShadowRecursive(child.get(), depthShader);
	}
}

void Scene::init() {
	setupUBOBindings();				// Camera, Lighting, Shadow

	initLightFrustumDebug();
	initDebugAABBDrawing();

	//initSelectionOutline();
	bindToUBOs(m_outlineShader.get());

	setupNodeUBOs(m_worldNode.get());

	if (m_skybox && m_skybox->shaderPtr) {
		setupSkyboxShaderUBOs(m_skybox->shaderPtr);
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
		auto it = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), node);
		if (it == m_selectedEntities.end()) {
			node->isSelected = true;
			m_selectedEntities.push_back(node);
		}
		else {
			node->isSelected = false;
			m_selectedEntities.erase(it);
		}
	}
	else {
		// Single select
		clearSelection();
		node->isSelected = true;
		m_selectedEntities.push_back(node);
	}
}
void Scene::clearSelection() {
	for (auto* node : m_selectedEntities) node->isSelected = false;
	m_selectedEntities.clear();
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

	for (auto& dirLight : m_directionalLights) {
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

	for (auto& spotLight : m_spotLights) {
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


void Scene::bindDepthMaps() const {
	// --Directinoal lights
	for (size_t i = 0; i < m_directionalLights.size() && i < MAX_LIGHTS; ++i) {
		if (m_directionalLights[i]->shadowCasterComponent) {
			glActiveTexture(GL_TEXTURE0 + 10 + i);
			glBindTexture(GL_TEXTURE_2D, m_directionalLights[i]->shadowCasterComponent->getDepthMapTexID());
		}
	}
	// --Point lights
	for (size_t i = 0; i < m_pointLights.size() && i < MAX_LIGHTS; ++i) {
		if (m_pointLights[i]->shadowCasterComponent) {
			glActiveTexture(GL_TEXTURE0 + 20 + i);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointLights[i]->shadowCasterComponent->getDepthMapTexID());
		}
	}
	// --Spot lights
	for (size_t i = 0; i < m_spotLights.size() && i < MAX_LIGHTS; ++i) {
		if (m_spotLights[i]->shadowCasterComponent) {
			glActiveTexture(GL_TEXTURE0 + 30 + i);
			glBindTexture(GL_TEXTURE_2D, m_spotLights[i]->shadowCasterComponent->getDepthMapTexID());
		}
	}
}
void Scene::bindIBLMaps() const {
	if (m_skybox) {
		glActiveTexture(GL_TEXTURE0 + 40);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_skybox->getIrradianceMapID());
	}
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

	data.numDirLights = static_cast<int>(m_directionalLights.size());
	data.numPointLights = static_cast<int>(m_pointLights.size());
	data.numSpotLights = static_cast<int>(m_spotLights.size());

	// --Directional
	for (size_t i = 0; i < m_directionalLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = m_directionalLights[i];
		auto& dst = data.directionalLight[i];
		dst.direction = glm::vec4(src->direction, 0.0f);
		dst.ambient = glm::vec4(src->light.ambient, 1.0f);
		dst.diffuse = glm::vec4(src->light.diffuse, 1.0f);
		dst.specular = glm::vec4(src->light.specular, 1.0f);
	}
	// --Point
	for (size_t i = 0; i < m_pointLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = m_pointLights[i];
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
	for (size_t i = 0; i < m_spotLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = m_spotLights[i];
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
	for (size_t i = 0; i < m_directionalLights.size() && i < MAX_LIGHTS; ++i) {
		if (m_directionalLights[i]->shadowCasterComponent) {
			data.directionalLightSpaceMatrices[i] = m_directionalLights[i]->shadowCasterComponent->getLightSpaceMatrix();
		}
		else {
			data.directionalLightSpaceMatrices[i] = glm::mat4(1.0f);
		}
	}
	// --Point lights
	//for (size_t i = 0; i < pointLights.size(); ++i) {
	//	if (pointLights[i]->shadowCasterComponent) {
	//		data.pointLightSpaceMatrices[i] = pointLights[i]->shadowCasterComponent->getLightSpaceMatrix();
	//	}
	//}
	// --Spot lights
	for (size_t i = 0; i < m_spotLights.size(); ++i) {
		if (m_spotLights[i]->shadowCasterComponent) {
			data.spotLightSpaceMatrices[i] = m_spotLights[i]->shadowCasterComponent->getLightSpaceMatrix();
		}
	}

	glBindBuffer(GL_UNIFORM_BUFFER, shadowUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShadowMatricesUBOData), &data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// LOAD EVERY SHADOW MAP TO OBJECT SHADERS
void Scene::setNodeShadowMapUniforms(const SceneNode* node) const {
	if (node->object && node->object->shaderPtr) {
		node->object->shaderPtr->use();
		for (size_t i = 0; i < m_directionalLights.size(); ++i) {
			std::string uniformName = "DirectionalShadowMap[" + std::to_string(i) + "]";
			node->object->shaderPtr->setInt(uniformName, 10 + i);
		}
		for (size_t i = 0; i < m_pointLights.size(); ++i) {
			std::string uniformName = "PointShadowMap[" + std::to_string(i) + "]";
			node->object->shaderPtr->setInt(uniformName, 20 + i);

			float farPlane = m_pointLights[i]->shadowCasterComponent->getFarPlane();
			if (farPlane <= 0.0f) {
				std::cerr << "ERROR: Far plane is zero/negative for point light " << i << std::endl;
			}
			node->object->shaderPtr->setFloat("omniShadowMapFarPlanes[" + std::to_string(i) + "]", farPlane);
		}
		for (size_t i = 0; i < m_spotLights.size(); ++i) {
			std::string uniformName = "SpotShadowMap[" + std::to_string(i) + "]";
			node->object->shaderPtr->setInt(uniformName, 30 + i);
		}
	}

	for (auto& child : node->children) {
		setNodeShadowMapUniforms(child.get());
	}
}

void Scene::setNodeIBLMapUniforms(const SceneNode* node) const {
	if (node->object && node->object->shaderPtr && m_skybox) {
		node->object->shaderPtr->use();

		int irradianceMapTexSlot = 40;		// TODO: ENUMS FOR THE TEXTURE SLOTS

		node->object->shaderPtr->setInt("irradianceMap", irradianceMapTexSlot);
	}

	for (auto& child : node->children) {
		setNodeIBLMapUniforms(child.get());
	}
}

/* ===== SELECTION DRAWING ================================================================= */
void Scene::drawSelectionStencil() const {
	if (m_selectedEntities.empty()) return;

	// WRITE TO THE STENCIL BUFFER
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);

	for (SceneNode* selectedNode : m_selectedEntities) {
		if (!selectedNode->object) continue;
		m_outlineShader->use();
		m_outlineShader->setMat4("model", selectedNode->worldMatrix);
		selectedNode->object->modelPtr->draw(*m_outlineShader);
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	// DRAWING THE OUTLINE
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glDisable(GL_DEPTH_TEST);

	m_outlineShader->use();
	m_outlineShader->setVec4("color", glm::vec4(0.8f, 0.4f, 1.0f, 0.2f));

	for (SceneNode* selectedNode : m_selectedEntities) {
		if (!selectedNode->object) continue;

		glm::mat4 fatterModel = glm::scale(selectedNode->worldMatrix, glm::vec3(1.03f));
		m_outlineShader->setMat4("model", fatterModel);
		selectedNode->object->modelPtr->draw(*m_outlineShader);
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
		for (auto* sel : m_selectedEntities) {
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