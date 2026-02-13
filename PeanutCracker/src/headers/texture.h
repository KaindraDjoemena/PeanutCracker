//#pragma once
//
//#include "glad/glad.h"
//#include "../../dependencies/stb_image/stb_image.h"
//
//#include <iostream>
//#include <filesystem>
//
//
//enum class TEX_TYPE {
//    ALBEDO,
//    NORMAL,
//    METALLIC,
//    ROUGHNESS,
//    AO,
//    ORM
//};
//
//class Texture {
//public:
//    GLuint m_ID   = 0;
//    GLuint m_unit = 0;
//
//    TEX_TYPE m_type;
//    std::filesystem::path m_path;
//
//    Texture(GLuint i_unit, const TEX_TYPE& i_type, const std::filesystem::path& i_path)
//        : m_unit(i_unit)
//        , m_type(i_type)
//        , m_path(i_path)
//    {
//        m_ID = loadTexture();
//    }
//
//    ~Texture() {
//        if (m_ID != 0) glDeleteTextures(1, &ID);
//    }
//
//    Texture(const Texture& other) = default;
//
//    GLuint load2DTex(bool gamma) const {
//        std::cout << "[TEX] loading texture: " << path << '\n';
//
//        GLuint ID = 0;
//        glGenTextures(1, &ID);
//
//        int w, h, nComps;
//        unsigned char* data = stbi_load(m_path.string(), &w, &h, &nComps, 0);
//        if (data) {
//            GLenum internalFormat;
//            GLenum dataFormat;
//
//            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//
//            if (nComps == 1) {
//                internalFormat = GL_RED;
//                dataFormat     = GL_RED;
//            }
//            else if (nComps == 3) {
//                internalFormat = gamma ? GL_SRGB : GL_RGB;
//                dataFormat     = GL_RGB;
//            }
//            else if (nrComps == 4) {
//                internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
//                dataFormat     = GL_RGBA;
//            }
//
//            glBindTexture(GL_TEXTURE_2D, ID);
//            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, dataFormat, GL_UNSIGNED_BYTE, data);
//            glGenerateMipmap(GL_TEXTURE_2D);
//
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//        }
//
//        stbi_image_free(data);
//
//        return ID;
//    }
//
//    GLuint load2DTexHDR(bool gamma) const
//    {
//        std::cout << "[TEX] loading texture: " << path << '\n';
//
//        GLuint ID = 0;
//        glGenTextures(1, &ID);
//
//        int w, h, nComps;
//        unsigned char* data = stbi_loadf(m_path.string(), &w, &h, &nComps, 0);
//        if (data)
//        {
//            glBindTexture(GL_TEXTURE_2D, ID);
//            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, data);
//            glGenerateMipmap(GL_TEXTURE_2D);
//
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//        }
//
//        stbi_image_free(data);
//
//        return ID;
//    }
//
//    void bind() const {
//        glActiveTexture(GL_TEXTURE0 + m_unit);
//        glBindTexture(GL_TEXTURE_2D, m_ID);
//    }
//
//    unsigned int getUnit() const { return m_unit; }
//    unsigned int getID() const { return m_ID; }
//
//private:
//
//};