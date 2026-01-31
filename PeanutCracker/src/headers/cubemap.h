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


	Cubemap(const Faces& i_faces, Shader* i_shader);

	~Cubemap();

	void setShaderObject(Shader* i_shader);

	void draw() const;

private:
	unsigned int loadCubemap() const;

	void setupMesh();
};