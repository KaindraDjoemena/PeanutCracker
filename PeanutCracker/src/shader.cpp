#include "headers/shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <iostream>


Shader::Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
	std::cout << "[SHADER] opening shader file at: " << vertexPath << '\n';
	std::cout << "[SHADER] opening shader file at: " << fragmentPath << '\n';

	std::string vertexCode = readFile(vertexPath);
	std::string fragmentCode = readFile(fragmentPath);

	if (vertexCode.empty() || fragmentCode.empty()) {
		std::cerr << "ERROR::SHADER::SOURCE_EMPTY" << '\n';
		return;
	}

	compile(vertexCode.c_str(), fragmentCode.c_str());
}

Shader::~Shader() {
	if (ID != 0) glDeleteProgram(ID);
}

// Using the program
void Shader::use() const { glUseProgram(ID); };

// Setting the uniform
void Shader::setBool(const std::string& name, bool value) const {
	glUniform1i(getUniformLocation(name), (int)value);
}
void Shader::setInt(const std::string& name, int value) const {
	glUniform1i(getUniformLocation(name), value);
};
void Shader::setFloat(const std::string& name, float value) const {
	glUniform1f(getUniformLocation(name), value);
};
void Shader::setMat4(const std::string& name, const glm::mat4& transformation) const {
	glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(transformation));
}
void Shader::setVec3(const std::string& name, const glm::vec3& vector) const {
	glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(vector));
}
void Shader::setVec3(const std::string& name, float a, float b, float c) const {
	glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(glm::vec3(a, b, c)));
}
void Shader::setVec4(const std::string& name, const glm::vec4& vector) const {
	glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(vector));
}
void Shader::setVec4(const std::string& name, float a, float b, float c, float d) const {
	glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(glm::vec4(a, b, c, d)));
}

void Shader::deleteObject() const {
	glDeleteProgram(ID);
}


std::string Shader::readFile(const std::filesystem::path& path) {
	if (!std::filesystem::exists(path)) {
		std::cerr << "ERROR::SHADER::FILE_NOT_FOUND: " << path << '\n';
		return "";
	}

	std::string code;
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		file.open(path);
		std::stringstream stream;
		stream << file.rdbuf();
		file.close();
		code = stream.str();
	}
	catch (const std::ifstream::failure& e) {
		std::cerr << "ERROR::SHADER::FILE_NOT_READ: " << path << " (" << e.what() << ")" << '\n';
	}
	return code;
}

int Shader::getUniformLocation(const std::string& name) const {
	if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
		return m_UniformLocationCache[name];

	int location = glGetUniformLocation(ID, name.c_str());

	// Cache new results
	m_UniformLocationCache[name] = location;
	return location;

}

// SHADER PROGRAM COMPILATION
void Shader::compile(const char* vShaderCode, const char* fShaderCode) {
	unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	checkCompileErrors(vertex, "VERTEX");

	unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	checkCompileErrors(fragment, "FRAGMENT");

	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	checkCompileErrors(ID, "PROGRAM");

	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void Shader::checkCompileErrors(unsigned int shaderOrProgram, std::string type) {
	int success;
	char infoLog[1024];
	if (type != "PROGRAM") {
		// Shader compile errors
		glGetShaderiv(shaderOrProgram, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shaderOrProgram, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << '\n';
		}
		else {
			std::cout << "[SHADER] shader compilation successful of type" << std::endl;
		}
	}
	else {
		// Program linking errors
		glGetProgramiv(shaderOrProgram, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shaderOrProgram, 1024, NULL, infoLog);
			std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << '\n';
		}
		else {
			std::cout << "[SHADER] program linking successful" << std::endl;
		}
	}
}
