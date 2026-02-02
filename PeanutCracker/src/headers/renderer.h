#pragma once

#include <glad/glad.h>

#include "scene.h"
#include "camera.h"
#include "sceneNode.h"
#include "cubemap.h"

enum class Render_Mode {
	STANDARD_DIFFUSE,
	WIREFRAME
};

struct Framebuffer {
	unsigned int fbo = 0, texture = 0, rbo = 0;

	unsigned int resolveFbo = 0, resolveTexture = 0;
	int samples = 4;

	int width = 0, height = 0;

	void bind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	}

	void setup(int w, int h) {
		glGenFramebuffers(1, &fbo);
		glGenTextures(1, &texture);
		glGenRenderbuffers(1, &rbo);

		glGenFramebuffers(1, &resolveFbo);
		glGenTextures(1, &resolveTexture);

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

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void resolve() const {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void cleanUp() {
		if (fbo) glDeleteFramebuffers(1, &fbo);
		if (texture) glDeleteTextures(1, &texture);
		if (rbo) glDeleteRenderbuffers(1, &rbo);
		if (resolveFbo) glDeleteFramebuffers(1, &resolveFbo);
		if (resolveTexture) glDeleteTextures(1, &resolveTexture);
		fbo = texture = rbo = resolveFbo = resolveTexture = 0;
	}

	~Framebuffer() { cleanUp(); }
};

class Renderer {
public:
	Renderer(int i_vWidth, int i_vHeight)  { m_viewportFBO.setup(i_vWidth, i_vHeight); }

	void update(Scene& scene, Camera& cam, int vWidth, int vHeight);
	void renderScene(const Scene& scene, const Camera& cam, int vWidth, int vHeight) const;

	Framebuffer* getViewportFBO() { return &m_viewportFBO; }

	void setRenderMode(Render_Mode renderMode) { _renderMode = renderMode; }

private:
	Render_Mode _renderMode     = Render_Mode::STANDARD_DIFFUSE;
	bool        _usingShadowMap = true;

	Framebuffer m_viewportFBO;


	void renderShadowPass(const Scene& scene, const Camera& cam) const;
	void renderLightPass(const Scene& scene, const Camera& cam, int vWidth, int vHeight) const;
	void renderObjects(const Scene& scene, const SceneNode* node, const Camera& cam) const;
	void renderShadowMap(const SceneNode* node, const Shader& depthShader) const;
	void renderSkybox(const Cubemap* skybox) const;

	void renderSelectionHightlight(const Scene& scene) const;
};
