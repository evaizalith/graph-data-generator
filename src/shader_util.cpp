#include <iostream>
#include <fstream>
#include <sstream>
#include "shader_util.hpp"
#include <filesystem>

const std::string SHADER_PATH = "../shaders/";

GLuint compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error (" << type << "):\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

std::string readFile(const std::string& path) {
    std::ifstream file(SHADER_PATH + path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint createShaderProgram(const std::string& computePath) {
    std::string computeSource = readFile(computePath);
    GLuint computeShader = compileShader(GL_COMPUTE_SHADER, computeSource);
    
    if (!computeShader) return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Compute program linking error:\n" << infoLog << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(computeShader);
    return program;
}

GLuint createShaderProgram(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSource = readFile(vertPath);
    std::string fragSource = readFile(fragPath);
    
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSource);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource);
    
    if (!vertShader || !fragShader) {
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Render program linking error:\n" << infoLog << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return program;
}
