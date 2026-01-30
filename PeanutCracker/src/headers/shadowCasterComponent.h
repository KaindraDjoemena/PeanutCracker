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

	ShadowCasterComponent(const glm::vec2 i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_width, float i_height, float i_nearPlane, float i_farPlane);
	ShadowCasterComponent(const glm::vec2 i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_outCosCutoff, float i_width, float i_height, float i_nearPlane, float i_farPlane);

	~ShadowCasterComponent();

	unsigned int getDepthMapTexID() const { return m_depthMapTextureID; }
	unsigned int getFboID() const { return m_fboID; }
	std::array<float, 6> getPlanes() const { return {m_leftPlane, m_rightPlane, m_bottomPlane, m_topPlane, m_nearPlane, m_farPlane}; }
	glm::mat4 getLightSpaceMatrix() const { return m_lightSpaceMatrix; }
	glm::vec2 getShadowMapRes() const { return m_shadowMapResolution; }

	void updateFrustum();

	void setFrustumPlanes(float i_left, float i_right, float i_bottom, float i_top, float i_near, float i_far);

	glm::mat4 calcProjMat() const;

	glm::mat4 calcViewMat(const glm::vec3& lightDirection, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f)) const;

	void calcLightSpaceMat(const glm::vec3& lightDirection, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f));

private:
	unsigned int m_depthMapTextureID = 0;
	unsigned int m_fboID = 0;
	glm::vec2	 m_shadowMapResolution;

	Shadow_Map_Projection m_projectionType;
	glm::mat4 m_lightViewMat = glm::mat4(1.0f);
	glm::mat4 m_lightProjMat = glm::mat4(1.0f);
	glm::mat4	 m_lightSpaceMatrix = glm::mat4(1.0f);

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


	void generateShadowMap(bool linearFilter = true);
};