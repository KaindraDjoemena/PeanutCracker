#pragma once
#include "frustum.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <iostream>


enum class Shadow_Map_Projection {
    ORTHOGRAPHIC,
    PERSPECTIVE
};

class ShadowCasterComponent {
public:
    Frustum frustum;

    ShadowCasterComponent() = delete;
    ShadowCasterComponent(int i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_size, float i_nearPlane, float i_farPlane);
    ShadowCasterComponent(bool isPoint, int i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_fov, float i_size, float i_nearPlane, float i_farPlane);

    ShadowCasterComponent(const ShadowCasterComponent&) = delete;
    ShadowCasterComponent& operator = (const ShadowCasterComponent&) = delete;

    ShadowCasterComponent(ShadowCasterComponent&& other) noexcept;
    ShadowCasterComponent& operator = (ShadowCasterComponent&& other) noexcept;

    ~ShadowCasterComponent() {
        if (m_depthMapTextureID != 0) {
            glDeleteTextures(1, &m_depthMapTextureID);
            m_depthMapTextureID = 0;
        }
        if (m_fboID != 0) {
            glDeleteFramebuffers(1, &m_fboID);
            m_fboID = 0;
        }
    }

    unsigned int getDepthMapTexID() const { return m_depthMapTextureID; }
    unsigned int getFboID() const { return m_fboID; }
    std::array<float, 6> getPlanes() const { return {m_leftPlane, m_rightPlane, m_bottomPlane, m_topPlane, m_nearPlane, m_farPlane}; }
    float getFarPlane() const { return m_farPlane; }
    float getNearPlane() const { return m_nearPlane; }
    glm::mat4 getLightSpaceMatrix() const { return m_lightSpaceMatrix; }
    std::array<glm::mat4, 6> getLightSpaceMats() const { return m_lightSpaceMatrices; }
    glm::vec2 getShadowMapRes() const { return m_shadowMapResolution; }

    void updateFrustum();

    void setFOVDeg(float fov);
    void setNearPlane(float n);
    void setFarPlane(float f);
    void setFrustumPlanes(float i_left, float i_right, float i_bottom, float i_top, float i_near, float i_far);

    glm::mat4 calcProjMat() const;
    glm::mat4 calcViewMat(const glm::vec3& lightDirection, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f)) const;

    void calcLightSpaceMat(const glm::vec3& lightDirection, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f));
    void calcLightSpaceMats(const glm::vec3& position);

private:
    unsigned int m_depthMapTextureID = 0;
    unsigned int m_fboID = 0;
    glm::vec2	 m_shadowMapResolution;

    Shadow_Map_Projection m_projType;
    glm::mat4 m_lightViewMat = glm::mat4(1.0f);
    glm::mat4 m_lightProjMat = glm::mat4(1.0f);
    glm::mat4	 m_lightSpaceMatrix = glm::mat4(1.0f);
    std::array<glm::mat4, 6> m_lightSpaceMatrices;

    float m_leftPlane	= -50.0f;
    float m_rightPlane	= 50.0f;
    float m_bottomPlane	= -50.0f;
    float m_topPlane	= 50.0f;
    float m_nearPlane	= 0.001f;
    float m_farPlane	= 50.0f;

    float m_planeWidth  = 50.0f;
    float m_planeHeight = 50.0f;
    float m_fov			= 45.0f;

    float m_projectionDist = 50.0f;

    float m_frustumDepth = 50.0f;


    void genDirShadowMap(bool linearFilter = true);
    void genOmniShadowMap(bool linearFilter = true);
};