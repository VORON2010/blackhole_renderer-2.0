#pragma once

#include <glad/glad.h>
#include <string>

std::string readFile(const std::string& path);
GLuint compileShader(const std::string& source, GLenum type);
GLuint createProgram(const std::string& fragSource);
