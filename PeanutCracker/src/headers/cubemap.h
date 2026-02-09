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
	VAO          vao;
	VBO          vbo;
	Shader*      shaderPtr;
	Shader*      convolutionShaderPtr;
	unsigned int m_texID = 0;
	unsigned int m_fboID = 0;
	unsigned int m_rboID = 0;
	unsigned int m_envCubemapTexID = 0;
	unsigned int m_irradianceMapID = 0;


	Cubemap(const std::string& hdrPath, Shader* i_shader, Shader* i_convolutionShader, const Shader& i_conversionShader);

	~Cubemap();

	unsigned int getIrradianceMapID() const { return m_irradianceMapID; }

	void setShaderObject(Shader* i_shader);

	void draw() const;

private:
	unsigned int loadHDRTex(const std::string& hdrPath) const;
	void allocateEnvCubemapTex();
	void hdrEquirectToEnvCubemap(unsigned int hdrTexID, const Shader& shader);

	void convoluteCubemap();

	void setupMesh();
};