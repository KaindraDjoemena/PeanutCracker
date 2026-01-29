#ifndef CUBEMAP_H
#define CUBEMAP_H

#include "../../dependencies//stb_image/stb_image.h"
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


struct Faces {
	std::array<std::filesystem::path, 6> cubeFaces;

	Faces(const std::filesystem::path& i_right_dir,		// +X
		  const std::filesystem::path& i_left_dir,		// -X
		  const std::filesystem::path& i_top_dir,		// +Y
		  const std::filesystem::path& i_bottom_dir,	// -Y
		  const std::filesystem::path& i_front_dir,		// +Z
		  const std::filesystem::path& i_back_dir		// -Z
	) : cubeFaces{ i_right_dir, i_left_dir, i_top_dir, i_bottom_dir, i_front_dir, i_back_dir } {}

};

class Cubemap {
public:
	VAO          vao;
	VBO          vbo;
	Shader*      shaderPtr;
	Faces        faces;
	unsigned int texture = 0;


	Cubemap(const Faces& i_faces, Shader* i_shader)
		: faces(i_faces)
		, shaderPtr(i_shader)
	{
		setupMesh();
	}

	~Cubemap() {
		if (texture != 0) glDeleteTextures(1, &texture);
	}

	void setShaderObject(Shader* i_shader) {
		shaderPtr = i_shader;
	}

	void draw() const {
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

private:
	unsigned int loadCubemap() const {
		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		int width, height, nrChannels;
		for (unsigned int i = 0; i < 6; i++) {
			unsigned char* data = stbi_load(faces.cubeFaces[i].string().c_str(), &width, &height, &nrChannels, 0);
			if (data) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			}
			else {
				std::cout << "[CUBEMAP] texture failed to load at path: " << faces.cubeFaces[i] << '\n';
			}

			stbi_image_free(data);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		return textureID;
	}

	void setupMesh() {
		float skybox_vertices[] = {
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
		vbo = VBO(skybox_vertices, 108 * sizeof(float));
		vao.linkAttrib(vbo, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);
		texture = loadCubemap();
		vao.unbind();
	}
};

#endif