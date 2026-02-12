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

	ShadowCasterComponent() = default;
	ShadowCasterComponent(int i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_size, float i_nearPlane, float i_farPlane);
	ShadowCasterComponent(bool isPoint, int i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_fov, float i_size, float i_nearPlane, float i_farPlane);

	//~ShadowCasterComponent();




    // 1. DELETE COPYING (Prevents accidental duplication of OpenGL IDs)
    ShadowCasterComponent(const ShadowCasterComponent&) = delete;
    ShadowCasterComponent& operator=(const ShadowCasterComponent&) = delete;

    // 2. MOVE CONSTRUCTOR (Transfers ownership)
    ShadowCasterComponent(ShadowCasterComponent&& other) noexcept
        : m_depthMapTextureID(other.m_depthMapTextureID)
        , m_fboID(other.m_fboID)
        , m_shadowMapResolution(other.m_shadowMapResolution)
        , m_projType(other.m_projType)
        // ... copy other simple scalar members ...
        , m_lightSpaceMatrix(other.m_lightSpaceMatrix)
    {
        // NULLIFY the source so its destructor doesn't kill the texture
        other.m_depthMapTextureID = 0;
        other.m_fboID = 0;
    }

    // 3. MOVE ASSIGNMENT (Handles "shadowCaster = ShadowCaster(...)")
    ShadowCasterComponent& operator=(ShadowCasterComponent&& other) noexcept {
        if (this != &other) {
            // Clean up existing resources if this object already had them
            cleanup();

            // Steal resources
            m_depthMapTextureID = other.m_depthMapTextureID;
            m_fboID = other.m_fboID;
            m_shadowMapResolution = other.m_shadowMapResolution;
            // ... copy other scalars ...
            m_lightSpaceMatrix = other.m_lightSpaceMatrix;

            // Nullify source
            other.m_depthMapTextureID = 0;
            other.m_fboID = 0;
        }
        return *this;
    }

    // 4. DESTRUCTOR
    ~ShadowCasterComponent() {
        cleanup();
    }

    // Helper to avoid code duplication
    void cleanup() {
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

	void setFOVDeg(float fov) { m_fov = fov * 2.0f + 2.0f; _isDirty = true; updateFrustum(); }
	void setFrustumPlanes(float i_left, float i_right, float i_bottom, float i_top, float i_near, float i_far);
	void setNearPlane(float n) { m_nearPlane = n; _isDirty = true; updateFrustum(); }
	void setFarPlane(float f)  { m_farPlane  = f; _isDirty = true; updateFrustum(); }

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
	float m_nearPlane	= 0.01f;
	float m_farPlane	= 50.0f;

	float m_planeWidth  = 50.0f;
	float m_planeHeight = 50.0f;
	float m_fov			= 45.0f;

	float m_projectionDist = 50.0f;


	bool _isDirty = true;


	void genDirShadowMap(bool linearFilter = true);
	void genOmniShadowMap(bool linearFilter = true);
};