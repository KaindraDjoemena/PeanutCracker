#pragma once

#include <glad/glad.h>
#include <iostream>

class VBO {
public:
	VBO() : m_ID(0)
	{
		glGenBuffers(1, &m_ID);
	}

	~VBO() { if (m_ID != 0) glDeleteBuffers(1, &m_ID); }

	VBO(VBO&& other) noexcept : m_ID(other.m_ID) {
		other.m_ID = 0;
	}

	VBO& operator=(VBO&& other) noexcept {
		if (this != &other) {
			if (m_ID != 0) glDeleteBuffers(1, &m_ID);
			m_ID = other.m_ID;
			other.m_ID = 0;
		}
		return *this;
	}

	VBO(const VBO&) = delete;
	VBO& operator = (const VBO&) = delete;

	GLuint getID() const { return m_ID; }

	template<typename T>
	void setData(const T* data, size_t count, GLenum usage = GL_STATIC_DRAW) {
		bind();
		glBufferData(GL_ARRAY_BUFFER, count * sizeof(T), data, usage);
	}

	void bind() const {
		glBindBuffer(GL_ARRAY_BUFFER, m_ID);
	}

	void unbind() const {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

private:
	GLuint m_ID;
};