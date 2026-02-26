#include "headers/cubemap.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>


const glm::mat4 Cubemap::m_captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

const std::array<glm::mat4, 6> Cubemap::m_captureViews = {
    glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

Cubemap::Cubemap(
    const std::filesystem::path& i_hdrPath,
    const Shader& i_convolutionShader,
    const Shader& i_conversionShader,
    const Shader& i_prefilterShader,
    const Shader& i_brdfShader)
    : m_envCubemap(512, TexType::TEX_CUBE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR)
    , m_irradianceMap(32, TexType::TEX_CUBE, GL_LINEAR, GL_LINEAR)
    , m_prefilterMap(128, TexType::TEX_CUBE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR)
    , m_brdfLUT(512, 512, GL_RG16F)
{
    setupCubeMesh();
    setupQuadMesh();

    glGenFramebuffers(1, &m_captureFBO);
    glGenRenderbuffers(1, &m_captureRBO);

    Texture hdrTexture(i_hdrPath, false, true);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_captureRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Equirect cubemap framebuffer not complete!" << '\n';
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    convertEquirectToCubemap(hdrTexture.getID(), i_conversionShader);
    m_envCubemap.generateMipmaps();

    generateIrradianceMap(i_convolutionShader);

    m_prefilterMap.generateMipmaps();
    generatePrefilterMap(i_prefilterShader);
    generateBRDFLUT(i_brdfShader);
}

Cubemap::~Cubemap() {
    if (m_captureFBO != 0) glDeleteFramebuffers(1, &m_captureFBO);
    if (m_captureRBO != 0) glDeleteRenderbuffers(1, &m_captureRBO);
}


void Cubemap::draw(const Shader& shader) const {
    glDepthFunc(GL_LEQUAL);

    shader.use();
    m_cubeVAO.bind();
    m_envCubemap.bind(0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    m_cubeVAO.unbind();

    glDepthFunc(GL_LESS);
}

// HDR equirect -> cubemap
void Cubemap::convertEquirectToCubemap(GLuint hdrTexID, const Shader& conversionShader) {
    std::cout << "[SKYBOX] Converting equirectangular to cubemap\n";

    conversionShader.use();
    conversionShader.setInt("equirectMap", 0);
    conversionShader.setMat4("projectionMat", m_captureProjection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexID);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);

    for (unsigned int i = 0; i < 6; ++i) {
        conversionShader.setMat4("viewMat", m_captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_envCubemap.getID(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_cubeVAO.bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);
        m_cubeVAO.unbind();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Cubemap::generateIrradianceMap(const Shader& convolutionShader) {
    std::cout << "[SKYBOX] Generating irradiance map\n";

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    convolutionShader.use();
    convolutionShader.setInt("environmentMap", 0);
    convolutionShader.setMat4("projection", m_captureProjection);

    m_envCubemap.bind(100);

    glViewport(0, 0, 32, 32);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // Solve diffuse integral by convolution
    for (unsigned int i = 0; i < 6; ++i) {
        convolutionShader.setMat4("view", m_captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_irradianceMap.getID(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_cubeVAO.bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);
        m_cubeVAO.unbind();
    }
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Cubemap::generatePrefilterMap(const Shader& prefilterShader) {
    std::cout << "[SKYBOX] Generating prefilter map\n";

    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", m_captureProjection);

    m_envCubemap.bind(0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);

    const unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

        glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
        prefilterShader.setFloat("roughness", roughness);

        for (unsigned int i = 0; i < 6; ++i) {
            prefilterShader.setMat4("view", m_captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_prefilterMap.getID(), mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_cubeVAO.bind();
            glDrawArrays(GL_TRIANGLES, 0, 36);
            m_cubeVAO.unbind();
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Cubemap::generateBRDFLUT(const Shader& brdfShader) {
    std::cout << "[SKYBOX] Generating BRDF LUT\n";

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_brdfLUT.getID(), 0);

    glViewport(0, 0, 512, 512);
    brdfShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_quadVAO.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_quadVAO.unbind();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Cubemap::setupCubeMesh() {
    static const std::array<float, 108> vertices = {
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f
    };

    m_cubeVAO.bind();
    m_cubeVBO.setData(vertices.data(), vertices.size() * sizeof(float), GL_STATIC_DRAW);
    m_cubeVAO.linkAttrib(m_cubeVBO, VertLayout::POS, 3 * sizeof(float), (void*)0);
    m_cubeVAO.unbind();
}

void Cubemap::setupQuadMesh() {
    static const std::array<float, 30> quadVertices = {
        // positions            // texCoords
        -1.0f,  1.0f, 0.0f,     0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,     0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,     1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f,     0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,     1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,     1.0f, 1.0f
    };

    m_quadVAO.bind();
    m_quadVBO.setData(quadVertices.data(), quadVertices.size() * sizeof(float), GL_STATIC_DRAW);
    m_quadVAO.linkAttrib(m_quadVBO, VertLayout::POS, 5 * sizeof(float), (void*)0);
    m_quadVAO.linkAttrib(m_quadVBO, VertLayout::UV,  5 * sizeof(float), (void*)(3 * sizeof(float)));
    m_quadVAO.unbind();
}