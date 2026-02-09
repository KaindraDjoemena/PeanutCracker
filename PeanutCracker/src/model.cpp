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
#include <assimp/pbrmaterial.h>

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
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
		return -1;
	}

	this->directory = std::filesystem::path(path).parent_path().string();

	// Process each mesh individually instead of bucketing by material
	processNode(scene->mRootNode, scene, glm::mat4(1.0f));

	return 0;
}

void Model::processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) {
	// Combine parent transform with this node's transform
	glm::mat4 nodeTransform = parentTransform * aiMatrix4x4ToGlm(node->mTransformation);

	// Process each mesh in this node
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene, nodeTransform));
	}

	// Recursively process child nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene, nodeTransform);
	}
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform) {
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
				vertex.tangent = glm::normalize(normalMatrix * glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z));
				vertex.bitangent = glm::normalize(normalMatrix * glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z));
			}
			else {
				vertex.tangent = glm::vec3(0.0f);
				vertex.bitangent = glm::vec3(0.0f);
			}
		}
		else {
			vertex.texCoords = glm::vec2(0.0f, 0.0f);
			vertex.tangent = glm::vec3(0.0f);
			vertex.bitangent = glm::vec3(0.0f);
		}

		vertices.push_back(vertex);
	}

	// Process Indices (unchanged)
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	// Process Material
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	std::vector<Texture> textures = loadMaterialTextures(material, scene);

	return Mesh(vertices, indices, textures);
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

// High-level material loader
unsigned int createDefaultTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a, bool sRGB = false, bool singleChannel = false) {
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	unsigned char pixel[] = { r, g, b, a };

	if (singleChannel) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &r);
	}
	else {
		GLenum internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}

// Add static members to Model class (or make them global)
static unsigned int defaultAlbedoTex;
static unsigned int defaultMetallicTex;
static unsigned int defaultRoughnessTex;
static unsigned int defaultAOTex;
static unsigned int defaultNormalTex;
static bool defaultsInitialized;

// Call this once before loading any models (e.g., in Model constructor or main)
static void initDefaultTextures() {
	if (!defaultsInitialized) {
		defaultAlbedoTex = createDefaultTexture(255, 0, 255, 255, true, false);   // Magenta

		// Blue channel = metallic value (0 = non-metallic)
		defaultMetallicTex = createDefaultTexture(0, 0, 0, 255, false, false);

		// Green channel = roughness value (255 = fully rough)
		defaultRoughnessTex = createDefaultTexture(0, 255, 0, 255, false, false);

		defaultAOTex = createDefaultTexture(255, 255, 255, 255, false, true);
		defaultNormalTex = createDefaultTexture(128, 128, 255, 255, false, false);
		defaultsInitialized = true;
	}
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, const aiScene* scene) {
	initDefaultTextures();
	std::vector<Texture> textures;

	aiString matName;
	mat->Get(AI_MATKEY_NAME, matName);
	std::cout << "[MODEL] Processing material: " << matName.C_Str() << std::endl;

	// === ALBEDO / BASE COLOR ===
	auto albedo = loadMaterialTexturesByType(mat, aiTextureType_DIFFUSE, "albedoMap", true);
	if (albedo.empty()) {
		albedo = loadMaterialTexturesByType(mat, aiTextureType_BASE_COLOR, "albedoMap", true);
	}
	if (albedo.empty()) {
		aiColor4D color;
		if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
			std::cout << "[MODEL] Creating solid color texture: ("
				<< color.r << ", " << color.g << ", " << color.b << ")" << std::endl;

			float sR = std::pow(color.r, 1.0f / 2.2f);
			float sG = std::pow(color.g, 1.0f / 2.2f);
			float sB = std::pow(color.b, 1.0f / 2.2f);

			unsigned char r = static_cast<unsigned char>(sR * 255.0f);
			unsigned char g = static_cast<unsigned char>(sG * 255.0f);
			unsigned char b = static_cast<unsigned char>(sB * 255.0f);
			unsigned char a = static_cast<unsigned char>(color.a * 255.0f);

			Texture solidColorTex;
			solidColorTex.id = createDefaultTexture(r, g, b, a, true, false);  // sRGB format
			solidColorTex.type = "albedoMap";
			solidColorTex.path = "solid_color";
			textures.push_back(solidColorTex);
		}
		else {
			Texture def; def.id = defaultAlbedoTex; def.type = "albedoMap"; def.path = "default";
			textures.push_back(def);
		}
	}
	else {
		textures.insert(textures.end(), albedo.begin(), albedo.end());
	}

	// === NORMAL MAP ===
	auto normal = loadMaterialTexturesByType(mat, aiTextureType_NORMALS, "normalMap", false);
	if (normal.empty())
		normal = loadMaterialTexturesByType(mat, aiTextureType_HEIGHT, "normalMap", false);
	if (normal.empty()) {
		Texture def; def.id = defaultNormalTex; def.type = "normalMap"; def.path = "default";
		textures.push_back(def);
	}
	else {
		textures.insert(textures.end(), normal.begin(), normal.end());
	}

	// === METALLIC & ROUGHNESS ===
	// GLTF typically stores these in a single texture: R=unused, G=roughness, B=metallic
	auto metallicRoughness = loadMaterialTexturesByType(mat, aiTextureType_UNKNOWN, "metallicMap", false);

	// Try alternate type if not found
	if (metallicRoughness.empty()) {
		metallicRoughness = loadMaterialTexturesByType(mat, aiTextureType_METALNESS, "metallicMap", false);
	}

	if (!metallicRoughness.empty()) {
		// Found combined texture - use it for both
		std::cout << "[MODEL] Using combined metallicRoughness texture" << std::endl;

		// Add for metallic (will read .b channel in shader)
		for (auto& tex : metallicRoughness) {
			tex.type = "metallicMap";
			textures.push_back(tex);
		}

		// Add same texture for roughness (will read .g channel in shader)
		for (auto& tex : metallicRoughness) {
			Texture roughTex = tex;
			roughTex.type = "roughnessMap";
			textures.push_back(roughTex);
		}
	}
	else {
		// No combined texture - try separate or use factors
		std::cout << "[MODEL] No combined texture, checking for factors..." << std::endl;

		// Metallic - put in BLUE channel, use LINEAR color space (not sRGB)
		float metallicFactor = 0.0f;
		if (mat->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == AI_SUCCESS) {
			std::cout << "[MODEL] Metallic factor: " << metallicFactor << std::endl;
			unsigned char val = static_cast<unsigned char>(metallicFactor * 255.0f);
			Texture metallicTex;
			// R=0, G=0, B=metallic_value, no sRGB encoding
			metallicTex.id = createDefaultTexture(0, 0, val, 255, false, false);
			metallicTex.type = "metallicMap";
			metallicTex.path = "solid_metallic";
			textures.push_back(metallicTex);
		}
		else {
			Texture def; def.id = defaultMetallicTex; def.type = "metallicMap"; def.path = "default";
			textures.push_back(def);
		}

		// Roughness - put in GREEN channel, use LINEAR color space (not sRGB)
		float roughnessFactor = 1.0f;
		if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == AI_SUCCESS) {
			std::cout << "[MODEL] Roughness factor: " << roughnessFactor << std::endl;
			unsigned char val = static_cast<unsigned char>(roughnessFactor * 255.0f);
			Texture roughnessTex;
			// R=0, G=roughness_value, B=0, no sRGB encoding
			roughnessTex.id = createDefaultTexture(0, val, 0, 255, false, false);
			roughnessTex.type = "roughnessMap";
			roughnessTex.path = "solid_roughness";
			textures.push_back(roughnessTex);
		}
		else {
			Texture def; def.id = defaultRoughnessTex; def.type = "roughnessMap"; def.path = "default";
			textures.push_back(def);
		}
	}

	// === AO ===
	auto ao = loadMaterialTexturesByType(mat, aiTextureType_AMBIENT_OCCLUSION, "aoMap", false);
	if (ao.empty()) {
		Texture def; def.id = defaultAOTex; def.type = "aoMap"; def.path = "default";
		textures.push_back(def);
	}
	else {
		textures.insert(textures.end(), ao.begin(), ao.end());
	}

	return textures;
}

// Helper to handle caching and actual texture type string assignment
std::vector<Texture> Model::loadMaterialTexturesByType(aiMaterial* mat, aiTextureType type, std::string typeName, bool gamma) {
	std::vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
		aiString str;
		mat->GetTexture(type, i, &str);

		std::cout << "[MODEL] Found texture path: " << str.C_Str() << " for type: " << typeName << std::endl;

		bool skip = false;

		for (unsigned int j = 0; j < textures_loaded.size(); j++) {
			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
				textures.push_back(textures_loaded[j]);
				skip = true;
				break;
			}
		}
		if (!skip) {
			std::cout << "[MODEL] Loading new texture..." << '\n';
			Texture texture;
			texture.id = TextureFromFile(str.C_Str(), this->directory, gamma);
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
		GLenum internalFormat;
		GLenum dataFormat;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		if (nrComponents == 1) {
			internalFormat = GL_RED;
			dataFormat = GL_RED;
		}
		else if (nrComponents == 3) {
			internalFormat = gamma ? GL_SRGB : GL_RGB;
			dataFormat = GL_RGB;
		}
		else if (nrComponents == 4) {
			internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
			dataFormat = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
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