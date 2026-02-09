#include "headers/cubemap.h"
#include "headers/vao.h"
#include "headers/vbo.h"
#include "headers/shader.h"

#include <stb_image.h>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <array>
#include <filesystem>


Cubemap::Cubemap(const std::string& hdrPath, Shader* i_shader, Shader* i_convolutionShader, const Shader& i_conversionShader)
	: shaderPtr(i_shader)
	, convolutionShaderPtr(i_convolutionShader)
{
	setupMesh();	// Bind VAO / VBO
	unsigned int equirectTex = loadHDRTex(hdrPath);	// Load .hdr as 2d texture
	allocateEnvCubemapTex();	// Allocate dan configure cubemap texture
	hdrEquirectToEnvCubemap(equirectTex, i_conversionShader);	// Make cubemap texture

	glDeleteTextures(1, &equirectTex);

	// Environment map convolution
	convoluteCubemap();
}

Cubemap::~Cubemap() {
	glDeleteFramebuffers(1, &m_fboID);
	glDeleteRenderbuffers(1, &m_rboID);
	glDeleteTextures(1, &m_envCubemapTexID);
	glDeleteTextures(1, &m_irradianceMapID);
}

void Cubemap::setShaderObject(Shader* i_shader) {
	shaderPtr = i_shader;
}

void Cubemap::draw() const {
	if (!shaderPtr) return;

	shaderPtr->use();

	glDepthFunc(GL_LEQUAL);

	vao.bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemapTexID);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	vao.unbind();

	glDepthFunc(GL_LESS);
}

// Load .hdr to equirectangular texture
unsigned int Cubemap::loadHDRTex(const std::string& hdrPath) const {
	int width, height, nrChannels;
	float* data = stbi_loadf(hdrPath.c_str(), &width, &height, &nrChannels, 0);
	unsigned int hdrTexID;
	if (data) {
		glGenTextures(1, &hdrTexID);
		glBindTexture(GL_TEXTURE_2D, hdrTexID);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		stbi_image_free(data);
	}
	else {
		std::cout << "[CUBEMAP] Failed to load HDR image" << std::endl;
	}

	return hdrTexID;
}

// Allocate the environment cubemap texture
void Cubemap::allocateEnvCubemapTex() {
	glGenFramebuffers(1, &m_fboID);
	glGenRenderbuffers(1, &m_rboID);
	std::cout << "[CUBEMAP] Generating Cube Map for: " << m_fboID << '\n';

	// Cubemap texture
	glGenTextures(1, &m_envCubemapTexID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemapTexID);

	for (unsigned int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
	
	glBindRenderbuffer(GL_RENDERBUFFER, m_rboID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rboID);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "ERROR: Equirect cubemap framebuffer not complete!" << '\n';
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// 1. Project HDR equirect texture to a unit cube
// 2. Capture the 6 faces of the cube in a framebuffer
// 3. From the frame buffer, write to the 6 faces of the environment cubemap (GL_TEXTURE_CUBE_MAP_POSITIVE_X, ...)
void Cubemap::hdrEquirectToEnvCubemap(unsigned int hdrTexID, const Shader& shader) {
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] = {
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	shader.use();
	shader.setInt("equirectMap", 0);
	shader.setMat4("projectionMat", captureProjection);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexID);

	glViewport(0, 0, 512, 512);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);

	// Write to the environment cubemap
	for (unsigned int i = 0; i < 6; i++) {
		shader.setMat4("viewMat", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_envCubemapTexID, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, 36);
		vao.unbind();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Cubemap::convoluteCubemap() {
	std::cout << "[CUBEMAP] Convoluting cubemap" << std::endl;
	glGenTextures(1, &m_irradianceMapID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_irradianceMapID);
	for (unsigned int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// 2. Prep the FBO for a smaller size
	glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
	glBindRenderbuffer(GL_RENDERBUFFER, m_rboID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
	std::cout << "[CUBEMAP] Irradiance cubemap fbo: " << m_fboID << std::endl;

	// 3. The Bake
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] = {
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};
	convolutionShaderPtr->use();
	convolutionShaderPtr->setInt("environmentMap", 0);
	convolutionShaderPtr->setMat4("projection", captureProjection);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemapTexID); // Sample from the HDR skybox we just made

	glViewport(0, 0, 32, 32);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	for (unsigned int i = 0; i < 6; ++i) {
		convolutionShaderPtr->setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_irradianceMapID, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, 36);
		vao.unbind();
	}
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Cubemap::setupMesh() {
	static const std::array<float, 108> skybox_vertices = {
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f
	};

	vao.bind();
	vbo = VBO(skybox_vertices.data(), skybox_vertices.size() * sizeof(float), GL_STATIC_DRAW);
	vao.linkAttrib(vbo, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);
	vao.unbind();
}