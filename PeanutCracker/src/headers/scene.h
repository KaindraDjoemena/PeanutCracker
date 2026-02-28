#pragma once

//#include "cubemap.h"
#include "object.h"
#include "light.h"
#include "assetManager.h"
#include "ray.h"
#include "sceneNode.h"
#include "cubemap.h"
#include "camera.h"
#include "refPRobe.h"

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
    const Shader& getSkyboxShader() const { return *m_skyboxShader; }
    const Shader& getConvolutionShader() const { return *m_convolutionShader; }
    const Shader& getConversionShader() const { return *m_conversionShader; }
    const Shader& getPrefilterShader() const { return *m_prefilterShader; }
    const Shader& getModelShader() const { return *m_modelShader; }
    const Shader& getDirDepthShader() const { return *m_dirDepthShader; }
    const Shader& getOmniDepthShader() const { return *m_omniDepthShader; }
    const Shader& getOutlineShader() const { return *m_outlineShader; }
    const Shader& getPickingShader() const { return *m_pickingShader; }
    const Shader& getPrimitiveShader() const { return *m_primitiveShader; }
    const Shader& getPostProcessShader() const { return *m_postProcessShader; }

    const std::vector<std::unique_ptr<DirectionalLight>>& getDirectionalLights() const { return m_directionalLights; }
    const std::vector<std::unique_ptr<PointLight>>& getPointLights() const { return m_pointLights; }
    const std::vector<std::unique_ptr<SpotLight>>& getSpotLights() const { return m_spotLights; }
    
    const std::vector<std::unique_ptr<RefProbe>>& getRefProbes() const { return m_refProbes; }

    const std::vector<SceneNode*>& getSelectedEnts() const { return m_selectedEntities; }


    /* ===== OBJECT PICKING & OPERATIONS ================================================================= */
    SceneNode* Scene::getNodeByPickingID(uint32_t pickingID) const;
    void handleSelectionLogic(SceneNode* node, bool isHoldingShift);
    void deleteSelectedEntities();
    void duplicateSelectedEntities();
    void clearSelection();


    /* ===== OBJECT LOADING QUEUE ================================================================= */
    void queueModelLoad(const std::filesystem::path& path);
    void processLoadQueue();


    /* ===== ADDING ENTITIES ================================================================= */
    void createAndAddObject(const std::string& modelPath);
    void createAndAddDirectionalLight(std::unique_ptr<DirectionalLight> light);
    void createAndAddPointLight(std::unique_ptr<PointLight> light);
    void createAndAddSpotLight(std::unique_ptr<SpotLight> light);
    void createAndAddReflectionProbe(std::unique_ptr<RefProbe> probe);
    void createAndAddSkyboxHDR(const std::filesystem::path& path);


    /* ===== DELETING ENTITIES ================================================================= */
    void deleteDirLight(int index);
    void deletePointLight(int index);
    void deleteSpotLight(int index);
    void deleteRefProbe(int index);
    void deleteSkybox();


    /* ===== UBOs ============================================================================*/
    void setupUBOBindings();                        // Binding point allocation
    void bindToUBOs(const Shader& shader) const;    // Binding shaders to binding points
    
    void updateCameraUBO(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) const;
    void updateLightingUBO() const;
    void updateRefProbeUBO() const;
    void updateShadowUBO() const;
    void updateShadowMapLSMats() const;

    // Bind texture
    void bindDepthMaps() const;
    void bindIBLMaps() const;
    void bindRefProbeMaps() const;

    // Bind texture to shader
    void setNodeShadowMapUniforms() const;
    void setNodeIBLMapUniforms() const;
    void setNodeRefMapUniforms() const;

private:
    // TODO: move inside texture.h
    enum Texture_Slot {
        MAT_TEX_SLOT		  = 10,
        DIR_SHADOW_MAP_SLOT   = 20,
        POINT_SHADOW_MAP_SLOT = 30,
        SPOT_SHADOW_MAP_SLOT  = 40,
        REF_ENV_MAP_SLOT      = 50,
        IRRADIANCE_MAP_SLOT   = 60,
        PREFILTER_MAP_SLOT    = 61,
        BRDF_LUT_SLOT         = 62
    };

    enum Binding_Point {
        CAMERA_BINDING_POINT    = 0,
        LIGHTS_BINDING_POINT    = 1,
        REF_PROBE_BINDING_POINT = 2,
        SHADOW_BINDING_POINT    = 3
    };

    struct alignas(16) DirectionalLightStruct {
        glm::vec4 direction;	// 16
        glm::vec4 color;        // 16
        float     power;        // 4
        float     range;        // 4
        float     normalBias;   // 4
        float     depthBias;    // 4
    };							// 48 Bytes

    struct alignas(16) PointLightStruct {
        glm::vec4 position;   // 16
        glm::vec4 color;      // 16
        float     power;      // 4
        float     radius;     // 4
        float     normalBias; // 4
        float     depthBias;  // 4 
    };						  // 48 Bytes

    struct alignas(16) SpotLightStruct {
        glm::vec4 position;		// 16
        glm::vec4 direction;	// 16
        glm::vec4 color;        // 16
        float     power;        // 4
        float     range;		// 4
        float     inCosCutoff;	// 4
        float     outCosCutoff;	// 4
        float     normalBias;   // 4
        float     depthBias;    // 4
        float     p0;		    // 4
        float     p1;           // 4
    };							// 80 Bytes

    // UBO DATA
    struct alignas(16) LightingUBOData {						// 1424 Bytes
        DirectionalLightStruct directionalLight[MAX_LIGHTS];	// 48 * 8  = 384
        PointLightStruct       pointLight[MAX_LIGHTS];			// 48 * 8  = 384
        SpotLightStruct        spotLight[MAX_LIGHTS];			// 80 * 8  = 640
        int numDirLights;										// 4
        int numPointLights;										// 4
        int numSpotLights;										// 4
        int padding;											// 4
    };

    struct alignas(16) ReflectionProbeUBOData {     // 1296 Bytes
        glm::vec4 position[MAX_LIGHTS];             // 16 * 8 = 128
        glm::mat4 worldMats[MAX_LIGHTS];            // 64 * 8 = 512
        glm::mat4 invWorldMats[MAX_LIGHTS];         // 64 * 8 = 512
        glm::vec4 proxyDims[MAX_LIGHTS];            // 16 * 8 = 128
        int numRefProbes;                           // 4
        int padding0;
        int padding1;
        int padding2;
    };

    struct alignas(16) ShadowMatricesUBOData {					// 1024 Bytes
        glm::mat4 directionalLightSpaceMatrices[MAX_LIGHTS];	// 64 * 8  = 512
        glm::mat4 spotLightSpaceMatrices[MAX_LIGHTS];			// 64 * 8  = 512
    };

    struct alignas(16) CameraMatricesUBOData {	// 144 Bytes
        glm::mat4 projection;		            // 64
        glm::mat4 view;				            // 64
        glm::vec4 cameraPos;		            // 16
    };


    AssetManager* m_assetManager;

    std::unique_ptr<SceneNode> m_worldNode;	// Scene graph parent
    std::unique_ptr<Cubemap>    m_skybox;

    std::vector<std::unique_ptr<DirectionalLight>> m_directionalLights;
    std::vector<std::unique_ptr<PointLight>>	   m_pointLights;
    std::vector<std::unique_ptr<SpotLight>>		   m_spotLights;
    std::vector<std::unique_ptr<RefProbe>>         m_refProbes;
    unsigned int numDirectionalLights = 0;
    unsigned int numPointLights       = 0;
    unsigned int numSpotLights        = 0;
    unsigned int numRefProbes         = 0;

    std::vector<SceneNode*> m_selectedEntities;

    std::vector<std::filesystem::path> loadQueue;

    GLuint m_cameraMatricesUBO = 0;
    GLuint m_lightingUBO       = 0;
    GLuint m_refProbeUBO       = 0;
    GLuint m_shadowUBO         = 0;

    std::shared_ptr<Shader> m_modelShader;
    std::shared_ptr<Shader> m_dirDepthShader;
    std::shared_ptr<Shader> m_omniDepthShader;
    std::shared_ptr<Shader> m_outlineShader;
    std::shared_ptr<Shader> m_pickingShader;
    std::shared_ptr<Shader> m_primitiveShader;
    std::shared_ptr<Shader> m_postProcessShader;

    std::shared_ptr<Shader> m_skyboxShader;
    std::shared_ptr<Shader> m_conversionShader;
    std::shared_ptr<Shader> m_convolutionShader;

    std::shared_ptr<Shader> m_prefilterShader;
    std::shared_ptr<Shader> m_brdfShader;

    Texture m_brdfLUT = Texture(512, 512, GL_RG16F);

    /* ===== UTILITIIES ================================================================= */
    void generateBRDFLUT();
};