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
    std::filesystem::path vPath = std::filesystem::path(SHADER_DIR) / vertexPath;
    std::filesystem::path fPath = std::filesystem::path(SHADER_DIR) / fragmentPath;

    std::cout << "[SHADER] opening shader file at: " << vPath.string() << '\n';
    std::cout << "[SHADER] opening shader file at: " << fPath.string() << '\n';

    std::string vertexCode   = readFile(vPath);
    std::string fragmentCode = readFile(fPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "ERROR::SHADER::SOURCE_EMPTY" << '\n';
        throw std::runtime_error("Empty shader source file detected");
    }

    ID = compile(vertexCode.c_str(), fragmentCode.c_str());
}
Shader::Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath, const std::filesystem::path& geometryPath) {
    std::filesystem::path vPath = std::filesystem::path(SHADER_DIR) / vertexPath;
    std::filesystem::path fPath = std::filesystem::path(SHADER_DIR) / fragmentPath;
    std::filesystem::path gPath = std::filesystem::path(SHADER_DIR) / geometryPath;
    
    std::cout << "[SHADER] opening shader file at: " << vPath << '\n';
    std::cout << "[SHADER] opening shader file at: " << fPath << '\n';
    std::cout << "[SHADER] opening shader file at: " << gPath << '\n';

    std::string vertexCode   = readFile(vPath);
    std::string fragmentCode = readFile(fPath);
    std::string geometryCode = readFile(gPath);

    if (vertexCode.empty() || geometryCode.empty() || fragmentCode.empty()) {
        std::cerr << "ERROR::SHADER::SOURCE_EMPTY" << '\n';
        throw std::runtime_error("Empty shader source file detected");
    }

    ID = compile(vertexCode.c_str(), fragmentCode.c_str(), geometryCode.c_str());
}

Shader::~Shader() {
    if (ID != 0) glDeleteProgram(ID);
}

void Shader::reload(const std::filesystem::path& vPath, const std::filesystem::path& fPath, const std::filesystem::path& gPath) {
    std::string vCode = readFile(vPath);
    std::string fCode = readFile(fPath);
    std::string gCode = (!gPath.empty()) ? readFile(gPath) : "";

    if (vCode.empty() || fCode.empty()) {
        std::cerr << "[SHADER] Reload failed: Could not read source files.\n";
        return;
    }
    unsigned int newID = compile(vCode.c_str(), fCode.c_str(), gCode.empty() ? nullptr : gCode.c_str());

    if (newID != 0) {
        if (ID != 0) glDeleteProgram(ID);
        ID = newID;
        m_UniformLocationCache.clear();
        std::cout << "[SHADER] Live-reload successful for program: " << ID << "\n";
    }
}


// Using the program
void Shader::use() const { glUseProgram(ID); };

// Setting the uniform
void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), (int)value);
}
void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}
void Shader::setUint(const std::string& name, unsigned int value) const {
    glUniform1ui(getUniformLocation(name), value);
}
void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}
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
    if (location == -1) {
        std::cerr << "[SHADER] Warning: uniform '" << name << "' not found! in " << ID << '\n';
    }

    // Cache new results
    m_UniformLocationCache[name] = location;
    return location;

}

// SHADER PROGRAM COMPILATION
unsigned int Shader::compile(const char* vShaderCode, const char* fShaderCode, const char* gShaderCode) {
    GLuint vertID    = 0;
    GLuint geomID    = 0;
    GLuint fragID	 = 0;
    GLuint programID = 0;
    
    vertID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertID, 1, &vShaderCode, NULL);
    glCompileShader(vertID);
    checkCompileErrors(vertID, "VERTEX");

    fragID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragID, 1, &fShaderCode, NULL);
    glCompileShader(fragID);
    checkCompileErrors(fragID, "FRAGMENT");

    if (gShaderCode != nullptr) {
        geomID = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geomID, 1, &gShaderCode, NULL);
        glCompileShader(geomID);
        checkCompileErrors(geomID, "GEOMETRY");
    }

    programID = glCreateProgram();
    glAttachShader(programID, vertID);
    glAttachShader(programID, fragID);
    if (gShaderCode != nullptr) glAttachShader(programID, geomID);

    glLinkProgram(programID);
    unsigned int linkCode = checkCompileErrors(programID, "PROGRAM");

    glDeleteShader(vertID);
    glDeleteShader(fragID);
    if (gShaderCode != nullptr) { glDeleteShader(geomID); }

    if (linkCode == 0) {
        glDeleteProgram(programID);
        return 0;
    }

    std::cout << "[SHADER] program " << programID << " linking successful" << std::endl;
    return programID;
}

unsigned int Shader::checkCompileErrors(unsigned int shaderOrProgram, std::string type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        // Shader compile errors
        glGetShaderiv(shaderOrProgram, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shaderOrProgram, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << '\n';
            return 0;
        }
        else {
            std::cout << "[SHADER] shader compilation successful of type " << type << std::endl;
            return 1;
        }
    }
    else {
        // Program linking errors
        glGetProgramiv(shaderOrProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderOrProgram, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << '\n';
            return 0;
        }
        
        return 1;
    }
}
