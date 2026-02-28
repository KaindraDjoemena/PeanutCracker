#include "headers/scene.h"
#include "headers/vao.h"
#include "headers/vbo.h"
#include "headers/shader.h"
//#include "headers/cubemap.h"
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

	m_modelShader       = m_assetManager->loadShaderObject("model.vert", "model.frag");
	m_dirDepthShader    = m_assetManager->loadShaderObject("dirDepth.vert", "dirDepth.frag");
	m_omniDepthShader   = m_assetManager->loadShaderObject("omniDepth.vert", "omniDepth.frag", "omniDepth.geom");
	m_outlineShader     = m_assetManager->loadShaderObject("outline.vert", "outline.frag");
	m_pickingShader		= m_assetManager->loadShaderObject("picking.vert", "picking.frag");
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
	bindToUBOs(*m_skyboxShader);
	bindToUBOs(*m_convolutionShader);

	bindToUBOs(*m_outlineShader);
	bindToUBOs(*m_primitiveShader);
	
	bindToUBOs(*m_postProcessShader);

	generateBRDFLUT();
}
//Scene::Scene(const Scene&) = delete;
//Scene::Scene& operator = (const Scene&) = delete;

/* ===== OBJECT PICKING & OPERATIONS ================================================================= */
//--PICKING
SceneNode* Scene::getNodeByPickingID(uint32_t targetID) const {
	uint32_t id = 1;
	
	// NOTE:: THIS IS LINEAR
	std::function<SceneNode*(SceneNode*)> find = [&](SceneNode* node) -> SceneNode* {
		if (node->object) {
			if (id == targetID) return node;
			++id;
		}
		for (auto& child : node->children) {
			if (SceneNode* result = find(child.get())) return result;
		}
		return nullptr;
	};

	return find(m_worldNode.get());
}
// TODO: MOVE TO SOME INPUT LAYER
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
void Scene::createAndAddReflectionProbe(std::unique_ptr<RefProbe> probe) {
	m_refProbes.push_back(std::move(probe));
	numRefProbes++;
}
void Scene::createAndAddSkyboxHDR(const std::filesystem::path& path) {
	auto m_skyboxPtr = std::make_unique<Cubemap>(
		path,
		*m_convolutionShader,
		*m_conversionShader,
		*m_prefilterShader
	);

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

	glGenBuffers(1, &m_cameraMatricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, m_cameraMatricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraMatricesUBOData), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, CAMERA_BINDING_POINT, m_cameraMatricesUBO);

	glGenBuffers(1, &m_lightingUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, m_lightingUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightingUBOData), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, LIGHTS_BINDING_POINT, m_lightingUBO);

	glGenBuffers(1, &m_refProbeUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, m_refProbeUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ReflectionProbeUBOData), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, REF_PROBE_BINDING_POINT, m_refProbeUBO); 

	glGenBuffers(1, &m_shadowUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, m_shadowUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShadowMatricesUBOData), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, SHADOW_BINDING_POINT, m_shadowUBO);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// --SHADER BINDING
// gets called for every shader
void Scene::bindToUBOs(const Shader& shader) const {

	unsigned int camIndex = glGetUniformBlockIndex(shader.ID, "CameraMatricesUBOData");
	if (camIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader.ID, camIndex, CAMERA_BINDING_POINT);
	}

	unsigned int lightIndex = glGetUniformBlockIndex(shader.ID, "LightingUBOData");
	if (lightIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader.ID, lightIndex, LIGHTS_BINDING_POINT);
	}

	unsigned int refProbeIndex = glGetUniformBlockIndex(shader.ID, "ReflectionProbeUBOData");
	if (refProbeIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader.ID, refProbeIndex, REF_PROBE_BINDING_POINT);
	}

	unsigned int shadowIndex = glGetUniformBlockIndex(shader.ID, "ShadowMatricesUBOData");
	if (shadowIndex != GL_INVALID_INDEX) {
		glUniformBlockBinding(shader.ID, shadowIndex, SHADOW_BINDING_POINT);
	}
}


void Scene::updateCameraUBO(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) const {
	CameraMatricesUBOData data = { projection, view, glm::vec4(cameraPos, 1.0f) };

	glBindBuffer(GL_UNIFORM_BUFFER, m_cameraMatricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraMatricesUBOData), &data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Scene::updateLightingUBO() const {
	LightingUBOData data = {};

	data.numDirLights = static_cast<int>(m_directionalLights.size());
	data.numPointLights = static_cast<int>(m_pointLights.size());
	data.numSpotLights = static_cast<int>(m_spotLights.size());

	for (size_t i = 0; i < m_directionalLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = m_directionalLights[i];
		auto& dst = data.directionalLight[i];

		dst.direction = glm::vec4(src->direction, 0.0f);
		dst.color = glm::vec4(src->light.color, 1.0f);
		dst.power = src->light.power;
		dst.range = src->range;
		dst.normalBias = src->light.normalBias;
		dst.depthBias = src->light.depthBias;
	}

	for (size_t i = 0; i < m_pointLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = m_pointLights[i];
		auto& dst = data.pointLight[i];

		dst.position = glm::vec4(src->position, 1.0f);
		dst.color = glm::vec4(src->light.color, 1.0f);
		dst.power = src->light.power;
		dst.radius = src->radius;
		dst.normalBias = src->light.normalBias;
		dst.depthBias = src->light.depthBias;
	}

	for (size_t i = 0; i < m_spotLights.size() && i < MAX_LIGHTS; ++i) {
		auto& src = m_spotLights[i];
		auto& dst = data.spotLight[i];

		dst.position = glm::vec4(src->position, 1.0f);
		dst.direction = glm::vec4(src->direction, 0.0f);
		dst.color = glm::vec4(src->light.color, 1.0f);
		dst.power = src->light.power;
		dst.range = src->range;
		dst.inCosCutoff = src->inCosCutoff;
		dst.outCosCutoff = src->outCosCutoff;
		dst.normalBias = src->light.normalBias;
		dst.depthBias = src->light.depthBias;
	}

	glBindBuffer(GL_UNIFORM_BUFFER, m_lightingUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightingUBOData), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Scene::updateRefProbeUBO() const {
	ReflectionProbeUBOData data = {};

	data.numRefProbes = numRefProbes;
	for (size_t i = 0; i < m_refProbes.size() && i < MAX_LIGHTS; ++i) {
		data.position[i]     = glm::vec4(m_refProbes[i]->transform.position, 1.0f);
		data.worldMats[i]    = m_refProbes[i]->transform.getModelMatrix();
		data.invWorldMats[i] = glm::inverse(m_refProbes[i]->transform.getModelMatrix());
		data.proxyDims[i]	 = glm::vec4(m_refProbes[i]->proxyDims, 1.0f);
	}

	glBindBuffer(GL_UNIFORM_BUFFER, m_refProbeUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ReflectionProbeUBOData), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Scene::updateShadowUBO() const {
	ShadowMatricesUBOData data = {};

	for (size_t i = 0; i < m_directionalLights.size() && i < MAX_LIGHTS; ++i) {
		data.directionalLightSpaceMatrices[i] = m_directionalLights[i]->shadowCasterComponent.getLightSpaceMatrix();
	}

	for (size_t i = 0; i < m_spotLights.size(); ++i) {
		data.spotLightSpaceMatrices[i] = m_spotLights[i]->shadowCasterComponent.getLightSpaceMatrix();
	}

	glBindBuffer(GL_UNIFORM_BUFFER, m_shadowUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShadowMatricesUBOData), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


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

void Scene::setNodeRefMapUniforms() const {

	m_modelShader->use();

	for (size_t i = 0; i < MAX_LIGHTS; ++i) {
		std::string uniformName = "refEnvMap[" + std::to_string(i) + "]";
		m_modelShader->setInt(uniformName, REF_ENV_MAP_SLOT + i);
	}
}

void Scene::updateShadowMapLSMats() const {

	for (auto& dirLight : m_directionalLights) {
		if (glm::length(dirLight->direction) < 0.001f) {
			std::cerr << "ERROR: Direction is zero or near-zero!" << '\n';
		}
		if (glm::any(glm::isnan(dirLight->direction))) {
			std::cerr << "ERROR: Direction contains NaN!" << '\n';
		}

		dirLight->shadowCasterComponent.calcLightSpaceMat(dirLight->direction, glm::vec3(0.0f, 0.0f, 0.0f));

		glm::mat4 test = dirLight->shadowCasterComponent.getLightSpaceMatrix();
		if (glm::any(glm::isnan(test[0]))) {
			std::cerr << "Matrix became NaN inside calcLightSpaceMat!" << '\n';
		}
	}

	for (auto& pointLight : m_pointLights) {
		pointLight->shadowCasterComponent.calcLightSpaceMats(pointLight->position);

		std::array<glm::mat4, 6> test = pointLight->shadowCasterComponent.getLightSpaceMats();
		if (glm::any(glm::isnan(test[0][0]))) {
			std::cerr << "POINTLIGHT: Matrix became NaN inside calcLightSpaceMats!" << '\n';
		}
	}
	
	for (auto& spotLight : m_spotLights) {
		if (glm::length(spotLight->direction) < 0.001f) {
			std::cerr << "ERROR: Direction is zero or near-zero!" << '\n';
		}
		if (glm::any(glm::isnan(spotLight->direction))) {
			std::cerr << "ERROR: Direction contains NaN!" << '\n';
		}

		spotLight->shadowCasterComponent.calcLightSpaceMat(spotLight->direction, spotLight->position);

		glm::mat4 test = spotLight->shadowCasterComponent.getLightSpaceMatrix();
		if (glm::any(glm::isnan(test[0]))) {
			std::cerr << "SPOTLIGHT: Matrix became NaN inside calcLightSpaceMat!" << '\n';
		}
	}
}

void Scene::bindDepthMaps() const {

	for (size_t i = 0; i < m_directionalLights.size() && i < MAX_LIGHTS; ++i) {
		glActiveTexture(GL_TEXTURE0 + DIR_SHADOW_MAP_SLOT + i);
		glBindTexture(GL_TEXTURE_2D, m_directionalLights[i]->shadowCasterComponent.getDepthMapTexID());
	}

	for (size_t i = 0; i < m_pointLights.size() && i < MAX_LIGHTS; ++i) {
		glActiveTexture(GL_TEXTURE0 + POINT_SHADOW_MAP_SLOT + i);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointLights[i]->shadowCasterComponent.getDepthMapTexID());
	}

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
		m_brdfLUT.bind(BRDF_LUT_SLOT);
	}
}
void Scene::bindRefProbeMaps() const {
	for (size_t i = 0; i < m_refProbes.size() && i < MAX_LIGHTS; ++i) {
		m_refProbes[i]->localEnvMap.getPrefilterMap().bind(REF_ENV_MAP_SLOT + i);
	}
}


// NOTE: MOVE TO RENDERER CLASS
/* ===== UTILITIES ============================================================ */
void Scene::generateBRDFLUT() {
	std::cout << "[SCENE] Generating BRDF LUT\n";

	//--VAO & VBO
	VAO quadVAO;
	VBO quadVBO;
	const std::array<float, 30> quadVertices = {
		// positions            // texCoords
		-1.0f,  1.0f, 0.0f,     0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f,     0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,     1.0f, 0.0f,

		-1.0f,  1.0f, 0.0f,     0.0f, 1.0f,
		 1.0f, -1.0f, 0.0f,     1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,     1.0f, 1.0f
	};
	quadVAO.bind();
	quadVBO.setData(quadVertices.data(), quadVertices.size() * sizeof(float), GL_STATIC_DRAW);
	quadVAO.linkAttrib(quadVBO, VertLayout::POS, 5 * sizeof(float), (void*)0);
	quadVAO.linkAttrib(quadVBO, VertLayout::UV, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	quadVAO.unbind();

	//--FBO & RBO
	GLuint fbo;
	GLuint rbo;
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_brdfLUT.getID(), 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "ERROR: Equirect cubemap framebuffer not complete!" << '\n';
	}

	//--Writing to the LUT
	glViewport(0, 0, 512, 512);
	m_brdfShader->use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	quadVAO.bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);
	quadVAO.unbind();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}