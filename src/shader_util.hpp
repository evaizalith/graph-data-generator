#ifndef EVA_SHADER_UTIL
#define EVA_SHADER_UTIL

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

GLuint createShaderProgram(const std::string& computePath);
GLuint createShaderProgram(const std::string& vertPath, const std::string& fragPath);
GLuint compileShader(GLenum type, const std::string& source);
std::string readFile(const std::string& path);

#endif
