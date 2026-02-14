#ifndef VAO_H
#define VAO_H

#include <glad/glad.h>
#include "vbo.h"


namespace VertLayout {
    struct AttribData {
        GLuint layout;
        GLuint components;
        GLenum type;
    };

    constexpr AttribData POS     = { 0, 3, GL_FLOAT };  // Position
    constexpr AttribData NORM    = { 1, 3, GL_FLOAT };  // Normals
    constexpr AttribData UV      = { 2, 2, GL_FLOAT };  // UV
    constexpr AttribData TAN     = { 3, 3, GL_FLOAT };  // Tangent
    constexpr AttribData BITAN   = { 4, 3, GL_FLOAT };  // Bitangent
    constexpr AttribData BONE_ID = { 5, 4, GL_INT };    // Bone indices
    constexpr AttribData BONE_W  = { 6, 4, GL_FLOAT };  // Bone weights
}

class VAO {
public:
    VAO() : m_ID(0) 
    {
        glGenVertexArrays(1, &m_ID);
    }
    ~VAO() { if (m_ID != 0) glDeleteVertexArrays(1, &m_ID); }
    VAO(VAO&& other) noexcept : m_ID(other.m_ID) { other.m_ID = 0; }

    VAO& operator=(VAO&& other) noexcept {
        if (this != &other) {
            if (m_ID != 0) glDeleteVertexArrays(1, &m_ID); // Delete old
            m_ID = other.m_ID;                             // Steal new
            other.m_ID = 0;                                // Nullify temporary
        }
        return *this;
    }

    VAO(const VAO&) = delete;
    VAO& operator = (const VAO&) = delete;

    unsigned int getID() const { return m_ID; }


    void linkAttrib(const VBO& VBO, const VertLayout::AttribData& attribData, GLsizeiptr stride, void* offset) const {
        VBO.bind();
        glEnableVertexAttribArray(attribData.layout);
        
        if (attribData.type == GL_INT   || attribData.type == GL_UNSIGNED_INT ||
            attribData.type == GL_SHORT || attribData.type == GL_UNSIGNED_SHORT ||
            attribData.type == GL_BYTE  || attribData.type == GL_UNSIGNED_BYTE)
        {
            glVertexAttribIPointer(attribData.layout, attribData.components, attribData.type, stride, offset);
        }
        else {
            glVertexAttribPointer(attribData.layout, attribData.components, attribData.type, GL_FALSE, stride, offset);
        
        }
        VBO.unbind();
    }

    void bind() const {
        glBindVertexArray(m_ID);
    }

    void unbind() const {
        glBindVertexArray(0);
    }

    void deleteObject() const {
        glDeleteVertexArrays(1, &m_ID);
    }

private:
    GLuint m_ID;
};

#endif