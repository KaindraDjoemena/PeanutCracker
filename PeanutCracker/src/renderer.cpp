#include "headers/renderer.h"

void Renderer::initScene(Scene& scene) {
	scene.init();
}

void Renderer::update(Scene& scene, Camera& cam, int vWidth, int vHeight) {
	glClearColor(m_winBgCol.r, m_winBgCol.g, m_winBgCol.b, m_winBgCol.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	if (m_viewportFBO.fbo == 0) {
		m_viewportFBO.setup(vWidth, vHeight);
	}
	else {
		m_viewportFBO.rescale(vWidth, vHeight);
	}

	cam.updateVectors();
	scene.updateShadowMapLSMats();
	scene.getWorldNode()->update(glm::mat4(1.0f), true);
	scene.updateCameraUBO(cam.getProjMat((float)vWidth / (float)vHeight), cam.getViewMat(), cam.getPos());
	scene.updateLightingUBO();
	scene.updateShadowUBO();
}

void Renderer::renderScene(const Scene& scene, const Camera& cam, int vWidth, int vHeight) const {
	// --Shadow map
	if (_usingShadowMap) {
		renderShadowPass(scene, cam);
	}

	// --Objects & skybox
	m_viewportFBO.bind(vWidth, vHeight);
	renderLightPass(scene, cam, vWidth, vHeight);

	renderSelectionHightlight(scene);
	m_viewportFBO.resolve();
	m_viewportFBO.unbind();
}

void Renderer::renderShadowPass(const Scene& scene, const Camera& cam) const {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	// --Rendering each object for each light from the lights pov,
	// and storing the depth value to an FBO
	// -- Directional lights
	scene.getDirDepthShader().use();
	for (auto& dirLight : scene.getDirectionalLights()) {
		if (!dirLight->shadowCasterComponent) continue;

		const glm::vec2 res = dirLight->shadowCasterComponent->getShadowMapRes();
		glViewport(0, 0, res.x, res.y);
		glBindFramebuffer(GL_FRAMEBUFFER, dirLight->shadowCasterComponent->getFboID());
		glClear(GL_DEPTH_BUFFER_BIT);

		scene.getDirDepthShader().setMat4("lightSpaceMatrix", dirLight->shadowCasterComponent->getLightSpaceMatrix());
		renderShadowMap(scene.getWorldNode(), scene.getDirDepthShader());
	}
	// -- Point lights
	scene.getOmniDepthShader().use();
	for (auto& pointLight : scene.getPointLights()) {
		if (!pointLight->shadowCasterComponent) continue;

		const glm::vec2 res = pointLight->shadowCasterComponent->getShadowMapRes();
		glViewport(0, 0, res.x, res.y);
		glBindFramebuffer(GL_FRAMEBUFFER, pointLight->shadowCasterComponent->getFboID());
		glClear(GL_DEPTH_BUFFER_BIT);

		std::array<glm::mat4, 6> lightSpaceMats = pointLight->shadowCasterComponent->getLightSpaceMats();
		scene.getOmniDepthShader().setMat4("shadowMatrices[0]", lightSpaceMats[0]);
		scene.getOmniDepthShader().setMat4("shadowMatrices[1]", lightSpaceMats[1]);
		scene.getOmniDepthShader().setMat4("shadowMatrices[2]", lightSpaceMats[2]);
		scene.getOmniDepthShader().setMat4("shadowMatrices[3]", lightSpaceMats[3]);
		scene.getOmniDepthShader().setMat4("shadowMatrices[4]", lightSpaceMats[4]);
		scene.getOmniDepthShader().setMat4("shadowMatrices[5]", lightSpaceMats[5]);
		scene.getOmniDepthShader().setVec3("lightPos", pointLight->position);
		scene.getOmniDepthShader().setFloat("farPlane", pointLight->shadowCasterComponent->getFarPlane());
		renderShadowMap(scene.getWorldNode(), scene.getOmniDepthShader());
	}
	// -- Spot lights
	scene.getDirDepthShader().use();
	for (auto& spotLight : scene.getSpotLights()) {
		if (!spotLight->shadowCasterComponent) continue;

		const glm::vec2 res = spotLight->shadowCasterComponent->getShadowMapRes();
		glViewport(0, 0, res.x, res.y);
		glBindFramebuffer(GL_FRAMEBUFFER, spotLight->shadowCasterComponent->getFboID());
		glClear(GL_DEPTH_BUFFER_BIT);

		scene.getDirDepthShader().setMat4("lightSpaceMatrix", spotLight->shadowCasterComponent->getLightSpaceMatrix());
		renderShadowMap(scene.getWorldNode(), scene.getDirDepthShader());
	}

	// --Reset opengl stuff
	glCullFace(GL_BACK);
}

void Renderer::renderLightPass(const Scene& scene, const Camera& cam, int vWidth, int vHeight) const {
	scene.bindDepthMaps();

	glActiveTexture(GL_TEXTURE0);
	// --Rendering the final scene
	renderSkybox(scene.getSkybox());


	//scene.getWorldNode()->update(glm::mat4(1.0f), true);

	scene.setNodeShadowMapUniforms(scene.getWorldNode());		// Set fragment shader shadow map uniformms

	if (_renderMode == Render_Mode::WIREFRAME) { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
	renderObjects(scene, scene.getWorldNode(), cam);
	if (_renderMode == Render_Mode::WIREFRAME) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }



	// --Drawing selected objects
	//drawSelectionStencil();

	// --Debug drawing
	//drawDirectionalLightFrustums(camera.getProjMat(vWidth / vHeight), camera.getViewMat());
	//drawSpotLightFrustums(camera.getProjMat(vWidth / vHeight), camera.getViewMat());
	//drawDebugAABBs(m_worldNode.get());
}

// Renders objects to the scene
void Renderer::renderObjects(const Scene& scene, const SceneNode* node, const Camera& cam) const {
	// Camera Frustum Culling
	bool isVisible = true;
	if (node->sphereColliderComponent && node != scene.getWorldNode()) {
		BoundingSphere boundingSphere = { node->sphereColliderComponent->worldCenter, node->sphereColliderComponent->worldRadius };
		if (!cam.getFrustum().isInFrustum(boundingSphere)) {
			isVisible = false;
		}
	}

	if (isVisible) {
		if (node->object) {
			node->object->draw(node->worldMatrix);
		}

		for (auto& child : node->children) {
			renderObjects(scene, child.get(), cam);
		}
	}
}

// Writes to the shadow map
void Renderer::renderShadowMap(const SceneNode* node, const Shader& depthShader) const {
	if (node->object) {
		node->object->drawShadow(node->worldMatrix, depthShader);
	}

	for (auto& child : node->children) {
		renderShadowMap(child.get(), depthShader);
	}
}

void Renderer::renderSkybox(const Cubemap* skybox) const {
	if (!skybox) return;

	skybox->draw();
}


void Renderer::renderSelectionHightlight(const Scene& scene) const {
	if (scene.getSelectedEnts().empty()) return;

	// WRITE TO THE STENCIL BUFFER
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);

	for (SceneNode* selectedNode : scene.getSelectedEnts()) {
		if (!selectedNode->object) continue;
		scene.getOutlineShader().use();
		scene.getOutlineShader().setMat4("model", selectedNode->worldMatrix);
		selectedNode->object->modelPtr->draw(scene.getOutlineShader());
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	// DRAWING THE OUTLINE
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glDisable(GL_DEPTH_TEST);

	scene.getOutlineShader().use();
	scene.getOutlineShader().setVec4("color", glm::vec4(0.8f, 0.4f, 1.0f, 0.2f));

	for (SceneNode* selectedNode : scene.getSelectedEnts()) {
		if (!selectedNode->object) continue;

		glm::mat4 fatterModel = glm::scale(selectedNode->worldMatrix, glm::vec3(1.03f));
		scene.getOutlineShader().setMat4("model", fatterModel);
		selectedNode->object->modelPtr->draw(scene.getOutlineShader());
	}

	// OPENGL STATE CLEANUP
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
}