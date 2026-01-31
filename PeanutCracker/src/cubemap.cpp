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


Cubemap::Cubemap(const Faces& i_faces, Shader* i_shader)
	: faces(i_faces)
	, shaderPtr(i_shader)
{
	setupMesh();
}

Cubemap::~Cubemap() {
	if (texture != 0) glDeleteTextures(1, &texture);
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
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	vao.unbind();

	glDepthFunc(GL_LESS);
}

unsigned int Cubemap::loadCubemap() const {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < 6; i++) {
		std::string path = faces.cubeFaces[i].string();

		if (stbi_is_16_bit(path.c_str())) {
			stbi_us* data = stbi_load_16(path.c_str(), &width, &height, &nrChannels, 3);
			if (data) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT, data);
			}
			else {
				std::cout << "[CUBEMAP] 16-bit texture failed to load at path: " << faces.cubeFaces[i] << '\n';
			}
			stbi_image_free(data);
		}
		else {
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 3);
			if (data) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			}
			else {
				std::cout << "[CUBEMAP] 8-bit texture failed to load at path: " << faces.cubeFaces[i] << '\n';
			}
			stbi_image_free(data);
		}
	}

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
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
	texture = loadCubemap();
	vao.unbind();
}