#include "headers/mesh.h"
#include "headers/shader.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include <string>
#include <vector>



Mesh::Mesh(std::vector<Vertex> i_vertices, std::vector<unsigned int> i_indices, std::vector<MaterialTexture> i_textures)
    : vertices(i_vertices)
    , indices(i_indices)
    , textures(i_textures)
{
    setupMesh();
}

void Mesh::draw(const Shader& shader, bool isShadowPass) const {
    // Texture Binding

    const int OBJECT_TEX_SLOT = 10;
    for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + OBJECT_TEX_SLOT + i);

        std::string name = textures[i].type;
        shader.setInt(("material." + name), OBJECT_TEX_SLOT + i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // draw mesh
    m_VAO.bind();
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    m_VAO.unbind();

    // setting the default texture back
    glActiveTexture(GL_TEXTURE0);

}

void Mesh::setupMesh() {
    m_VAO.bind();
    m_VBO.setData(vertices.data(), vertices.size());
    m_EBO.setData(indices.data(),  indices.size());

    m_VAO.linkAttrib(m_VBO, VertLayout::POS,     sizeof(Vertex), (void*)0);
    m_VAO.linkAttrib(m_VBO, VertLayout::NORM,    sizeof(Vertex), (void*)offsetof(Vertex, normal));
    m_VAO.linkAttrib(m_VBO, VertLayout::UV,      sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    m_VAO.linkAttrib(m_VBO, VertLayout::TAN,     sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    m_VAO.linkAttrib(m_VBO, VertLayout::BITAN,   sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
    m_VAO.linkAttrib(m_VBO, VertLayout::BONE_ID, sizeof(Vertex), (void*)offsetof(Vertex, m_boneIDs));
    m_VAO.linkAttrib(m_VBO, VertLayout::BONE_W,  sizeof(Vertex), (void*)offsetof(Vertex, m_weights));

    m_VAO.unbind();
}
