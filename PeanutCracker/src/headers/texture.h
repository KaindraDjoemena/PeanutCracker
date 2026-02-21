#pragma once

#include "glad/glad.h"
#include "glm/glm.hpp"

#include <filesystem>


namespace TexSlot {
    constexpr unsigned int MAT_TEX = 10;
    constexpr unsigned int DIR_SHAD_MAP = 20;
    constexpr unsigned int POINT_SHAD_MAP = 30;
    constexpr unsigned int SPOT_SHAD_MAP = 40;
    constexpr unsigned int IRRADIANCE_MAP = 50;
    constexpr unsigned int PREFILTER_MAP = 60;
    constexpr unsigned int BRDF_LUT = 70;
}

namespace MatTex {
    constexpr unsigned int ALBEDO = 0;
    constexpr unsigned int NORM = 1;
    constexpr unsigned int METALLIC = 2;
    constexpr unsigned int ROUGHNESS = 3;
    constexpr unsigned int AO = 4;
    constexpr unsigned int ORM = 5;
}

enum class TexType {
    TEX_2D   = GL_TEXTURE_2D,
    TEX_CUBE = GL_TEXTURE_CUBE_MAP
};


class Texture {
public:
    Texture();

    // Load 2D texture
    Texture(const std::filesystem::path& i_path, bool sRGB = false, bool hdr = false);
    
    // Make 1x1 colored texture
    Texture(const glm::vec4& color, bool sRGB);

    // Empty cubemap texture
    Texture(int size, TexType type, GLenum minFilter, GLenum magFilter);

    // Empty 2D texture
    Texture(int w, int h,
        GLenum internalFormat,
        bool generateMips = false,
        GLenum wrapS = GL_CLAMP_TO_EDGE,
        GLenum wrapT = GL_CLAMP_TO_EDGE,
        GLenum minFilter = GL_LINEAR,
        GLenum magFilter = GL_LINEAR);

    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept
        : m_ID(other.m_ID), m_type(other.m_type) {
        other.m_ID = 0;  // Prevent deletion
    }

    Texture& operator=(Texture&& other) noexcept {
        if (this != &other) {
            if (m_ID) glDeleteTextures(1, &m_ID);
            m_ID = other.m_ID;
            m_type = other.m_type;
            other.m_ID = 0;
        }
        return *this;
    }

    GLuint getID() const { return m_ID; }
    TexType getType() const { return m_type; }

    void generateMipmaps() const;

    void bind(unsigned int slot) const;
    void unbind() const;

private:
    GLuint m_ID;
    TexType m_type;

    // Helpers for file loading
    GLuint load2D(const std::filesystem::path& i_path, bool sRGB) const;
    GLuint loadHDR(const std::filesystem::path& i_path) const;

    // Helpers for format inference
    static GLenum getBaseFormat(GLenum internalFormat);
    static GLenum getDataType(GLenum internalFormat);
};