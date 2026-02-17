#include "headers/texture.h"

#include "../stb_image/stb_image.h"

#include <iostream>


// 1. File loading constructor
Texture::Texture(const std::filesystem::path& i_path, bool sRGB, bool hdr)
    : m_ID(0)
    , m_type(TexType::TEX_2D)
{
    m_ID = hdr ? loadHDR(i_path) : load2D(i_path, sRGB);
}

// 2. Cubemap constructor
Texture::Texture(int size, TexType type, GLenum minFilter, GLenum magFilter)
    : m_ID(0)
    , m_type(type)
{
    if (type != TexType::TEX_CUBE) {
        std::cerr << "[TEX] Error: Use 2D constructor for non-cubemap textures\n";
        return;
    }

    std::cout << "[TEX] Allocating cubemap: " << size << "x" << size << '\n';

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);
}

// 3. Explicit 2D constructor
Texture::Texture(int w, int h,
    GLenum internalFormat,
    bool generateMips,
    GLenum wrapS,
    GLenum wrapT,
    GLenum minFilter,
    GLenum magFilter)
    : m_ID(0)
    , m_type(TexType::TEX_2D)
{
    std::cout << "[TEX] Allocating 2D: " << w << "x" << h << " (fmt: " << internalFormat << ")\n";

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0,
        getBaseFormat(internalFormat), getDataType(internalFormat), nullptr);

    if (generateMips) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

Texture::~Texture() {
    if (m_ID != 0) glDeleteTextures(1, &m_ID);
}


void Texture::generateMipmaps() const {
    glBindTexture(static_cast<GLenum>(m_type), m_ID);
    glGenerateMipmap(static_cast<GLenum>(m_type));
}

void Texture::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(static_cast<GLenum>(m_type), m_ID);
}

void Texture::unbind() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(static_cast<GLenum>(m_type), 0);
}


// File loading helpers
GLuint Texture::load2D(const std::filesystem::path& i_path, bool sRGB) const {
    std::cout << "[TEX] Loading: " << i_path << '\n';

    int w, h, nComps;
    unsigned char* data = stbi_load(i_path.string().c_str(), &w, &h, &nComps, 0);
    if (!data) {
        std::cerr << "[TEX] Failed to load: " << i_path << '\n';
        return 0;
    }

    GLenum internalFormat, dataFormat;
    if (nComps == 1) {
        internalFormat = GL_RED;
        dataFormat = GL_RED;
    }
    else if (nComps == 3) {
        internalFormat = sRGB ? GL_SRGB : GL_RGB;
        dataFormat = GL_RGB;
    }
    else { // 4
        internalFormat = sRGB ? GL_SRGB_ALPHA : GL_RGBA;
        dataFormat = GL_RGBA;
    }

    GLuint ID;
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return ID;
}

GLuint Texture::loadHDR(const std::filesystem::path& i_path) const {
    std::cout << "[TEX] Loading HDR: " << i_path << '\n';

    int w, h, nComps;
    float* data = stbi_loadf(i_path.string().c_str(), &w, &h, &nComps, 0);
    if (!data) {
        std::cerr << "[TEX] Failed to load HDR: " << i_path << '\n';
        return 0;
    }

    GLuint ID;
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return ID;
}


// Format helpers
GLenum Texture::getBaseFormat(GLenum internalFormat) {
    switch (internalFormat) {
    case GL_R8:
    case GL_R16F:
    case GL_R32F:           return GL_RED;
    case GL_RG8:
    case GL_RG16F:
    case GL_RG32F:          return GL_RG;
    case GL_RGB8:
    case GL_RGB16F:
    case GL_RGB32F:
    case GL_SRGB8:          return GL_RGB;
    case GL_RGBA8:
    case GL_RGBA16F:
    case GL_RGBA32F:
    case GL_SRGB8_ALPHA8:   return GL_RGBA;
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT;
    default:
        std::cerr << "[TEX] Unknown internal format: " << internalFormat << '\n';
        return GL_RGBA;
    }
}

GLenum Texture::getDataType(GLenum internalFormat) {
    switch (internalFormat) {
    case GL_R16F:
    case GL_RG16F:
    case GL_RGB16F:
    case GL_RGBA16F:        return GL_HALF_FLOAT;
    case GL_R32F:
    case GL_RG32F:
    case GL_RGB32F:
    case GL_RGBA32F:
    case GL_DEPTH_COMPONENT32F: return GL_FLOAT;
    case GL_DEPTH_COMPONENT16:  return GL_UNSIGNED_SHORT;
    case GL_DEPTH_COMPONENT24:  return GL_UNSIGNED_INT; // or GL_FLOAT, depending
    default:                    return GL_UNSIGNED_BYTE;
    }
}