#ifndef SHADER_H
#define SHADER_H

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


class Shader {
public:
	unsigned int ID;

	Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
		std::cout << "opening shader file at: " << vertexPath << std::endl;
		std::cout << "opening shader file at: " << fragmentPath << std::endl;

		std::string vertexCode = readFile(vertexPath);
		std::string fragmentCode = readFile(fragmentPath);



		//std::cout << "=== VERTEX SHADER SOURCE ===\n";
		//std::cout << vertexCode << "\n";

		//std::cout << "=== FRAGMENT SHADER SOURCE ===\n";
		//std::cout << fragmentCode << "\n";



		if (vertexCode.empty() || fragmentCode.empty()) {
			std::cerr << "ERROR::SHADER::SOURCE_EMPTY" << std::endl;
			return;
		}

		compile(vertexCode.c_str(), fragmentCode.c_str());
	}

	~Shader() {
		if (ID != 0) glDeleteProgram(ID);
	}

	Shader(const Shader&) = delete;
	Shader& operator = (const Shader&) = delete;

	// Using the program
	void use() const { glUseProgram(ID); };

	// Setting the uniform
	void setBool(const std::string& name, bool value) const {
		glUniform1i(getUniformLocation(name), (int)value);
	}
	void setInt(const std::string& name, int value) const {
		glUniform1i(getUniformLocation(name), value);
	};
	void setFloat(const std::string& name, float value) const {
		glUniform1f(getUniformLocation(name), value);
	};
	void setMat4(const std::string& name, const glm::mat4& transformation) const {
		glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(transformation));
	}
	void setVec3(const std::string& name, const glm::vec3& vector) const {
		glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(vector));
	}
	void setVec3(const std::string& name, float a, float b, float c) {
		glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(glm::vec3(a, b, c)));
	}
	void setVec4(const std::string& name, const glm::vec4& vector) const {
		glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(vector));
	}
	void setVec4(const std::string& name, float a, float b, float c, float d) const {
		glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(glm::vec4(a, b, c, d)));
	}

	void deleteObject() const {
		glDeleteProgram(ID);
	}

private:
	mutable std::unordered_map<std::string, int> m_UniformLocationCache;

	std::string readFile(const std::filesystem::path& path) {
		if (!std::filesystem::exists(path)) {
			std::cerr << "ERROR::SHADER::FILE_NOT_FOUND: " << path << std::endl;
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
			std::cerr << "ERROR::SHADER::FILE_NOT_READ: " << path << " (" << e.what() << ")" << std::endl;
		}
		return code;
	}

	int getUniformLocation(const std::string& name) const {
		if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
			return m_UniformLocationCache[name];

		int location = glGetUniformLocation(ID, name.c_str());

		// Cache new results
		m_UniformLocationCache[name] = location;
		return location;

	}

	// SHADER PROGRAM COMPILATION
	void compile(const char* vShaderCode, const char* fShaderCode) {
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

	void checkCompileErrors(unsigned int shaderOrProgram, std::string type) {
		int success;
		char infoLog[1024];
		if (type != "PROGRAM") {
			// Shader compile errors
			glGetShaderiv(shaderOrProgram, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(shaderOrProgram, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else {
			// Program linking errors
			glGetProgramiv(shaderOrProgram, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shaderOrProgram, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}

};

#endif