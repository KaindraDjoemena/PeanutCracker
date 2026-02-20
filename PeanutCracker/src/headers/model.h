#pragma once

#include "mesh.h"
#include "shader.h"
#include "material.h"

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


class AssetManager;

struct AABB {
	glm::vec3 min = glm::vec3(FLT_MAX);
	glm::vec3 max = glm::vec3(-FLT_MAX);
};

class Model {
public:
	std::vector<Mesh>    meshes;            // One mesh per material type
	std::shared_ptr<Material> material;
	std::string          directory;
	std::string          path;
	AABB				 aabb;
	bool                 gammaCorrection;

	Model(AssetManager* assetManager, std::string const& path, bool gamma = false);

	void draw(const Shader& shader);

	int loadModel(AssetManager* assetManager, std::string const& path);

private:
	Mesh processMesh(AssetManager* assetManager, aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
	
	void processNode(AssetManager* assetManager, aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
	
	glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from);
};

