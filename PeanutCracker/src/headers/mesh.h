#pragma once

#include "shader.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>


const unsigned int MAX_BONE_INFLUENCE = 4;


struct Vertex {
	glm::vec3   position;
	glm::vec3   normal;
	glm::vec2   texCoords;
	glm::vec3   tangent;
	glm::vec3   bitangent;

	int         m_boneIDs[MAX_BONE_INFLUENCE];
	float       m_weights[MAX_BONE_INFLUENCE];
};

struct Texture {
	unsigned int    id = 0;
	std::string     type;
	std::string     path;
};

class Mesh {
public:
	std::vector<Vertex>			vertices;
	std::vector<unsigned int>	indices;
	std::vector<Texture>		textures;
	unsigned int				VAO;

	Mesh(std::vector<Vertex> i_vertices, std::vector<unsigned int> i_indices, std::vector<Texture> i_textures);

	void draw(Shader* shader, bool isShadowPass = false);

private:
	unsigned int VBO, EBO;

	void setupMesh();
};