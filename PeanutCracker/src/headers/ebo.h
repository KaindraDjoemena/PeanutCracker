#ifndef EBO_H
#define EBO_H

#include <glad/glad.h>


class EBO {
public:
	EBO(unsigned int* indices, GLsizeiptr size) {
		glGenBuffers(1, &m_ID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	}

	void bind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
	}

	void unbind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void deleteObject() const {
		glDeleteBuffers(1, &m_ID);
	}

private:
	unsigned int m_ID;
};

#endif EBO_H