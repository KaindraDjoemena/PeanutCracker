#ifndef VAO_H
#define VAO_H

#include <glad/glad.h>
#include "vbo.h"


class VAO {
public:
	VAO() { glGenVertexArrays(1, &m_ID); }
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

	void linkAttrib(VBO& VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset) {
		VBO.bind();
		glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
		glEnableVertexAttribArray(layout);
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
	unsigned int m_ID;
};

#endif