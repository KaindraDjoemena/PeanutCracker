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

// TODO: CHANGE TO FILESYSTEM::PATH
std::shared_ptr<Model> AssetManager::loadModel(const std::string& path) {
	auto modelPath = modelCache.find(path);
	if (modelPath != modelCache.end()) {
		return modelPath->second;
	}

	auto newModel = std::make_shared<Model>(this, path);
	modelCache[path] = newModel;
	return newModel;
}

std::shared_ptr<Texture> AssetManager::loadTexture(const std::filesystem::path& path, bool sRGB, bool hdr) {
    std::string key = path.string();

    if (textureCache.find(key) != textureCache.end()) {
        return textureCache[key];
    }

    auto newTexture = std::make_shared<Texture>(path, sRGB, hdr);
    textureCache[key] = newTexture;
    return newTexture;
}

std::shared_ptr<Material> AssetManager::loadMaterial(aiMaterial* mat, const std::filesystem::path& dir, int matIndex) {
    std::string key = dir.string() + "_index_" + std::to_string(matIndex);

    if (materialCache.find(key) != materialCache.end()) {
        return materialCache[key];
    }

    aiString aiName;
    mat->Get(AI_MATKEY_NAME, aiName);

    auto material = std::make_shared<Material>();
    material->name = aiName.C_Str();

    // Helper func
    auto getTex = [&](aiTextureType type, bool sRGB) -> std::shared_ptr<Texture> {
        if (mat->GetTextureCount(type) > 0) {
            aiString path;
            mat->GetTexture(type, 0, &path);
            return loadTexture(dir / path.C_Str(), sRGB, false);
        }
        
        return nullptr;
    };

    //--Albedo
    material->albedoMap = getTex(aiTextureType_DIFFUSE, true);
    if (!material->albedoMap) {
        material->albedoMap = getTex(aiTextureType_BASE_COLOR, true);
    }

    if (!material->albedoMap) {
        aiColor4D defaultAlbedo = { 1.0f, 0.0f, 1.0f, 1.0f };
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, defaultAlbedo);
        material->albedoMap = getOrCreateSolidTexture(
            glm::vec4(defaultAlbedo.r, defaultAlbedo.g, defaultAlbedo.b, defaultAlbedo.a),
            true);
    }

    //--Normals
    material->normalMap = getTex(aiTextureType_NORMALS, false);
    if (!material->normalMap) {
        material->normalMap = getOrCreateSolidTexture(
            glm::vec4(0.5f, 0.5f, 1.0f, 1.0f),
            false);
    }

    //--ORM check
    auto metalTex   = getTex(aiTextureType_METALNESS, false);
    auto roughTex   = getTex(aiTextureType_DIFFUSE_ROUGHNESS, false);
    auto unknownTex = getTex(aiTextureType_UNKNOWN, false);

    if (unknownTex) {
        // ORM
        material->aoMap = unknownTex;
        material->roughnessMap = unknownTex;
        material->metallicMap = unknownTex;
    }
    else if (metalTex || roughTex) {
        // Seperate
        material->metallicMap  = metalTex ? metalTex : getOrCreateSolidTexture(glm::vec4(0, 0, 0, 1), false);
        material->roughnessMap = roughTex ? roughTex : getOrCreateSolidTexture(glm::vec4(0, 0.5f, 0, 1), false);
        material->aoMap        = getOrCreateSolidTexture(glm::vec4(1, 0, 0, 1), false);
    }
    else {
        float metalFactor = 0.0f;
        float roughFactor = 0.5f;
        mat->Get(AI_MATKEY_METALLIC_FACTOR, metalFactor);
        mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughFactor);

        material->metallicMap  = getOrCreateSolidTexture(glm::vec4(0, 0, metalFactor, 1), false);
        material->roughnessMap = getOrCreateSolidTexture(glm::vec4(0, roughFactor, 0, 1), false);
        material->aoMap        = getOrCreateSolidTexture(glm::vec4(1, 0, 0, 1), false);
    }

    materialCache[key] = material;
    return material;
}

std::shared_ptr<Texture> AssetManager::getOrCreateSolidTexture(const glm::vec4& color, bool sRGB) {
    std::string key = "solid_" + std::to_string(color.r) + "_" + std::to_string(color.g) +
        "_" + std::to_string(color.b) + "_" + std::to_string(color.a) + (sRGB ? "_srgb" : "_lin");

    if (textureCache.count(key)) return textureCache[key];

    auto tex = std::make_shared<Texture>(color, sRGB);  // create 1x1 colored texture
    textureCache[key] = tex;
    return tex;
}