#pragma once

#include <glm/glm.hpp>

#include <string>
#include <filesystem>
#include <unordered_map>


class Shader {
public:
	unsigned int ID;

	Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
	Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& geomPath, const std::filesystem::path& fragmentPath);

	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator = (const Shader&) = delete;

	// Using the program
	void use() const;

	// Setting the uniform
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setMat4(const std::string& name, const glm::mat4& transformation) const;
	void setVec3(const std::string& name, const glm::vec3& vector) const;
	void setVec3(const std::string& name, float a, float b, float c) const;
	void setVec4(const std::string& name, const glm::vec4& vector) const;
	void setVec4(const std::string& name, float a, float b, float c, float d) const;

	void deleteObject() const;

private:
	mutable std::unordered_map<std::string, int> m_UniformLocationCache;

	std::string readFile(const std::filesystem::path& path);

	int getUniformLocation(const std::string& name) const;

	// SHADER PROGRAM COMPILATION
	void compile(const char* vShaderCode, const char* fShaderCode);
	void compile(const char* vShaderCode, const char* gShaderCode, const char* fShaderCode);

	void checkCompileErrors(unsigned int shaderOrProgram, std::string type);

};