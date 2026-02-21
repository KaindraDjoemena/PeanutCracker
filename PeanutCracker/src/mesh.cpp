#include "headers/mesh.h"
#include "headers/shader.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include <string>
#include <vector>



Mesh::Mesh(std::vector<Vertex> i_vertices, std::vector<unsigned int> i_indices, std::shared_ptr<Material> i_mat)
    : vertices(i_vertices)
    , indices(i_indices)
    , material(i_mat)
{
    setupMesh();
}

// TODO: REMOVE isShadowPass
void Mesh::draw(const Shader& shader, bool isShadowPass) const {
    material->bind(shader);

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
