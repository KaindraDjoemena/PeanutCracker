#ifndef SHADOW_CASTER_COMPONENT_H
#define SHADOW_CASTER_COMPONENT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


enum class Shadow_Map_Projection {
	ORTHOGRAPHIC,
	PERSPECTIVE
};

struct Light_Frustum {
	glm::vec2 leftRight;
	glm::vec2 bottomTop;
	glm::vec2 nearFar;

	Light_Frustum() = default;

	Light_Frustum(const glm::vec2& i_leftRight,
			const glm::vec2& i_bottomTop,
			const glm::vec2& i_nearFar)
		: leftRight(i_leftRight)
		, bottomTop(i_bottomTop)
		, nearFar(i_nearFar) {}
};

class ShadowCasterComponent {
public:
	ShadowCasterComponent(const glm::vec2 i_shadowMapRes, Shadow_Map_Projection i_projectionType)
		: m_shadowMapResolution(i_shadowMapRes)
		, projectionType(i_projectionType) 
	{
		m_frustum = Light_Frustum(glm::vec2(-m_projectionDist, m_projectionDist),
								  glm::vec2(-m_projectionDist, m_projectionDist),
								  glm::vec2(m_nearPlane, m_farPlane));
		generateShadowMap();
	}

	~ShadowCasterComponent() {
		glDeleteFramebuffers(1, &m_fboID);
		glDeleteTextures(1, &m_depthMapTextureID);
	}

	unsigned int getDepthMapTexID() const { return m_depthMapTextureID; }
	unsigned int getFboID() const { return m_fboID; }
	glm::mat4 getLightSpaceMatrix() const{ return m_lightSpaceMatrix; }
	glm::vec2 getShadowMapRes() const { return m_shadowMapResolution; }
	Light_Frustum getFrustum() const { return m_frustum; }

	void setFrustum(const glm::vec2& i_leftRight, const glm::vec2& i_bottomTop, const glm::vec2& i_nearFar) {
		m_frustum.leftRight = i_leftRight;
		m_frustum.bottomTop = i_bottomTop;
		m_frustum.nearFar	= i_nearFar;
	}

	void calcLightSpaceMat(const glm::vec3& lightDirection) {
		glm::mat4 lightProjection = glm::mat4(1.0f);

		switch (projectionType) {
			case Shadow_Map_Projection::ORTHOGRAPHIC:
				lightProjection = glm::ortho(m_frustum.leftRight.x, m_frustum.leftRight.y,
											 m_frustum.bottomTop.x, m_frustum.bottomTop.y,
											 m_frustum.nearFar.x, m_frustum.nearFar.y);
				break;
			case Shadow_Map_Projection::PERSPECTIVE: 
				// TODO: ADD THIS
				break;
			default: break;
		}
		
		glm::vec3 lightPos  = glm::vec3(0.0f);
		glm::vec3 targetPos = lightDirection;
		glm::vec3 upVec		= glm::vec3(0.0f, 1.0f, 0.0f);

		// If the light is pointing up/down, upVec uses the x axis
		if (std::abs(glm::dot(lightDirection, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) {
			upVec = glm::vec3(1.0f, 0.0f, 0.0f);
		}

		glm::mat4 lightView = glm::lookAt(lightPos, targetPos, upVec);

		m_lightSpaceMatrix = lightProjection * lightView;
	}

private:
	unsigned int m_depthMapTextureID = 0;
	unsigned int m_fboID = 0;
	glm::mat4	 m_lightSpaceMatrix;
	glm::vec2	 m_shadowMapResolution;

	Shadow_Map_Projection projectionType;
	float m_nearPlane	= 0.1f;
	float m_farPlane	= 50.0f;

	float m_projectionDist = 50.0f;

	Light_Frustum m_frustum;

	void generateShadowMap() {
		glGenFramebuffers(1, &m_fboID);

		// --Creating the depth texture
		glGenTextures(1, &m_depthMapTextureID);
		glBindTexture(GL_TEXTURE_2D, m_depthMapTextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_shadowMapResolution.x, m_shadowMapResolution.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		// --Attatching to the FOBs depth buffer
		glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMapTextureID, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);




		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cerr << "[ERROR] ShadowMap FBO is not complete!" << std::endl;
		}
		else {
			std::cout << "[SUCCESS] ShadowMap FBO is complete." << std::endl;
		}




		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};

#endif