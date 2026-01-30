#ifndef VBO_H
#define VBO_H

#include <glad/glad.h>


class VBO {
public:
	VBO() : m_ID(0) {}

	VBO(GLfloat* vertices, GLsizeiptr size, GLenum drawMode) {
		glGenBuffers(1, &m_ID);
		glBindBuffer(GL_ARRAY_BUFFER, m_ID);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, drawMode);
	}

	~VBO() { if (m_ID != 0) glDeleteBuffers(1, &m_ID); }

	VBO(VBO&& other) noexcept : m_ID(other.m_ID) {
		other.m_ID = 0;
	}

	VBO& operator=(VBO&& other) noexcept {
		if (this != &other) {
			if (m_ID != 0) glDeleteBuffers(1, &m_ID);  // Delete old
			m_ID = other.m_ID;                          // Steal new
			other.m_ID = 0;                             // Nullify temporary
		}
		return *this;
	}

	VBO(const VBO&) = delete;
	VBO& operator = (const VBO&) = delete;

	unsigned int getID() { return m_ID; }

	void bind() const {
		glBindBuffer(GL_ARRAY_BUFFER, m_ID);
	}

	void unbind() const {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

private:
	unsigned int m_ID;
};

#endif VBO_H