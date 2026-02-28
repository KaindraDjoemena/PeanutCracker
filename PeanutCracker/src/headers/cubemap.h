#pragma once

#include <filesystem>
#include <array>

#include "texture.h"
#include "vao.h"
#include "vbo.h"
#include "shader.h"


class Cubemap {
public:
    Cubemap(
        const std::filesystem::path& i_hdrPath,
        const Shader& i_convolutionShader,
        const Shader& i_conversionShader,
        const Shader& i_prefilterShader
    );

    Cubemap(
        const Shader& i_convolutionShader,
        const Shader& i_conversionShader,
        const Shader& i_prefilterShader
    );

    ~Cubemap();

    //Skybox(const Skybox&) = delete;
    //Skybox& operator=(const Skybox&) = delete;
    //Skybox(Skybox&&) = default;
    //Skybox& operator=(Skybox&&) = default;

    const Texture& getEnvironmentMap() const { return m_envCubemap; }
    const Texture& getIrradianceMap() const { return m_irradianceMap; }
    const Texture& getPrefilterMap() const { return m_prefilterMap; }
    
    void draw(const Shader& shader) const;

    void generatePrefilterMap(const Shader& prefilterShader) const;

private:
    Texture m_envCubemap;     // Environment cubemap
    Texture m_irradianceMap;  // Diffuse irradiance
    Texture m_prefilterMap;   // Specular prefilter map
    Texture m_brdfLUT;        // BRDF integration texture

    VAO m_cubeVAO;
    VBO m_cubeVBO;
    VAO m_quadVAO;
    VBO m_quadVBO;

    GLuint m_captureFBO = 0;
    GLuint m_captureRBO = 0;

    static const glm::mat4 m_captureProjection;
    static const std::array<glm::mat4, 6> m_captureViews;


    void convertEquirectToCubemap(GLuint hdrTexID, const Shader& conversionShader);

    void generateIrradianceMap(const Shader& convolutionShader);


    void setupCubeMesh();
    
    void setupQuadMesh();
};