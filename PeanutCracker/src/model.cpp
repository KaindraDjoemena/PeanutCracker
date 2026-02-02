#include "headers/model.h"
#include "headers/mesh.h"
#include "headers/shader.h"

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



Model::Model(std::string const& path, bool gamma)
	: gammaCorrection(gamma), path(path) {
	std::cout << "[MODEL] Loading model: " << path << '\n';
	loadModel(path);
}

void Model::draw(const Shader& shader) {
	for (unsigned int i = 0; i < meshes.size(); i++)
		meshes[i].draw(shader);
}

int Model::loadModel(std::string const& path) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
		aiProcess_FlipUVs | aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
		return -1;
	}

	this->directory = std::filesystem::path(path).parent_path().string();

	// 1. Group all geometry by Material Index
	std::map<unsigned int, MeshBucket> buckets;
	processNode(scene->mRootNode, scene, buckets);

	// 2. Finalize each bucket into a single Mesh object
	for (auto& [materialIndex, data] : buckets) {
		aiMaterial* material = scene->mMaterials[materialIndex];
		std::vector<Texture> textures = loadMaterialTextures(material, scene);

		// This creates the large VBO/VAO/EBO for the entire material group
		meshes.push_back(Mesh(data.vertices, data.indices, textures));
	}

	return 0;
}

void Model::processNode(aiNode* node, const aiScene* scene, std::map<unsigned int, MeshBucket>& buckets) {
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		unsigned int matIndex = mesh->mMaterialIndex;
		MeshBucket& bucket = buckets[matIndex];

		// Offset indices by current number of vertices in this bucket
		unsigned int vertexOffset = static_cast<unsigned int>(bucket.vertices.size());

		// Process Vertices
		for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
			Vertex vertex;
			glm::vec3 vector;

			// Positions
			vector.x = mesh->mVertices[j].x;
			vector.y = mesh->mVertices[j].y;
			vector.z = mesh->mVertices[j].z;
			vertex.position = vector;

			// Keep track of max and min AABB
			aabb.min.x = std::min(aabb.min.x, vertex.position.x);
			aabb.min.y = std::min(aabb.min.y, vertex.position.y);
			aabb.min.z = std::min(aabb.min.z, vertex.position.z);
			aabb.max.x = std::max(aabb.max.x, vertex.position.x);
			aabb.max.y = std::max(aabb.max.y, vertex.position.y);
			aabb.max.z = std::max(aabb.max.z, vertex.position.z);

			// Normals
			if (mesh->HasNormals()) {
				vector.x = mesh->mNormals[j].x;
				vector.y = mesh->mNormals[j].y;
				vector.z = mesh->mNormals[j].z;
				vertex.normal = vector;
			}

			// Texture Coordinates, Tangents, Bitangents
			if (mesh->mTextureCoords[0]) {
				vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);

				if (mesh->HasTangentsAndBitangents()) {
					vertex.tangent = glm::vec3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
					vertex.bitangent = glm::vec3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
				}
			}
			else {
				vertex.texCoords = glm::vec2(0.0f, 0.0f);
			}

			bucket.vertices.push_back(vertex);
		}

		// Process Indices
		for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			for (unsigned int k = 0; k < face.mNumIndices; k++)
				bucket.indices.push_back(face.mIndices[k] + vertexOffset);
		}
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene, buckets);
	}
}

// High-level material loader
std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, const aiScene* scene) {
	std::vector<Texture> textures;

	auto diffuse = loadMaterialTexturesByType(mat, aiTextureType_DIFFUSE, "texture_diffuse");
	textures.insert(textures.end(), diffuse.begin(), diffuse.end());

	auto specular = loadMaterialTexturesByType(mat, aiTextureType_SPECULAR, "texture_specular");
	textures.insert(textures.end(), specular.begin(), specular.end());

	auto normal = loadMaterialTexturesByType(mat, aiTextureType_HEIGHT, "texture_normal");
	textures.insert(textures.end(), normal.begin(), normal.end());

	auto height = loadMaterialTexturesByType(mat, aiTextureType_AMBIENT, "texture_height");
	textures.insert(textures.end(), height.begin(), height.end());

	return textures;
}

// Helper to handle caching and actual texture type string assignment
std::vector<Texture> Model::loadMaterialTexturesByType(aiMaterial* mat, aiTextureType type, std::string typeName) {
	std::vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
		aiString str;
		mat->GetTexture(type, i, &str);
		bool skip = false;

		for (unsigned int j = 0; j < textures_loaded.size(); j++) {
			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
				textures.push_back(textures_loaded[j]);
				skip = true;
				break;
			}
		}
		if (!skip) {
			std::cout << "[MODEL] creating texture..." << '\n';
			Texture texture;
			texture.id = TextureFromFile(str.C_Str(), this->directory);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}
	return textures;
}

unsigned int Model::TextureFromFile(const char* path, const std::string& directory, bool gamma) {
	std::string filename = (std::filesystem::path(directory) / std::filesystem::path(path)).string();

	std::cout << "[MODEL] loading texture from " << filename << "..." << '\n';

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = GL_RED;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);

		std::cout << "[MODEL] done" << '\n';
	}
	else
	{
		std::cout << "[MODEL] texture failed to load at " << path << '\n';
		stbi_image_free(data);
	}

	return textureID;
}
