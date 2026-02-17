#include "headers/renderer.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

void Renderer::initScene(Scene& scene) {
    setupUnitLine();
    setupUnitQuad();
    setupUnitCone();
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

    // --Debug
    renderSelectionHightlight(scene);
    renderLightAreas(scene, cam, vWidth, vHeight);


    m_viewportFBO.resolve();

    renderPostProcess(scene, vWidth, vHeight);

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
        const glm::vec2 res = dirLight->shadowCasterComponent.getShadowMapRes();
        glViewport(0, 0, res.x, res.y);
        glBindFramebuffer(GL_FRAMEBUFFER, dirLight->shadowCasterComponent.getFboID());

        glClear(GL_DEPTH_BUFFER_BIT);

        scene.getDirDepthShader().setMat4("lightSpaceMatrix", dirLight->shadowCasterComponent.getLightSpaceMatrix());
        renderShadowMap(scene.getWorldNode(), scene.getDirDepthShader());
    }
    // -- Point lights
    scene.getOmniDepthShader().use();
    for (auto& pointLight : scene.getPointLights()) {
        const glm::vec2 res = pointLight->shadowCasterComponent.getShadowMapRes();
        glViewport(0, 0, res.x, res.y);
        glBindFramebuffer(GL_FRAMEBUFFER, pointLight->shadowCasterComponent.getFboID());
        glClear(GL_DEPTH_BUFFER_BIT);

        std::array<glm::mat4, 6> lightSpaceMats = pointLight->shadowCasterComponent.getLightSpaceMats();
        scene.getOmniDepthShader().setMat4("shadowMatrices[0]", lightSpaceMats[0]);
        scene.getOmniDepthShader().setMat4("shadowMatrices[1]", lightSpaceMats[1]);
        scene.getOmniDepthShader().setMat4("shadowMatrices[2]", lightSpaceMats[2]);
        scene.getOmniDepthShader().setMat4("shadowMatrices[3]", lightSpaceMats[3]);
        scene.getOmniDepthShader().setMat4("shadowMatrices[4]", lightSpaceMats[4]);
        scene.getOmniDepthShader().setMat4("shadowMatrices[5]", lightSpaceMats[5]);
        scene.getOmniDepthShader().setVec3("lightPos", pointLight->position);
        scene.getOmniDepthShader().setFloat("farPlane", pointLight->shadowCasterComponent.getFarPlane());
        renderShadowMap(scene.getWorldNode(), scene.getOmniDepthShader());
    }
    // -- Spot lights
    scene.getDirDepthShader().use();
    for (auto& spotLight : scene.getSpotLights()) {
        const glm::vec2 res = spotLight->shadowCasterComponent.getShadowMapRes();
        glViewport(0, 0, res.x, res.y);
        glBindFramebuffer(GL_FRAMEBUFFER, spotLight->shadowCasterComponent.getFboID());
        glClear(GL_DEPTH_BUFFER_BIT);

        scene.getDirDepthShader().setMat4("lightSpaceMatrix", spotLight->shadowCasterComponent.getLightSpaceMatrix());
        renderShadowMap(scene.getWorldNode(), scene.getDirDepthShader());
    }

    // --Reset opengl stuff
    glCullFace(GL_BACK);
}

void Renderer::renderLightPass(const Scene& scene, const Camera& cam, int vWidth, int vHeight) const {
    scene.bindDepthMaps();
    scene.bindIBLMaps();

    if (scene.getSkybox()) {
        renderSkybox(scene);
    }

    scene.setNodeShadowMapUniforms(); // Set fragment shader shadow map uniformms
    scene.setNodeIBLMapUniforms();

    if (_renderMode == Render_Mode::WIREFRAME) { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
    renderObjects(scene, scene.getWorldNode(), cam);
    if (_renderMode == Render_Mode::WIREFRAME) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }

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
            node->object->draw(scene.getModelShader(), node->worldMatrix);
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

void Renderer::renderSkybox(const Scene& scene) const {
    scene.getSkybox()->draw(scene.getSkyboxShader());
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


    // === DRAWING THE OUTLINE ===========================================================
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

    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
}

// HACK: its pretty late im tired. gotta fix these uniform settings
void Renderer::renderLightAreas(const Scene& scene, const Camera& cam, int vWidth, int vHeight) const {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    const Shader& primitiveShader = scene.getPrimitiveShader();
    // TODO: PUT INSIDE SOME RENDERER CONST
    static const glm::vec3 yellowCol = glm::vec3(1.0f, 1.0f, 0.0f);
    static const glm::vec3 blueCol   = glm::vec3(0.0f, 0.0f, 1.0f);
    

    // TODO: USE A UBO
    primitiveShader.use();
    primitiveShader.setMat4("view", cam.getViewMat());
    primitiveShader.setMat4("projection", cam.getProjMat((float)vWidth / (float)vHeight));

    // --Dir light
    m_lineVAO.bind();
    primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::LINE));	// 0
    for (auto& dirLight : scene.getDirectionalLights()) {
        // light direction
        glm::mat4 arrowMat = calcLookAtMat(dirLight->position, dirLight->position + dirLight->direction);
        arrowMat = glm::scale(arrowMat, glm::vec3(1.0f, 1.0f, dirLight->range));

        primitiveShader.setMat4("model", arrowMat);
        primitiveShader.setVec3("color", yellowCol);
        glDrawArrays(GL_LINES, 0, 2);

        // light location
        m_quadVAO.bind();
        primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::SDF));   // 2
        glm::mat4 locMat = glm::scale(calcBillboardMat(dirLight->position, cam.getViewMat()), glm::vec3(0.15f));
        
        primitiveShader.setMat4("model", locMat);
        primitiveShader.setFloat("thickness", 0.5f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        m_lineVAO.bind();
        primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::LINE));   // 0
    }
    m_lineVAO.unbind();

    // --Point light
    m_quadVAO.bind();
    primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::SDF));   // 2
    for (auto& pointLight : scene.getPointLights()) {
        glm::mat4 baseMat = calcBillboardMat(pointLight->position, cam.getViewMat());
        
        // light area
        glm::mat4 areaMat = glm::scale(baseMat, glm::vec3(pointLight->radius));
        primitiveShader.setMat4("model", areaMat);
        primitiveShader.setVec3("color", yellowCol);
        primitiveShader.setFloat("thickness", 0.001f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // light location
        glm::mat4 locMat = glm::scale(baseMat, glm::vec3(0.1f));
        primitiveShader.setMat4("model", locMat);
        primitiveShader.setFloat("thickness", 0.5f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    m_quadVAO.unbind();

    // --Spot light
    m_coneVAO.bind();
    primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::LINE));	// 0
    for (auto& spotLight : scene.getSpotLights()) {
        glm::mat4 baseMat = calcLookAtMat(spotLight->position, spotLight->position + spotLight->direction);

        float outerAngle = std::acos(glm::clamp(spotLight->outCosCutoff, -1.0f, 1.0f));
        float innerAngle = std::acos(glm::clamp(spotLight->inCosCutoff, -1.0f, 1.0f));
        float outerRadius = spotLight->range * std::tan(outerAngle);
        float innerRadius = spotLight->range * std::tan(innerAngle);

        // outer cone
        primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::LINE));
        glm::mat4 outerConeMat = glm::scale(baseMat, glm::vec3(outerRadius, outerRadius, spotLight->range));
        primitiveShader.setMat4("model", outerConeMat);
        primitiveShader.setVec3("color", yellowCol);
        glDrawArrays(GL_LINES, 0, m_coneVertexCount);

        // inner cone
        primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::LINE));
        glm::mat4 innerConeMat = glm::scale(baseMat, glm::vec3(innerRadius, innerRadius, spotLight->range));
        primitiveShader.setMat4("model", innerConeMat);
        primitiveShader.setVec3("color", blueCol);
        glDrawArrays(GL_LINES, 0, m_coneVertexCount);

        // light location
        m_quadVAO.bind();
        primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::SDF));
        glm::mat4 locMat = glm::scale(calcBillboardMat(spotLight->position, cam.getViewMat()), glm::vec3(0.1f));
        primitiveShader.setMat4("model", locMat);
        primitiveShader.setVec3("color", yellowCol);
        primitiveShader.setFloat("thickness", 0.5f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glm::vec3 basePos  = spotLight->position + spotLight->direction * spotLight->range;
        glm::mat4 ringBase = calcLookAtMat(basePos, basePos + spotLight->direction);

        // outer ring
        primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::SDF));
        primitiveShader.setMat4("model", glm::scale(ringBase, glm::vec3(outerRadius)));
        primitiveShader.setVec3("color", yellowCol);
        primitiveShader.setFloat("thickness", 0.001f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // inner ring
        primitiveShader.setInt("mode", static_cast<int>(Primitive_Mode::SDF));
        primitiveShader.setFloat("dashCount", 16.0f);
        primitiveShader.setFloat("dashRatio", 0.5f);
        primitiveShader.setMat4("model", glm::scale(ringBase, glm::vec3(innerRadius)));
        primitiveShader.setVec3("color", blueCol);
        primitiveShader.setFloat("thickness", 0.001f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        m_coneVAO.bind();
    }
    m_coneVAO.unbind();

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}


void Renderer::setupUnitCone() {
    // TODO: USE ARRAYS?
    std::vector<float> coneVertices;
    int segments = 32;

    // 1. Generate the Base Circle (Lines connecting segments)
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / (float)segments * 2.0f * glm::pi<float>();
        float angle2 = (float)(i + 1) / (float)segments * 2.0f * glm::pi<float>();

        // Point A
        coneVertices.push_back(cos(angle1)); coneVertices.push_back(sin(angle1)); coneVertices.push_back(-1.0f);
        // Point B
        coneVertices.push_back(cos(angle2)); coneVertices.push_back(sin(angle2)); coneVertices.push_back(-1.0f);
    }

    // 2. Generate the "Spokes" (4 lines from Tip to Circle at 0, 90, 180, 270 degrees)
    for (int i = 0; i < 4; i++) {
        float angle = (float)i / 4.0f * 2.0f * glm::pi<float>();

        // Tip (Apex)
        coneVertices.push_back(0.0f); coneVertices.push_back(0.0f); coneVertices.push_back(0.0f);
        // Base Point
        coneVertices.push_back(cos(angle)); coneVertices.push_back(sin(angle)); coneVertices.push_back(-1.0f);
    }

    m_coneVertexCount = (int)coneVertices.size() / 3;

    m_coneVAO.bind();
    m_coneVBO.setData(coneVertices.data(), coneVertices.size());
    m_coneVAO.linkAttrib(m_coneVBO, VertLayout::POS, 3 * sizeof(float), (void*)0);
    m_coneVAO.unbind();
}
void Renderer::setupUnitQuad() {
    float quadVertices[] = {
        // XYZ					// UV
        -1.0f,  1.0f,  0.0f,	0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f,    0.0f, 0.0f,
         1.0f, -1.0f,  0.0f,    1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f,    0.0f, 1.0f,
         1.0f, -1.0f,  0.0f,    1.0f, 0.0f,
         1.0f,  1.0f,  0.0f,    1.0f, 1.0f
    };

    m_quadVAO.bind();
    m_quadVBO.setData(quadVertices, sizeof(quadVertices));
    m_quadVAO.linkAttrib(m_quadVBO, VertLayout::POS, 5 * sizeof(float), (void*)0);
    m_quadVAO.linkAttrib(m_quadVBO, VertLayout::UV,  5 * sizeof(float), (void*)(3 * sizeof(float)));
    m_quadVAO.unbind();
}
void Renderer::setupUnitLine() {
    float lineVertices[] = {
        0.0f, 0.0f,  0.0f,	// origin
        0.0f, 0.0f, -1.0f	// destination
    };

    m_lineVAO.bind();
    m_lineVBO.setData(lineVertices, sizeof(lineVertices), GL_STATIC_DRAW);
    m_lineVAO.linkAttrib(m_lineVBO, VertLayout::POS, 3 * sizeof(float), (void*)0);
    m_lineVAO.unbind();
}

void Renderer::renderPostProcess(const Scene& scene, int vWidth, int vHeight) const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_viewportFBO.screenFbo);
    glViewport(0, 0, vWidth, vHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    scene.getPostProcessShader().use();
    scene.getPostProcessShader().setFloat("EV100", m_EV100);

    glBindTexture(GL_TEXTURE_2D, m_viewportFBO.resolveTexture);
    scene.getPostProcessShader().setInt("hdrBuffer", 0);

    m_quadVAO.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_quadVAO.unbind();

    glEnable(GL_DEPTH_TEST);
}