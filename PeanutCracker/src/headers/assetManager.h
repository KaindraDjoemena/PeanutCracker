#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "cubemap.h"
#include "model.h"
#include "object.h"
#include "shader.h"
#include "scene.h"

#include <string>
#include <filesystem>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

class AssetManager {
public:
	AssetManager() = default;

	// Shader cache struct
	struct CachedShader {
		std::shared_ptr<Shader> shader;
		std::filesystem::file_time_type vertLastModified;
		std::filesystem::file_time_type fragLastModified;
	};

	std::shared_ptr<Shader> loadShaderObject(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
		std::string compositeKey = vertPath.string() + "|" + fragPath.string();

		auto cacheIt = shaderCache.find(compositeKey);
		if (cacheIt != shaderCache.end()) {
			auto currentVertTime = std::filesystem::last_write_time(vertPath);
			auto currentFragTime = std::filesystem::last_write_time(fragPath);

			if (currentVertTime == cacheIt->second.vertLastModified &&
				currentFragTime == cacheIt->second.fragLastModified) {
				return cacheIt->second.shader;
			}

			std::cout << "[AssetManager] Reloading modified shader: " << compositeKey << '\n';
			shaderCache.erase(cacheIt);
		}

		// Load new or modified shader
		try {
			auto newShaderObject = std::make_shared<Shader>(vertPath, fragPath);

			CachedShader cached;
			cached.shader = newShaderObject;
			cached.vertLastModified = std::filesystem::last_write_time(vertPath);
			cached.fragLastModified = std::filesystem::last_write_time(fragPath);

			shaderCache[compositeKey] = cached;
			return newShaderObject;
		}
		catch (const std::exception& e) {
			std::cerr << "[AssetManager] Failed to load shader: " << e.what() << '\n';

			if (cacheIt != shaderCache.end()) {
				std::cout << "[AssetManager] Keeping old cached version due to compile error" << '\n';
				return cacheIt->second.shader;
			}
			return nullptr;
		}
	}

	// Force reload shader object
	void reloadShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
		std::string compositeKey = vertPath.string() + "|" + fragPath.string();

		auto cacheIt = shaderCache.find(compositeKey);

		try {
			auto newShader = std::make_shared<Shader>(vertPath, fragPath);

			if (cacheIt != shaderCache.end()) {
				std::cout << "[AssetManager] Hot-reloaded shader: " << compositeKey << '\n';
			}

			CachedShader cached;
			cached.shader = newShader;
			cached.vertLastModified = std::filesystem::last_write_time(vertPath);
			cached.fragLastModified = std::filesystem::last_write_time(fragPath);
			shaderCache[compositeKey] = cached;

		}
		catch (const std::exception& e) {
			std::cerr << "[AssetManager] Hot-reload failed: " << e.what() << '\n';
		}
	}

	// Clear shader cache
	void clearShaderCache() {
		shaderCache.clear();
		std::cout << "[AssetManager] Shader cache cleared" << '\n';
	}

	std::shared_ptr<Model> loadModel(const std::string& path) {
		auto modelPath = modelCache.find(path);
		if (modelPath != modelCache.end()) {
			return modelPath->second;
		}

		auto newModel = std::make_shared<Model>(path);
		modelCache[path] = newModel;
		return newModel;
	}

private:
	std::unordered_map<std::string, std::shared_ptr<Model>> modelCache;
	std::unordered_map<std::string, CachedShader> shaderCache;
};

#endif