#pragma once

#include <glad/glad.h>

#include "scene.h"
#include "camera.h"
#include "sceneNode.h"
#include "skybox.h"

enum class Render_Mode {
    PBR,
    WIREFRAME
};

struct Framebuffer {
    unsigned int fbo = 0, texture = 0, rbo = 0;

    unsigned int resolveFbo = 0, resolveTexture = 0;
    unsigned int screenFbo = 0, screenTexture = 0;
    int samples = 16;

    int width = 0, height = 0;

    void bind(int w, int h) const {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void setup(int w, int h) {
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &texture);
        glGenRenderbuffers(1, &rbo);

        glGenFramebuffers(1, &resolveFbo);
        glGenTextures(1, &resolveTexture);

        glGenFramebuffers(1, &screenFbo);
        glGenTextures(1, &screenTexture);

        rescale(w, h);
    }

    void rescale(int w, int h) {
        if (w <= 0 || h <= 0) return;
        if (w == width && h == height) return;
        width = w;
        height = h;

        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA16F, width, height, GL_TRUE);

        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "ERROR: Multisampled FBO incomplete\n";

        glBindTexture(GL_TEXTURE_2D, resolveTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "ERROR: Resolve FBO incomplete\n";

        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, screenFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void resolve() const {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    void cleanUp() {
        if (fbo) glDeleteFramebuffers(1, &fbo);
        if (texture) glDeleteTextures(1, &texture);
        if (rbo) glDeleteRenderbuffers(1, &rbo);
        if (resolveFbo) glDeleteFramebuffers(1, &resolveFbo);
        if (resolveTexture) glDeleteTextures(1, &resolveTexture);
        if (screenFbo) glDeleteFramebuffers(1, &screenFbo);
        if (screenTexture) glDeleteTextures(1, &screenTexture);
        fbo = texture = rbo = resolveFbo = resolveTexture = 0;
    }

    ~Framebuffer() { cleanUp(); }
};

class Renderer {
public:
    Renderer(int i_vWidth, int i_vHeight)  { m_viewportFBO.setup(i_vWidth, i_vHeight); }

    void initScene(Scene& scene);
    void update(Scene& scene, Camera& cam, int vWidth, int vHeight);
    void renderScene(const Scene& scene, const Camera& cam, int vWidth, int vHeight) const;

    Framebuffer* getViewportFBO() { return &m_viewportFBO; }

    glm::vec4 getBgCol() const { return m_winBgCol; }
    float getEV100() const { return m_EV100; }

    void setRenderMode(Render_Mode renderMode) { _renderMode = renderMode; }
    void setShadowMode(bool usingShadowMap)    { _usingShadowMap = usingShadowMap; }
    void setBgCol(const glm::vec4& bgCol) { m_winBgCol = bgCol; }
    void setEV100(float i_EV100) { m_EV100 = i_EV100; }

private:
    enum class Primitive_Mode {
        LINE = 0,
        SDF  = 1
    };

    Render_Mode _renderMode     = Render_Mode::PBR;
    bool        _usingShadowMap = true;
    
    glm::vec4  m_winBgCol = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    Framebuffer m_viewportFBO;

    // Post-processing
    VAO m_lineVAO;
    VBO m_lineVBO;

    VAO m_quadVAO;
    VBO m_quadVBO;

    VAO m_coneVAO;
    VBO m_coneVBO;
    int m_coneVertexCount = 0;

    float m_EV100 = 0.0f;
    
    void renderPostProcess(const Scene& scene, int vWidth, int vHeight) const;

    void renderShadowPass(const Scene& scene, const Camera& cam) const;
    void renderLightPass(const Scene& scene, const Camera& cam, int vWidth, int vHeight) const;
    void renderObjects(const Scene& scene, const SceneNode* node, const Camera& cam) const;
    void renderShadowMap(const SceneNode* node, const Shader& depthShader) const;
    void renderSkybox(const Scene& scena) const;

    void renderSelectionHightlight(const Scene& scene) const;
    void renderLightAreas(const Scene& scene, const Camera& cam, int vWidth, int vHeight) const;

    // TODO: inside a rendering utils class? (the renderer class just keeps track of the vao)
    void setupUnitLine();
    void setupUnitQuad();
    void setupUnitCone();

    // TODO: put inside a utils file for these operations (e.g. extracting 3x3 from 4x4 mat)
    inline glm::mat4 calcBillboardMat(const glm::vec3& position, const glm::mat4& viewMat) const {
        glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), position);

        modelMat[0][0] = viewMat[0][0]; modelMat[0][1] = viewMat[1][0]; modelMat[0][2] = viewMat[2][0];
        modelMat[1][0] = viewMat[0][1]; modelMat[1][1] = viewMat[1][1]; modelMat[1][2] = viewMat[2][1];
        modelMat[2][0] = viewMat[0][2]; modelMat[2][1] = viewMat[1][2]; modelMat[2][2] = viewMat[2][2];

        return modelMat;
    }

    inline glm::mat4 calcLookAtMat(const glm::vec3& position, const glm::vec3& target, const glm::vec3& tempUp = glm::vec3(0.0f, 1.0f, 0.0f)) const {
        glm::vec3 upVec = std::abs(glm::normalize(target - position).y) < 0.99f ?
            tempUp : glm::vec3(1.0f, 0.0f, 0.0f);

        return glm::inverse(glm::lookAt(position, target, upVec));
    }

};
