#include "headers/shadowCasterComponent.h"
#include "headers/frustum.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <iostream>


ShadowCasterComponent::ShadowCasterComponent(const glm::vec2 i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_width, float i_height, float i_nearPlane, float i_farPlane)
	: m_shadowMapResolution(i_shadowMapRes)
	, m_projectionType(i_projectionType)
	, m_planeWidth(i_width)
	, m_planeHeight(i_height)
	, m_nearPlane(i_nearPlane)
	, m_farPlane(i_farPlane)
{
	generateShadowMap();
}
ShadowCasterComponent::ShadowCasterComponent(const glm::vec2 i_shadowMapRes, Shadow_Map_Projection i_projectionType, float i_outCosCutoff, float i_width, float i_height, float i_nearPlane, float i_farPlane)
	: m_shadowMapResolution(i_shadowMapRes)
	, m_projectionType(i_projectionType)
	, m_fov(glm::degrees(acos(glm::clamp(i_outCosCutoff, -1.0f, 1.0f)) * 2.0f + glm::radians(2.0f)))
	, m_planeWidth(i_width)
	, m_planeHeight(i_height)
	, m_nearPlane(i_nearPlane)
	, m_farPlane(i_farPlane)
{
	generateShadowMap();
}

ShadowCasterComponent::~ShadowCasterComponent() {
	glDeleteFramebuffers(1, &m_fboID);
	glDeleteTextures(1, &m_depthMapTextureID);
}

void ShadowCasterComponent::updateFrustum() {
	frustum.constructFrustum(m_planeWidth / m_planeHeight, m_lightProjMat, m_lightViewMat);
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

	// *** FLAGS ***
	_isDirty = true;
	updateFrustum();
}

glm::mat4 ShadowCasterComponent::calcProjMat() const {
	switch (m_projectionType) {
	case Shadow_Map_Projection::ORTHOGRAPHIC:
		return glm::ortho(m_leftPlane, m_rightPlane,
			m_bottomPlane, m_topPlane,
			m_nearPlane, m_farPlane);
	case Shadow_Map_Projection::PERSPECTIVE:
		return glm::perspective(glm::radians(m_fov), (m_planeWidth / m_planeHeight), m_nearPlane, m_farPlane);
	default: break;
	}
}

glm::mat4 ShadowCasterComponent::calcViewMat(const glm::vec3& lightDirection, const glm::vec3& position) const {
	if (glm::length(lightDirection) < 0.001f) {
		return glm::mat4(1.0f);
	}

	glm::vec3 normDir = glm::normalize(lightDirection);
	glm::vec3 upVec = glm::vec3(0.0f, 1.0f, 0.0f);
	if (std::abs(glm::dot(glm::normalize(lightDirection), upVec)) > 0.99f) {
		upVec = glm::vec3(1.0f, 0.0f, 0.0f);
	}

	float depth = 50.0f;
	switch (m_projectionType) {
	case Shadow_Map_Projection::ORTHOGRAPHIC:
		glm::vec3 lightPos = position - normDir * depth;
		return glm::lookAt(lightPos, position, upVec);
	case Shadow_Map_Projection::PERSPECTIVE:
		return glm::lookAt(position, position + normDir, upVec);
	default:
		return glm::mat4(1.0f);
	}
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

void ShadowCasterComponent::generateShadowMap(bool linearFilter) {
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
		std::cerr << "ERROR: Shadow framebuffer not complete!" << '\n';
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// *** FLAGS ***
	_isDirty = false;
}