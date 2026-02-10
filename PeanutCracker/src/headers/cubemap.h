#pragma once

#include "vao.h"
#include "vbo.h"
#include "shader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <array>
#include <filesystem>


class Cubemap {
public:
	unsigned int m_texID = 0;
	unsigned int m_fboID = 0;
	unsigned int m_rboID = 0;
	unsigned int m_envCubemapTexID = 0;
	unsigned int m_irradianceMapID = 0;
	unsigned int m_preFilterMapID = 0;
	unsigned int m_BRDFLutTexID = 0;


	Cubemap(const std::string& hdrPath,
			const Shader& i_convolutionShader,
			const Shader& i_conversionShader,
			const Shader& i_prefilterShader,
			const Shader& i_brdfShader);

	~Cubemap();

	unsigned int getIrradianceMapID() const { return m_irradianceMapID; }
	unsigned int getPreFilterMapID()  const { return m_preFilterMapID; }
	unsigned int getBRDFLutTexID()    const { return m_BRDFLutTexID; }

	void draw(const Shader& shader) const;

private:
	VAO m_vao;
	VBO m_vbo;
	VAO m_quadVAO;
	VBO m_quadVBO;

	glm::mat4 m_captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	std::vector<glm::mat4> m_captureViews = {
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	unsigned int loadHDRTex(const std::string& hdrPath) const;
	void allocateEnvCubemapTex();
	void hdrEquirectToEnvCubemap(unsigned int hdrTexID, const Shader& shader);
	void convoluteCubemap(const Shader& convolutionShader);

	void genPreFilterCubemap(const Shader& prefilterShader);
	void genBRDFLut(const Shader& brdfShader);

	void setupMesh();
	void setupQuad();
};