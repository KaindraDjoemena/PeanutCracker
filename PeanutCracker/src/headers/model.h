#pragma once

#include "mesh.h"
#include "shader.h"

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../../dependencies/stb_image/stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <map>
#include <vector>



struct AABB {
	glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
};

class Model {
public:
	// Temporary structure to hold data while grouping by material
	struct MeshBucket {
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
	};

	std::vector<Mesh>    meshes;            // One mesh per material type
	std::vector<Texture> textures_loaded;
	std::string          directory;
	std::string          path;
	AABB				 aabb;
	bool                 gammaCorrection;

	Model(std::string const& path, bool gamma = false);

	void draw(Shader* shader);

	int loadModel(std::string const& path);

private:
	void processNode(aiNode* node, const aiScene* scene, std::map<unsigned int, MeshBucket>& buckets);

	// High-level material loader
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, const aiScene* scene);

	// Helper to handle caching and actual texture type string assignment
	std::vector<Texture> loadMaterialTexturesByType(aiMaterial* mat, aiTextureType type, std::string typeName);
	
	unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);
};

