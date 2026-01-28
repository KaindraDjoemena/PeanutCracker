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


class AssetManager {
public:
	AssetManager() = default;

	//std::shared_ptr<Cubemap> loadCubeMap(const Faces& faces, std::shared_ptr<Shader> shaderObjectPtr) {
	//	// 1. Create a robust composite key string by concatenating all six paths IN ORDER
	//	std::string compositeKey =
	//		faces.cubeFaces[0].string() + "|" + // +X
	//		faces.cubeFaces[1].string() + "|" + // -X
	//		faces.cubeFaces[2].string() + "|" + // +Y
	//		faces.cubeFaces[3].string() + "|" + // -Y
	//		faces.cubeFaces[4].string() + "|" + // +Z
	//		faces.cubeFaces[5].string();        // -Z

	//	// 2. Calculate the hash value (the actual map key)
	//	size_t hashKey = std::hash<std::string>{}(compositeKey);

	//	// 3. Check the cache
	//	auto it = cubemapCache.find(hashKey);
	//	if (it != cubemapCache.end()) {
	//		return it->second; // Return cached shared_ptr
	//	}

	//	// 4. If not found, create the new Cubemap object.
	//	// Assuming your Cubemap constructor takes the Faces struct directly, as shown in your Scene code.
	//	auto newCubeMap = std::make_shared<Cubemap>(faces, shaderObjectPtr.get());

	//	// 5. Store and return
	//	cubemapCache[hashKey] = newCubeMap;
	//	return newCubeMap;
	//}

	std::shared_ptr<Shader> loadShaderObject(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
		std::string strVertPath  = vertPath.string();
		std::string strFragPath  = fragPath.string();
		std::string compositeKey = strVertPath + "|" + strFragPath;		// the "|" delimiter is not that good. Hash functions are better i think(?)

		auto shaderPath = shaderCache.find(compositeKey);
		if (shaderPath != shaderCache.end()) {
			return shaderPath->second;
		}

		auto newShaderObject = std::make_shared<Shader>(vertPath, fragPath);

		shaderCache[compositeKey] = newShaderObject;
		return newShaderObject;
	}

	std::shared_ptr<Model> loadModel(const std::string& path) {
		auto modelPath = modelCache.find(path);
		if (modelPath != modelCache.end()) {
			return modelPath->second;
		}

		auto newModel = std::make_shared<Model>(path);	// Load model

		modelCache[path] = newModel;
		return newModel;
	}

private:
	std::unordered_map<std::string, std::shared_ptr<Model>>	 modelCache;
	std::unordered_map<std::string, std::shared_ptr<Shader>> shaderCache;
};

#endif