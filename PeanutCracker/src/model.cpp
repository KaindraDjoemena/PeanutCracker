#include "headers/model.h"

#include "headers/mesh.h"
#include "headers/shader.h"
#include "headers/assetManager.h"

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <iostream>
#include <filesystem>
#include <vector>



Model::Model(
	AssetManager* assetManager,
	std::string const& path,
	bool gamma)
	: gammaCorrection(gamma)
	, path(path)
{
	std::cout << "[MODEL] Loading model: " << path << '\n';
	loadModel(assetManager, path);
}

void Model::draw(const Shader& shader) {
	for (unsigned int i = 0; i < meshes.size(); i++)
		meshes[i].draw(shader);
}

int Model::loadModel(AssetManager* assetManager, std::string const& path) {
	Assimp::Importer importer;

	unsigned int importFlags = (
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices);

	const aiScene* scene = importer.ReadFile(path, importFlags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
		return -1;
	}

	this->directory = std::filesystem::path(path).parent_path().string();

	processNode(assetManager, scene->mRootNode, scene, glm::mat4(1.0f));

	return 0;
}

void Model::processNode(AssetManager* assetManager, aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) {
	// Combine parent transform with this node's transform
	glm::mat4 nodeTransform = parentTransform * aiMatrix4x4ToGlm(node->mTransformation);

	// Process each mesh in this node
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(assetManager, mesh, scene, nodeTransform));
	}

	// Recursively process child nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(assetManager, node->mChildren[i], scene, nodeTransform);
	}
}

Mesh Model::processMesh(AssetManager* assetManager, aiMesh* mesh, const aiScene* scene, const glm::mat4& transform) {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	// Process Vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex;

		// Transform position by node's transform matrix
		glm::vec4 pos = transform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
		vertex.position = glm::vec3(pos);

		// Keep track of max and min AABB
		aabb.min.x = std::min(aabb.min.x, vertex.position.x);
		aabb.min.y = std::min(aabb.min.y, vertex.position.y);
		aabb.min.z = std::min(aabb.min.z, vertex.position.z);
		aabb.max.x = std::max(aabb.max.x, vertex.position.x);
		aabb.max.y = std::max(aabb.max.y, vertex.position.y);
		aabb.max.z = std::max(aabb.max.z, vertex.position.z);

		// Transform normals (use transpose of inverse for correct normal transformation)
		if (mesh->HasNormals()) {
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
			glm::vec3 normal = normalMatrix * glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			vertex.normal = glm::normalize(normal);
		}

		// Texture Coordinates, Tangents, Bitangents
		if (mesh->mTextureCoords[0]) {
			vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

			if (mesh->HasTangentsAndBitangents()) {
				glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
				vertex.tangent   = glm::normalize(normalMatrix * glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z));
				vertex.bitangent = glm::normalize(normalMatrix * glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z));
			}
			else {
				vertex.tangent   = glm::vec3(0.0f);
				vertex.bitangent = glm::vec3(0.0f);
			}
		}
		else {
			vertex.texCoords = glm::vec2(0.0f, 0.0f);
			vertex.tangent   = glm::vec3(0.0f);
			vertex.bitangent = glm::vec3(0.0f);
		}

		vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	auto material = assetManager->loadMaterial(scene->mMaterials[mesh->mMaterialIndex], directory);

	return Mesh(vertices, indices, material);
}

// Helper function to convert Assimp matrix to GLM matrix
glm::mat4 Model::aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
	glm::mat4 to;
	to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
	to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
	to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
	to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
	return to;
}