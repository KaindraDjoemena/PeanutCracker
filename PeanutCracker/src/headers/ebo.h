#pragma once

#include <glad/glad.h>


class EBO {
public:
	EBO() : m_ID(0)
	{
		glGenBuffers(1, &m_ID);
	}

	~EBO() { if (m_ID != 0) glDeleteBuffers(1, &m_ID); }

	EBO(EBO&& other) noexcept : m_ID(other.m_ID) {
		other.m_ID = 0;
	}

	EBO& operator=(EBO&& other) noexcept {
		if (this != &other) {
			if (m_ID != 0) glDeleteBuffers(1, &m_ID);
			m_ID = other.m_ID;
			other.m_ID = 0;
		}
		return *this;
	}

	EBO(const EBO&) = delete;
	EBO& operator = (const EBO&) = delete;

	GLuint getID() const { return m_ID; }

	template<typename T>
	void setData(const T* data, size_t count, GLenum usage = GL_STATIC_DRAW) {
		bind();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(T), data, usage);
	}

	void bind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
	}

	void unbind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

private:
	GLuint m_ID;
};