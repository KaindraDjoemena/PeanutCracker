#include "headers/assetManager.h"
#include "headers/model.h"
#include "headers/object.h"
#include "headers/shader.h"

#include <string>
#include <filesystem>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>


AssetManager::AssetManager() = default;

std::shared_ptr<Shader> AssetManager::loadShaderObject(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
    std::string compositeKey = vertPath.string() + "|" + fragPath.string();
    if (shaderCache.find(compositeKey) != shaderCache.end()) {
        return shaderCache[compositeKey].shader;
    }

    auto newShaderObject = std::make_shared<Shader>(vertPath, fragPath);

    auto fullV = std::filesystem::path(SHADER_DIR) / vertPath;
    auto fullF = std::filesystem::path(SHADER_DIR) / fragPath;

    CachedShader cached;
    cached.shader = newShaderObject;
    cached.vertPath = vertPath;
    cached.fragPath = fragPath;
    cached.hasGeom = false;
    cached.vertLastModified = std::filesystem::last_write_time(fullV);
    cached.fragLastModified = std::filesystem::last_write_time(fullF);

    shaderCache[compositeKey] = cached;

    return newShaderObject;
}

std::shared_ptr<Shader> AssetManager::loadShaderObject(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath, const std::filesystem::path& geomPath) {
	auto newShaderObject = std::make_shared<Shader>(vertPath, fragPath, geomPath);

	std::string compositeKey = vertPath.string() + "|" + fragPath.string() + "|" + geomPath.string();
	if (shaderCache.find(compositeKey) != shaderCache.end()) {
		return shaderCache[compositeKey].shader;
	}

	auto fullV = std::filesystem::path(SHADER_DIR) / vertPath;
	auto fullF = std::filesystem::path(SHADER_DIR) / fragPath;
	auto fullG = std::filesystem::path(SHADER_DIR) / geomPath;

	CachedShader cached;
	cached.shader = newShaderObject;
	cached.vertPath = vertPath;
	cached.fragPath = fragPath;
	cached.geomPath = geomPath;
	cached.hasGeom = true;
	cached.vertLastModified = std::filesystem::last_write_time(fullV);
	cached.fragLastModified = std::filesystem::last_write_time(fullF);
	cached.geomLastModified = std::filesystem::last_write_time(fullG);

	shaderCache[compositeKey] = cached;

	return newShaderObject;
}

void AssetManager::reloadShaders() {
    for (auto& [key, cached] : shaderCache) {
        auto fullV = std::filesystem::path(SHADER_DIR) / cached.vertPath;
        auto fullF = std::filesystem::path(SHADER_DIR) / cached.fragPath;

        try {
            auto curVTime = std::filesystem::last_write_time(fullV);
            auto curFTime = std::filesystem::last_write_time(fullF);

            bool changed = (curVTime != cached.vertLastModified || curFTime != cached.fragLastModified);

            std::filesystem::file_time_type curGTime;
            std::filesystem::path fullG = "";

            if (cached.hasGeom) {
                fullG = std::filesystem::path(SHADER_DIR) / cached.geomPath;
                curGTime = std::filesystem::last_write_time(fullG);
                if (curGTime != cached.geomLastModified) changed = true;
            }

            if (changed) {
                cached.shader->reload(fullV, fullF, fullG);

                cached.vertLastModified = curVTime;
                cached.fragLastModified = curFTime;
                if (cached.hasGeom) cached.geomLastModified = curGTime;
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cout << "[AssetManager] Skipping reload, file busy..." << std::endl;
        }
    }
}

std::shared_ptr<Model> AssetManager::loadModel(const std::string& path) {
	auto modelPath = modelCache.find(path);
	if (modelPath != modelCache.end()) {
		return modelPath->second;
	}

	auto newModel = std::make_shared<Model>(path);
	modelCache[path] = newModel;
	return newModel;
}