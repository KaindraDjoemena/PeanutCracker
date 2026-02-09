#pragma once

#include "cubemap.h"
#include "object.h"
#include "light.h"
#include "assetManager.h"
#include "ray.h"
#include "sceneNode.h"
#include "camera.h"

//#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <vector>
#include <filesystem>
#include <algorithm>


const unsigned int MAX_LIGHTS = 8;


class Scene {
public:
	Scene(AssetManager* i_assetManager);
	Scene(const Scene&) = delete;
	//Scene& operator = (const Scene&);


	/* ===== GETTERS =================================================================================== */
	const SceneNode* getWorldNode() const { return m_worldNode.get(); }
	SceneNode* getWorldNode() { return m_worldNode.get(); }
	const Cubemap* getSkybox() const { return m_skybox.get(); }
	const Shader& getDirDepthShader() const { return *m_dirDepthShader; }
	const Shader& getOmniDepthShader() const { return *m_omniDepthShader; }
	const Shader& getOutlineShader() const { return *m_outlineShader; }

	const std::vector<std::unique_ptr<DirectionalLight>>& getDirectionalLights() const { return m_directionalLights; }
	const std::vector<std::unique_ptr<PointLight>>& getPointLights() const { return m_pointLights; }
	const std::vector<std::unique_ptr<SpotLight>>& getSpotLights() const { return m_spotLights; }
	const std::vector<SceneNode*>& getSelectedEnts() const { return m_selectedEntities; }

	/* ===== OBJECT PICKING & OPERATIONS ================================================================= */
	// --PICKING
	void selectEntity(MouseRay& worldRay, bool isHoldingShift);
	// --DELETION
	void deleteSelectedEntities();
	// --DUPLICATION
	void duplicateSelectedEntities();


	/* ===== OBJECT LOADING QUEUE ================================================================= */
	void queueModelLoad(const std::filesystem::path& path);
	void processLoadQueue();


	/* ===== ADDING ENTITIES ================================================================= */
	void createAndAddObject(const std::string& modelPath, const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);
	void createAndAddSkybox(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);
	void createAndAddDirectionalLight(std::unique_ptr<DirectionalLight> light);
	void createAndAddPointLight(std::unique_ptr<PointLight> light);
	void createAndAddSpotLight(std::unique_ptr<SpotLight> light);
	void createAndAddSkyboxHDR(const std::string& path);


	/* ===== DELETING ENTITIES ================================================================= */
	void deleteDirLight(int index);
	void deletePointLight(int index);
	void deleteSpotLight(int index);

	void bindDepthMaps() const;
	void bindIBLMaps() const;


	/* ===== UBOs ============================================================================*/
	// --ALLOCATION
	void setupUBOBindings();
	// --SHADER BINDING
	void bindToUBOs(const Shader* shader) const;
	void setupSkyboxShaderUBOs(const Shader* shader) const;

	/* ===== UPDATING UBOs ================================================================= */
	// BINDING NODE SHADERS TO UBOs
	void setupNodeUBOs(SceneNode* node);
	// CAMERA UBO
	void updateCameraUBO(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) const;
	// LIGHTING UBO
	void updateLightingUBO() const;
	// SHADOW UBO
	void updateShadowUBO() const;

	// LOAD EVERY SHADOW MAP TO OBJECT SHADERS
	void setNodeShadowMapUniforms(const SceneNode* node) const;
	void setNodeIBLMapUniforms(const SceneNode* node) const;
	void updateShadowMapLSMats() const;

	/* ===== RENDERING ================================================================================== */
	// --For objects
	void renderRecursive(const Camera& camera, SceneNode* node) const;
	// --For the shadow map
	void renderShadowRecursive(const SceneNode* node, const Shader& depthShader) const;

	void init();

private:
	AssetManager* m_assetManager;

	// BINDING POINT ENUM
	enum Binding_Point {
		CAMERA_BINDING_POINT,	// 0
		LIGHTS_BINDING_POINT,	// 1
		SHADOW_BINDING_POINT	// 2
	};

	// UBO DATA
	struct DirectionalLightStruct {
		glm::vec4 direction;	// 16
		glm::vec4 ambient;		// 16
		glm::vec4 diffuse;		// 16
		glm::vec4 specular;		// 16
	};							// 64 Bytes

	struct PointLightStruct {
		glm::vec4 position;		// 16
		glm::vec4 ambient;		// 16
		glm::vec4 diffuse;		// 16
		glm::vec4 specular;		// 16
		float constant;			// 4
		float linear;			// 4
		float quadratic;		// 4
		float _padding;			// 4
	};							// 80 Bytes

	struct SpotLightStruct {
		glm::vec4 position;		// 16
		glm::vec4 direction;	// 16
		glm::vec4 ambient;		// 16
		glm::vec4 diffuse;		// 16
		glm::vec4 specular;		// 16
		float constant;			// 4
		float linear;			// 4
		float quadratic;		// 4
		float inCosCutoff;		// 4
		float outCosCutoff;		// 4
		float _padding0;		// 4
		float _padding1;		// 4
		float _padding2;		// 4
	};							// 112 Bytes

	// UBO DATA
	struct LightingUBOData {									// 2064 Bytes
		DirectionalLightStruct directionalLight[MAX_LIGHTS];	// 64 * 8  = 512
		PointLightStruct       pointLight[MAX_LIGHTS];			// 80 * 8  = 640
		SpotLightStruct        spotLight[MAX_LIGHTS];			// 112 * 8 = 896
		int numDirLights;										// 4
		int numPointLights;										// 4
		int numSpotLights;										// 4
		int padding;											// 4
	};

	struct ShadowMatricesUBOData {								// 1024 Bytes
		glm::mat4 directionalLightSpaceMatrices[MAX_LIGHTS];	// 64 * 8  = 512
		glm::mat4 spotLightSpaceMatrices[MAX_LIGHTS];			// 64 * 8  = 512
	};

	struct CameraMatricesUBOData {	// 144 Bytes
		glm::mat4 projection;		// 64
		glm::mat4 view;				// 64
		glm::vec4 cameraPos;		// 16
	};


	std::unique_ptr<SceneNode> m_worldNode;	// Scene graph parent
	std::unique_ptr<Cubemap>   m_skybox;

	std::vector<std::unique_ptr<DirectionalLight>> m_directionalLights;
	std::vector<std::unique_ptr<PointLight>>	   m_pointLights;
	std::vector<std::unique_ptr<SpotLight>>		   m_spotLights;
	std::vector<SceneNode*>						   m_selectedEntities;

	std::vector<std::filesystem::path> loadQueue;

	unsigned int numDirectionalLights = 0;
	unsigned int numPointLights       = 0;
	unsigned int numSpotLights        = 0;

	unsigned int cameraMatricesUBO = 0;
	unsigned int lightingUBO       = 0;
	unsigned int shadowUBO         = 0;

	std::shared_ptr<Shader> m_dirDepthShader;
	std::shared_ptr<Shader> m_omniDepthShader;
	std::shared_ptr<Shader> m_outlineShader;

	VAO m_debugVAO;
	VBO m_debugVBO;
	std::shared_ptr<Shader> m_debugShader;

	VAO m_lightFrustumVAO;
	VBO m_lightFrustumVBO;
	bool drawLightFrustums = true;
	std::shared_ptr<Shader> m_frustumShader;

	/* ===== PICKING OPERATIONS ================================================================= */
	void findBestNodeRecursive(SceneNode* node, MouseRay& worldRay, float& shortestDist, SceneNode*& bestNode);
	void handleSelectionLogic(SceneNode* node, bool isHoldingShift);
	void clearSelection();
	void debugPrintSceneGraph(SceneNode* node, int depth = 0);


	/* ===== LIGHT FRUSTUM DRAWING ================================================================= */
	void initLightFrustumDebug();
	void drawDirectionalLightFrustums(const glm::mat4& projMat, const glm::mat4& viewMat, const glm::vec4& color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), float lineWidth = 2.0f) const;
	//void drawPointLightFrustums();
	void drawSpotLightFrustums(const glm::mat4& projMat, const glm::mat4& viewMat, const glm::vec4& color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f), float lineWidth = 2.0f) const;

	/* ===== SELECTION DRAWING ================================================================= */
	void drawSelectionStencil() const;

	/* ===== AABB BOX DRAWING ================================================================= */
	void initDebugAABBDrawing();
	void drawDebugAABBs(SceneNode* node) const;
};