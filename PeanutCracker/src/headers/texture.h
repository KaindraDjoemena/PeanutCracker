#ifndef TEXTURE_H
#define TEXTURE_H

#include "glad/glad.h"
#include "../../dependencies/stb_image/stb_image.h"

#include <iostream>


class Texture {
public:
    Texture(char const* path,
            unsigned int textureUnit
    ) : m_unit(textureUnit) {
        loadTexture(path);
    }

    Texture(const Texture& other) = default;

    void bind() const {
        glActiveTexture(GL_TEXTURE0 + m_unit);
        glBindTexture(GL_TEXTURE_2D, m_ID);
    }

    unsigned int getUnit() const { return m_unit; }
    unsigned int getID() const { return m_ID; }

private:
    unsigned int m_ID;
    unsigned int m_unit;

    void loadTexture(char const* path) {
        std::cout << "[TEXTURE] loading texture: " << path << '\n';

        glGenTextures(1, &m_ID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format = 0;
            if (nrComponents == 1) { format = GL_RED; }
            else if (nrComponents == 3) { format = GL_RGB; }
            else if (nrComponents == 4) { format = GL_RGBA; }

            glBindTexture(GL_TEXTURE_2D, m_ID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else {
            std::cout << "[TETURE] Failed to load texture: " << path << '\n';
        }
        stbi_image_free(data);
    }
};

#endif