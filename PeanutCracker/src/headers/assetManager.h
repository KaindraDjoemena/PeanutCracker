#pragma once

#include "object.h"
#include "shader.h"
#include "texture.h"
#include "material.h"

#include <assimp/material.h>

#include <string>
#include <filesystem>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

class Model;

class AssetManager {
public:
	AssetManager();

	// Shader cache struct
	struct CachedShader {
		std::shared_ptr<Shader> shader;

		std::filesystem::path vertPath;
		std::filesystem::path geomPath;
		std::filesystem::path fragPath;

		bool hasGeom = false;		// TODO: PACK THIS BETTER

		std::filesystem::file_time_type vertLastModified;
		std::filesystem::file_time_type geomLastModified;
		std::filesystem::file_time_type fragLastModified;
	};

	std::shared_ptr<Shader> loadShaderObject(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);
	std::shared_ptr<Shader> loadShaderObject(const std::filesystem::path& vertPath, const std::filesystem::path& geomPath, const std::filesystem::path& fragPath);

	// Force reload shader object
	void reloadShaders();

	std::shared_ptr<Model> loadModel(const std::string& path);
	std::shared_ptr<Texture> loadTexture(const std::filesystem::path& path, bool sRGB, bool hdr);
	std::shared_ptr<Material> loadMaterial(aiMaterial* mat, const std::filesystem::path& dir);

private:
	std::unordered_map<std::string, std::shared_ptr<Model>> modelCache;
	std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
	std::unordered_map<std::string, std::shared_ptr<Material>> materialCache;
	std::unordered_map<std::string, CachedShader> shaderCache;

	std::shared_ptr<Texture> getOrCreateSolidTexture(const glm::vec4& color, bool sRGB);
};