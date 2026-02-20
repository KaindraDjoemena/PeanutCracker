#include "headers/scene.h"
#include "headers/vao.h"
#include "headers/vbo.h"
#include "headers/shader.h"
//#include "headers/cubemap.h"
#include "headers/skybox.h"
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

	m_modelShader       = m_assetManager->loadShaderObject("model.vert", "model.frag");
	m_dirDepthShader    = m_assetManager->loadShaderObject("dirDepth.vert", "dirDepth.frag");
	m_omniDepthShader   = m_assetManager->loadShaderObject("omniDepth.vert", "omniDepth.frag", "omniDepth.geom");
	m_outlineShader     = m_assetManager->loadShaderObject("outline.vert", "outline.frag");
	m_primitiveShader   = m_assetManager->loadShaderObject("primitive.vert", "primitive.frag");
	m_postProcessShader = m_assetManager->loadShaderObject("postprocess.vert", "postprocess.frag");

	m_skyboxShader      = m_assetManager->loadShaderObject("skybox.vert", "skybox.frag");
	m_conversionShader  = m_assetManager->loadShaderObject("equirectToUnitCube.vert", "equirectToUnitCube.frag");
	m_convolutionShader = m_assetManager->loadShaderObject("cubemapConvolution.vert", "cubemapConvolution.frag");
	m_prefilterShader   = m_assetManager->loadShaderObject("prefilter.vert", "prefilter.frag");
	m_brdfShader        = m_assetManager->loadShaderObject("brdfLut.vert", "brdfLut.frag");

	setupUBOBindings();

	bindToUBOs(*m_modelShader);
	bindToUBOs(*m_dirDepthShader);
	bindToUBOs(*m_omniDepthShader);
	bindToUBOs(*m_outlineShader);
	bindToUBOs(*m_primitiveShader);
	bindToUBOs(*m_postProcessShader);
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
		createAndAddObject(path.string());
	}
	loadQueue.clear();
}


/* ===== ADDING ENTITIES ================================================================= */
void Scene::createAndAddObject(const std::string& modelPath) {
	std::shared_ptr<Model> modelPtr = m_assetManager->loadModel(modelPath);		// reads from the cache first

	// Instantiate new node
	std::string name = std::filesystem::path(modelPath).filename().string();
	auto newNode = std::make_unique<SceneNode>(name);
	newNode->object = std::make_unique<Object>(modelPtr.get());

	newNode->setSphereComponentRadius();

	m_selectedEntities.push_back(newNode.get());
	m_worldNode->addChild(std::move(newNode));
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
void Scene::createAndAddSkyboxHDR(const std::filesystem::path& path) {
	auto m_skyboxPtr = std::make_unique<Skybox>(
		path,
		*m_convolutionShader,
		*m_conversionShader,
		*m_prefilterShader,
		*m_brdfShader
	);
	setupSkyboxShaderUBOs(*m_skyboxShader);
	setupSkyboxShaderUBOs(*m_convolutionShader);

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
void Scene::deleteSkybox() {
	m_skybox.reset();
}


/* ===== UBOs ============================================================================*/
// --ALLOCATION
// gets called once in to set the binding points
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
// gets called for every shader
void Scene::bindToUBOs(const Shader& shader) const {
	// Camera
	unsigned int camIndex = glGetUniformBlockIndex(shader.ID, "CameraMatricesUBOData");
	if (camIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader.ID, camIndex, CAMERA_BINDING_POINT);
	}

	// Lighting
	unsigned int lightIndex = glGetUniformBlockIndex(shader.ID, "LightingUBOData");
	if (lightIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader.ID, lightIndex, LIGHTS_BINDING_POINT);
	}

	// Shadow
	unsigned int shadowIndex = glGetUniformBlockIndex(shader.ID, "ShadowMatricesUBOData");
	if (shadowIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader.ID, shadowIndex, SHADOW_BINDING_POINT);
	}
}
void Scene::setupSkyboxShaderUBOs(const Shader& shader) const {
	unsigned int camIndex = glGetUniformBlockIndex(shader.ID, "CameraMatricesUBOData");
	if (camIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader.ID, camIndex, CAMERA_BINDING_POINT);
	}
}

void Scene::updateShadowMapLSMats() const {
	// -- Updating light space matrices
	for (auto& dirLight : m_directionalLights) {
		// Check if direction is valid before calc
		if (glm::length(dirLight->direction) < 0.001f) {
			std::cerr << "ERROR: Direction is zero or near-zero!" << '\n';
		}
		if (glm::any(glm::isnan(dirLight->direction))) {
			std::cerr << "ERROR: Direction contains NaN!" << '\n';
		}

		dirLight->shadowCasterComponent.calcLightSpaceMat(dirLight->direction, glm::vec3(0.0f, 0.0f, 0.0f));

		// Check result immediately
		glm::mat4 test = dirLight->shadowCasterComponent.getLightSpaceMatrix();
		if (glm::any(glm::isnan(test[0]))) {
			std::cerr << "Matrix became NaN inside calcLightSpaceMat!" << '\n';
		}
	}
	for (auto& pointLight : m_pointLights) {
		pointLight->shadowCasterComponent.calcLightSpaceMats(pointLight->position);

		// Check result immediately
		std::array<glm::mat4, 6> test = pointLight->shadowCasterComponent.getLightSpaceMats();
		if (glm::any(glm::isnan(test[0][0]))) {
			std::cerr << "POINTLIGHT: Matrix became NaN inside calcLightSpaceMats!" << '\n';
		}
	}
	for (auto& spotLight : m_spotLights) {
		// Check if direction is valid before calc
		if (glm::length(spotLight->direction) < 0.001f) {
			std::cerr << "ERROR: Direction is zero or near-zero!" << '\n';
		}
		if (glm::any(glm::isnan(spotLight->direction))) {
			std::cerr << "ERROR: Direction contains NaN!" << '\n';
		}

		spotLight->shadowCasterComponent.calcLightSpaceMat(spotLight->direction, spotLight->position);

		// Check result immediately
		glm::mat4 test = spotLight->shadowCasterComponent.getLightSpaceMatrix();
		if (glm::any(glm::isnan(test[0]))) {
			std::cerr << "SPOTLIGHT: Matrix became NaN inside calcLightSpaceMat!" << '\n';
		}
	}
}

void Scene::init() {

	//initLightFrustumDebug();
	//initDebugAABBDrawing();

	//initSelectionOutline();
	bindToUBOs(*m_outlineShader);

	//if (m_skybox && m_skybox->shaderPtr) {
	//	setupSkyboxShaderUBOs(m_skybox->shaderPtr);
	//}
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
/*
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
		"lightFrustum.vert",
		"lightFrustum.frag"
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
		glm::mat4 lightVP = dirLight->shadowCasterComponent.getLightSpaceMatrix();
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
		glm::mat4 lightVP = spotLight->shadowCasterComponent.getLightSpaceMatrix();
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
*/


void Scene::bindDepthMaps() const {
	// --Directinoal lights
	for (size_t i = 0; i < m_directionalLights.size() && i < MAX_LIGHTS; ++i) {
		glActiveTexture(GL_TEXTURE0 + DIR_SHADOW_MAP_SLOT + i);
		glBindTexture(GL_TEXTURE_2D, m_directionalLights[i]->shadowCasterComponent.getDepthMapTexID());
	}
	// --Point lights
	for (size_t i = 0; i < m_pointLights.size() && i < MAX_LIGHTS; ++i) {
		glActiveTexture(GL_TEXTURE0 + POINT_SHADOW_MAP_SLOT + i);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointLights[i]->shadowCasterComponent.getDepthMapTexID());
	}
	// --Spot lights
	for (size_t i = 0; i < m_spotLights.size() && i < MAX_LIGHTS; ++i) {
		glActiveTexture(GL_TEXTURE0 + SPOT_SHADOW_MAP_SLOT + i);
		glBindTexture(GL_TEXTURE_2D, m_spotLights[i]->shadowCasterComponent.getDepthMapTexID());
	}

	glActiveTexture(GL_TEXTURE0);
}
void Scene::bindIBLMaps() const {
	if (m_skybox) {
		
		m_skybox->getIrradianceMap().bind(IRRADIANCE_MAP_SLOT);
		m_skybox->getPrefilterMap().bind(PREFILTER_MAP_SLOT);
		m_skybox->getBRDFLUT().bind(BRDF_LUT_SLOT);

	}
}

/* ===== UPDATING UBOs ================================================================= */
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

	data.numDirLights   = static_cast<int>(m_directionalLights.size());
	data.numPointLights = static_cast<int>(m_pointLights.size());
	data.numSpotLights  = static_cast<int>(m_spotLights.size());

	// --Directional
	for (size_t i = 0; i < m_directionalLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = m_directionalLights[i];
		auto& dst = data.directionalLight[i];
		
		dst.direction  = glm::vec4(src->direction, 0.0f);
		dst.color      = glm::vec4(src->light.color, 1.0f);
		dst.power      = src->light.power;
		dst.range      = src->range;
		dst.normalBias = src->light.normalBias;
		dst.depthBias  = src->light.depthBias;
	}
	// --Point
	for (size_t i = 0; i < m_pointLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = m_pointLights[i];
		auto& dst = data.pointLight[i];
		
		dst.position   = glm::vec4(src->position, 1.0f);
		dst.color      = glm::vec4(src->light.color, 1.0f);
		dst.power      = src->light.power;
		dst.radius     = src->radius;
		dst.normalBias = src->light.normalBias;
		dst.depthBias  = src->light.depthBias;
	}
	// --Spot
	for (size_t i = 0; i < m_spotLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = m_spotLights[i];
		auto& dst = data.spotLight[i];
		
		dst.position     = glm::vec4(src->position, 1.0f);
		dst.direction    = glm::vec4(src->direction, 0.0f);
		dst.color        = glm::vec4(src->light.color, 1.0f);
		dst.power        = src->light.power;
		dst.range        = src->range;
		dst.inCosCutoff  = src->inCosCutoff;
		dst.outCosCutoff = src->outCosCutoff;
		dst.normalBias   = src->light.depthBias;
		dst.depthBias    = src->light.depthBias;
	}

	glBindBuffer(GL_UNIFORM_BUFFER, lightingUBO);
	//glBufferData(GL_UNIFORM_BUFFER, sizeof(LightingUBOData), &data, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightingUBOData), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
// SHADOW UBO
void Scene::updateShadowUBO() const {
	ShadowMatricesUBOData data = {};

	// --Directional lights
	for (size_t i = 0; i < m_directionalLights.size() && i < MAX_LIGHTS; ++i) {
		data.directionalLightSpaceMatrices[i] = m_directionalLights[i]->shadowCasterComponent.getLightSpaceMatrix();
	}
	// --Spot lights
	for (size_t i = 0; i < m_spotLights.size(); ++i) {
		data.spotLightSpaceMatrices[i] = m_spotLights[i]->shadowCasterComponent.getLightSpaceMatrix();
	}

	glBindBuffer(GL_UNIFORM_BUFFER, shadowUBO);
	//glBufferData(GL_UNIFORM_BUFFER, sizeof(ShadowMatricesUBOData), &data, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShadowMatricesUBOData), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// LOAD EVERY SHADOW MAP TO OBJECT SHADERS
void Scene::setNodeShadowMapUniforms() const {
	m_modelShader->use();

	for (size_t i = 0; i < MAX_LIGHTS; ++i) {
		std::string uniformName = "DirectionalShadowMap[" + std::to_string(i) + "]";
		m_modelShader->setInt(uniformName, DIR_SHADOW_MAP_SLOT + i);
	}
	for (size_t i = 0; i < MAX_LIGHTS; ++i) {
		std::string uniformName = "PointShadowMap[" + std::to_string(i) + "]";
		m_modelShader->setInt(uniformName, POINT_SHADOW_MAP_SLOT + i);
	}
	for (size_t i = 0; i < MAX_LIGHTS; ++i) {
		std::string uniformName = "SpotShadowMap[" + std::to_string(i) + "]";
		m_modelShader->setInt(uniformName, SPOT_SHADOW_MAP_SLOT + i);
	}
}

void Scene::setNodeIBLMapUniforms() const {
	m_modelShader->use();
	m_modelShader->setInt("irradianceMap", IRRADIANCE_MAP_SLOT);
	m_modelShader->setInt("prefilterMap", PREFILTER_MAP_SLOT);
	m_modelShader->setInt("brdfLUT", BRDF_LUT_SLOT);
}

/* ===== SELECTION DRAWING ================================================================= */
/*
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
*/

/* ===== AABB BOX DRAWING ================================================================= */
/*
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

	m_debugShader = m_assetManager->loadShaderObject("debug.vert", "debug.frag");
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
*/