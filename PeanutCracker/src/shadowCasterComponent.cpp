#include "headers/shadowCasterComponent.h"
#include "headers/frustum.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <iostream>


ShadowCasterComponent::ShadowCasterComponent(int i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_size, float i_nearPlane, float i_frustumDepth)
    : m_shadowMapResolution{ i_shadowMapRes, i_shadowMapRes }
    , m_projType(i_projectionType)
    , m_frustumDepth(i_frustumDepth)
    , m_planeWidth(i_size)
    , m_planeHeight(i_size)
    , m_nearPlane(i_nearPlane)
    , m_farPlane(i_nearPlane + i_frustumDepth)
    , m_leftPlane(-i_size)
    , m_rightPlane(i_size)
    , m_bottomPlane(-i_size)
    , m_topPlane(i_size)
{
    updateFrustum();
    genDirShadowMap();
}

ShadowCasterComponent::ShadowCasterComponent(bool isPoint, int i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_fov, float i_size, float i_nearPlane, float i_farPlane)
    : m_shadowMapResolution{ i_shadowMapRes, i_shadowMapRes }
    , m_projType(i_projectionType)
    , m_fov(i_fov)
    , m_planeWidth(i_size)
    , m_planeHeight(i_size)
    , m_nearPlane(i_nearPlane)
    , m_farPlane(i_farPlane)
    , m_leftPlane(-i_size)
    , m_rightPlane(i_size)
    , m_bottomPlane(-i_size)
    , m_topPlane(i_size)
{
    updateFrustum();
    if (isPoint) {
        genOmniShadowMap();
    }
    else {
        genDirShadowMap();
    }
}


void ShadowCasterComponent::setFOVDeg(float fov) {
    m_fov = fov * 2.0f + 2.0f;

    updateFrustum();
}

void ShadowCasterComponent::setNearPlane(float n) {
    m_nearPlane = n;

    updateFrustum();
}

void ShadowCasterComponent::setFarPlane(float f) {
    m_farPlane = f;
    
    updateFrustum();
}

void ShadowCasterComponent::setFrustumPlanes(float i_left, float i_right, float i_bottom, float i_top, float i_near, float i_far) {
    if (i_left >= i_right || i_bottom >= i_top || i_near >= i_far) {
        std::cerr << "Invalid frustum planes: left >= right or bottom >= top or near >= far" << '\n';
        return;
    }

    m_leftPlane = i_left;
    m_rightPlane = i_right;
    m_bottomPlane = i_bottom;
    m_topPlane = i_top;
    m_nearPlane = i_near;
    m_farPlane = i_far;

    updateFrustum();
}


void ShadowCasterComponent::updateFrustum() {
    frustum.constructFrustum(m_planeWidth / m_planeHeight, m_lightProjMat, m_lightViewMat);
}


glm::mat4 ShadowCasterComponent::calcProjMat() const {
    glm::mat4 projMat = glm::mat4(1.0f);
    switch (m_projType) {
    case Shadow_Map_Projection::ORTHOGRAPHIC:
        projMat = glm::ortho(m_leftPlane, m_rightPlane,
            m_bottomPlane, m_topPlane,
            m_nearPlane, m_farPlane);
        break;
    case Shadow_Map_Projection::PERSPECTIVE:
        projMat = glm::perspective(glm::radians(m_fov), (m_planeWidth / m_planeHeight), m_nearPlane, m_farPlane);
        break;
    default: break;
    }

    return projMat;
}

glm::mat4 ShadowCasterComponent::calcViewMat(const glm::vec3& lightDirection, const glm::vec3& position) const {
    if (glm::length(lightDirection) < 0.001f) {
        return glm::mat4(1.0f);
    }

    glm::vec3 normDir = glm::normalize(lightDirection);
    glm::vec3 upVec;
    if (std::abs(normDir.y) < 0.99f) {
        upVec = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    else {
        upVec = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 viewMat = glm::mat4(1.0f);
    glm::vec3 lightPos = position - normDir * m_frustumDepth;

    switch (m_projType) {
    case Shadow_Map_Projection::ORTHOGRAPHIC:
        viewMat = glm::lookAt(lightPos, position, upVec);   // FIX: light near frustum is 'pulled' by m_frustumDepth backward from (0, 0, 0). shadow frustum far plane at (0, 0, 0)
        break;
    case Shadow_Map_Projection::PERSPECTIVE:
        viewMat = glm::lookAt(position, position + normDir, upVec);
        break;
    default:
        break;
    }

    return viewMat;
}


void ShadowCasterComponent::calcLightSpaceMat(const glm::vec3& lightDirection, const glm::vec3& position) {
    if (glm::length(lightDirection) < 0.001f) {
        m_lightSpaceMatrix = glm::mat4(1.0f);
        return;
    }

    m_lightProjMat = calcProjMat();
    m_lightViewMat = calcViewMat(lightDirection, position);
    m_lightSpaceMatrix = m_lightProjMat * m_lightViewMat;
}

void ShadowCasterComponent::calcLightSpaceMats(const glm::vec3& position) {
    m_lightProjMat = calcProjMat();

    m_lightSpaceMatrices[0] = m_lightProjMat * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)); 
    m_lightSpaceMatrices[1] = m_lightProjMat * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    m_lightSpaceMatrices[2] = m_lightProjMat * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));  
    m_lightSpaceMatrices[3] = m_lightProjMat * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    m_lightSpaceMatrices[4] = m_lightProjMat * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)); 
    m_lightSpaceMatrices[5] = m_lightProjMat * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
}


void ShadowCasterComponent::genDirShadowMap(bool linearFilter) {
    glGenFramebuffers(1, &m_fboID);
    std::cout << "[SHADOW CASTER] Generating Shadow Map for: " << m_fboID << '\n';

    // --Creating the depth texture
    glGenTextures(1, &m_depthMapTextureID);
    glBindTexture(GL_TEXTURE_2D, m_depthMapTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_shadowMapResolution.x, m_shadowMapResolution.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    if (linearFilter) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // --PFC
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // --Attatching to the FOBs depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMapTextureID, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Directional Shadow framebuffer not complete!" << '\n';
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowCasterComponent::genOmniShadowMap(bool linearFilter) {
    glGenFramebuffers(1, &m_fboID);
    std::cout << "[SHADOW CASTER] Generating Omni Shadow Map for: " << m_fboID << '\n';

    glGenTextures(1, &m_depthMapTextureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_depthMapTextureID);

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, m_shadowMapResolution.x, m_shadowMapResolution.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }

    if (linearFilter) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthMapTextureID, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Omni Shadow framebuffer not complete!" << '\n';
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}